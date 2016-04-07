
/*********************************************************************
 * INCLUDES
 */
#include <string.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>

#include "hci_tl.h"
#include "gatt.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "simpleGATTprofile.h"

#if defined(SENSORTAG_HW)
#include "bsp_spi.h"
#endif // SENSORTAG_HW

#if defined(FEATURE_OAD) || defined(IMAGE_INVALIDATE)
#include "oad_target.h"
#include "oad.h"
#endif //FEATURE_OAD || IMAGE_INVALIDATE

#include "peripheral.h"
#include "gapbondmgr.h"

#include "osal_snv.h"
#include "ICallBleAPIMSG.h"

//#include "util.h"
//#include "board_key.h"
//#include "board_lcd.h"
//#include "board_led.h"
//#include "Board.h"

//#include "simpleBLEPeripheral.h"

//#include <ti/drivers/lcd/LCDDogm1286.h>
#include "board_buzzer.h"

// Entity ID globally used to check for source and/or destination of messages
static ICall_EntityID BuzzerSelfEntity;

// Semaphore globally used to post events to the application thread
ICall_Semaphore BuzzerSem;


// Task pending events
static uint16_t events = 0;


#ifndef SBP_TASK_STACK_SIZE
#define SBP_TASK_STACK_SIZE                   644
#endif

// Task configuration
Task_Struct sbpBuzzerTask;
Char sbpBuzzerTaskStack[SBP_TASK_STACK_SIZE];



static void Buzzer_taskFxn(UArg a0, UArg a1)
{  
  for (;;)
  {
    // Waits for a signal to the semaphore associated with the calling thread.
    // Note that the semaphore associated with a thread is signaled when a
    // message is queued to the message receive queue of the thread or when
    // ICall_signal() function is called onto the semaphore.
    ICall_Errno errno = ICall_wait(ICALL_TIMEOUT_FOREVER);
    
    if (events & BUZZER_EVT)
    {
         events &= ~BUZZER_EVT;
         
         int count = 0;  
         
         while(1)
         {
             GPIOPinToggle(GPIO_PIN_13);
             Task_sleep(1*1000/Clock_tickPeriod/8);
    
             if(++count > 500)
             {
                GPIOPinWrite(GPIO_PIN_13, 0);
                break;    
             }      
         }
    }
  }
}

void Board_createTask4Buzzer()
{
  
    // Register the current thread as an ICall dispatcher application
  // so that the application can send and receive messages.
  //ICall_registerApp(&BuzzerSelfEntity, &BuzzerSem);
  
  
  Task_Params taskParams;

  // Configure task
  Task_Params_init(&taskParams);
  taskParams.stack = sbpBuzzerTaskStack;
  taskParams.stackSize = 644;
  taskParams.priority = 1;




  Task_construct(&sbpBuzzerTask, Buzzer_taskFxn, &taskParams, NULL);


}

void Board_Buzzer_Semaphore_post(uint16_t message)
{
    events = message;
    Semaphore_post(BuzzerSem);
}









