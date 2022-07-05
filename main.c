/*	last modified :  1:00 PM , Saturday, July 02, 2022
 *	in this project We have 2 senders, 1 receiver, and Queue. We send messages
 *	to Queue and count the number of failed and success sending
 *
 *
 *
 * This Project was made by:
 * - Abdelrhman Ahmed   	- Fatma Adel
 *
 * 	Copyright (c) 2014 Liviu Ionescu.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "Semphr.h"

#define CCM_RAM __attribute__((section(".ccmram")))

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"


//**********************  Global Counters and constant  **********************
unsigned int succ_counter = 0; 	//counter counting total number of transmitted messages
unsigned int fail_counter = 0;	//counter counting total number of blocked messages
unsigned int receive_counter = 0;	//counter counting total number of received message in the queue.

const unsigned int lower_bounds[6] = {50, 80, 110, 140, 170, 200};
const unsigned int upper_bounds[6] = {150, 200, 250, 300, 350, 400};

int loop_iteration = 0; //it is for check if array of time distribution finished or not

const unsigned int SIZE_OF_QUEUE = 2;
const unsigned int SIZE_OF_SENDED_MESSAGE = 15;
const TickType_t FIXED_TIME = pdMS_TO_TICKS ( 100 );
TickType_t RANDOM_TIME1 = pdMS_TO_TICKS ( 100 );
TickType_t RANDOM_TIME2 = pdMS_TO_TICKS ( 100 );

//*****************************  Handlers Definitions  *****************************
xQueueHandle QueueHandler = NULL;
SemaphoreHandle_t BinaryHandleForSender1 = NULL;
SemaphoreHandle_t BinaryHandleForSender2 = NULL;
SemaphoreHandle_t BinaryHandleForReceiver = NULL;
SemaphoreHandle_t MutexHandle = NULL;
TimerHandle_t   xTimer1Handler = NULL
			  , xTimer2Handler = NULL
			  , xTimer3Handler = NULL;

BaseType_t xTimer1Started = 0
		 , xTimer2Started = 0
		 , xTimer3Started = 0;

//*****************************  helpful function  *****************************

//get random number for timer using normal distribution
void UpdateRandomValue()
{
    srand (time(NULL));
    static int get_random1 , get_random2 , range ;
    static double RandFun1 , RandFun2;
    RandFun1 = rand() / (1.0 + RAND_MAX);
    RandFun2 = rand() / (1.0 + RAND_MAX);
    range = upper_bounds [loop_iteration] - lower_bounds [loop_iteration] +1 ;
    get_random1 = (int)(RandFun1 * range)+ lower_bounds [loop_iteration] ;
    	RANDOM_TIME1 = pdMS_TO_TICKS ( get_random1 );
    get_random2 = (int)(RandFun2 * range)+ lower_bounds [loop_iteration] ;
    RANDOM_TIME2 = pdMS_TO_TICKS ( get_random2 );
}

//reset function
void Reset()
{
	if( receive_counter != 0 ) //because when called from main for the first time
	{
		printf("\n total number of successfully sent messages : %d\n",succ_counter );
		printf("total number of blocked messages : %d \n",fail_counter );
		loop_iteration++;
	}

	if(loop_iteration == 6)
	{
		printf("\n\n\n  ***************************  Game Over  ***************************  \n\n\n ");
		xTimerDelete(xTimer1Handler , NULL);
		xTimerDelete(xTimer2Handler , NULL);
		xTimerDelete(xTimer3Handler , NULL);
		vTaskEndScheduler();
	}

	printf("\n\n  ---------------------------  loop_iteration : %d  ---------------------------  \n\n" , loop_iteration+1);

	succ_counter = 0;
	fail_counter = 0;
	receive_counter = 0;

	xQueueReset(QueueHandler);

	UpdateRandomValue(); //change the random value to new one
	xTimerChangePeriod( xTimer1Handler , RANDOM_TIME1 , NULL);
	//printf("RANDOM_TIME for Timer 1 : %d \n\n", (int)RANDOM_TIME1);

	UpdateRandomValue(); //change the random value to new one
	xTimerChangePeriod( xTimer2Handler , RANDOM_TIME2 , NULL);
	//printf("RANDOM_TIME for Timer 2 : %d \n\n", (int)RANDOM_TIME2);
}

//*****************************  Tasks  *****************************
void sender_1(void *ptr)
{
	char send_message[50];
	BaseType_t check;	// check for Queue state
	while(1)
	{
		xSemaphoreTake(BinaryHandleForSender1 , RANDOM_TIME1); //if the Semaphore is free
		sprintf(send_message, "Time is %lu ", (long unsigned int)xTaskGetTickCount() ); //get current Tick time

		check = xQueueSend(QueueHandler , &send_message , 0); //check if the queue is not full

		if ( check == pdPASS )
		{
			printf("success to send massage from sender_1 \n");
			xSemaphoreTake(MutexHandle,100);
			succ_counter++;
			xSemaphoreGive(MutexHandle);
		}
		else
		{
			printf("failed to send massage from sender_1 \n");
			xSemaphoreTake(MutexHandle,100);
			fail_counter++ ;
			xSemaphoreGive(MutexHandle);
		}
		UpdateRandomValue(); //change the random value to new one
		xTimerChangePeriod( xTimer1Handler , RANDOM_TIME1 , NULL);
		//printf("RANDOM_TIME for Timer 1 : %d \n\n", (int)RANDOM_TIME1);
	}
}

void sender_2(void *ptr)
{
	char send_message[50];
	BaseType_t check;		// check for Queue state
	while(1)
	{
		xSemaphoreTake(BinaryHandleForSender2 , RANDOM_TIME2);
		sprintf(send_message, "Time is %lu ", (long unsigned int)xTaskGetTickCount() );

		check = xQueueSend(QueueHandler , &send_message ,0); //check if the queue not full

		if ( check == pdPASS )
		{
			printf("success to send massage from sender_2 \n");
			xSemaphoreTake(MutexHandle,100);
			succ_counter++;
			xSemaphoreGive(MutexHandle);
		}
		else
		{
			printf("failed to send massage from sender_2 \n");
			xSemaphoreTake(MutexHandle,100);
			fail_counter++ ;
			xSemaphoreGive(MutexHandle);
		}

			UpdateRandomValue(); //change the random value to new one
			xTimerChangePeriod( xTimer2Handler , RANDOM_TIME2 , NULL);
			//printf("\n\n RANDOM_TIME for Timer 2 : %d \n\n", (int)RANDOM_TIME2);
	}
}

void receiver(void *ptr)
{
	char rec_message[50] ;
	BaseType_t check;		// check for Queue state
	while(1)
	{
		xSemaphoreTake(BinaryHandleForReceiver , FIXED_TIME);

		check = xQueueReceive(QueueHandler , &rec_message ,0);

		if ( check == pdPASS )
		{
			printf("%d - success to receive massage : %s \n\n",receive_counter+1,rec_message);
			xSemaphoreTake(MutexHandle,100);
			receive_counter++;
			xSemaphoreGive(MutexHandle);
		/*	UpdateRandomValue(); //change the random value to new one
			xTimerChangePeriod( xTimer1Handler , RANDOM_TIME1 , NULL);
			printf("\n\n RANDOM_TIME for Timer 1 : %d \n\n", (int)RANDOM_TIME1);

			UpdateRandomValue(); //change the random value to new one
			xTimerChangePeriod( xTimer2Handler , RANDOM_TIME2 , NULL);
			printf("\n\n RANDOM_TIME for Timer 2 : %d \n\n", (int)RANDOM_TIME2);*/
		}
		else
		{
			printf("failed to receive the massage. \n\n");
		}
		if (receive_counter == 500)
		{
			Reset();
		}
	}
}

//*******************************  Timer call back function  *******************************

static void Sender1TimerCallBack()
{
	xSemaphoreGive(BinaryHandleForSender1);
}

static void Sender2TimerCallBack()
{
	xSemaphoreGive(BinaryHandleForSender2);
}

static void ReceiverTimerCallBack()
{
	xSemaphoreGive(BinaryHandleForReceiver);
}


//************************************  main funcion  ************************************

int main(int argc, char* argv[])
{
	//########  create tasks & queue & timers  ########
	QueueHandler = xQueueCreate(SIZE_OF_QUEUE , 20 * sizeof(char));

	if(QueueHandler != NULL)
	{
		xTaskCreate(sender_1 , "sender1" , 1024 , NULL , 1 , NULL);
		xTaskCreate(sender_2 , "sender2" , 1024 , NULL , 1 , NULL);
		xTaskCreate(receiver , "receiver" , 1024 , NULL , 2 , NULL);
	}

	vSemaphoreCreateBinary(BinaryHandleForSender1);
	vSemaphoreCreateBinary(BinaryHandleForSender2);
	vSemaphoreCreateBinary(BinaryHandleForReceiver);
	MutexHandle = xSemaphoreCreateMutex();
	xTimer1Handler = xTimerCreate( "Timer1", ( pdMS_TO_TICKS(RANDOM_TIME1) ), pdTRUE, ( void * ) 0, Sender1TimerCallBack);
	xTimer2Handler = xTimerCreate( "Timer2", ( pdMS_TO_TICKS(RANDOM_TIME2) ), pdTRUE, ( void * ) 0, Sender2TimerCallBack);
	xTimer3Handler = xTimerCreate( "Timer3", ( pdMS_TO_TICKS(FIXED_TIME) ) , pdTRUE, ( void * ) 0, ReceiverTimerCallBack);

	if( ( xTimer1Handler != NULL ) && ( xTimer2Handler != NULL ) && ( xTimer3Handler != NULL ) && QueueHandler != NULL )
	{
		xTimer1Started = xTimerStart( xTimer1Handler, 0 );
		xTimer2Started = xTimerStart( xTimer2Handler, 0 );
		xTimer3Started = xTimerStart( xTimer2Handler, 0 );
	}

	Reset(); //call the reset function to initialize

	//if every thing is create without any mistake, start scheduler
	if( xTimer1Started == pdPASS && xTimer2Started == pdPASS && xTimer3Started == pdPASS)
	{
		vTaskStartScheduler();
	}
	else
	{
		printf("error to Start Task Scheduler ... \n");
		exit(0);
	}

	return 0;

}




#pragma GCC diagnostic pop



void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
