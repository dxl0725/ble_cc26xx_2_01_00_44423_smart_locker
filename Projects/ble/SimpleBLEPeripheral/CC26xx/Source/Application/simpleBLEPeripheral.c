/**************************************************************************************************
  Filename:       simpleBLEPeripheral.c
  Revised:        $Date: 2015-07-13 11:43:11 -0700 (Mon, 13 Jul 2015) $
  Revision:       $Revision: 44336 $

  Description:    This file contains the Simple BLE Peripheral sample application
                  for use with the CC2650 Bluetooth Low Energy Protocol Stack.

  Copyright 2013 - 2015 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

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

#include "util.h"
#include "board_lcd.h"
#include "board_key.h"
#include "Board.h"

#include "simpleBLEPeripheral.h"

#include <ti/drivers/lcd/LCDDogm1286.h>

//2016.02.22
#include <driverlib/aon_rtc.h>
#include <driverlib/aon_batmon.h>

/*********************************************************************
 * CONSTANTS
 */
// Advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          160

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

#ifndef FEATURE_OAD
// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic
// parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     80

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic
// parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     800
#else
// Minimum connection interval (units of 1.25ms, 8=10ms) if automatic
// parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     8

// Maximum connection interval (units of 1.25ms, 8=10ms) if automatic
// parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     8
#endif // FEATURE_OAD

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter
// update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          1000

// Whether to enable automatic parameter update request when a connection is
// formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         TRUE

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL         6

// How often to perform periodic event (in msec)
#define SBP_PERIODIC_EVT_PERIOD               5000

#ifdef FEATURE_OAD
// The size of an OAD packet.
#define OAD_PACKET_SIZE                       ((OAD_BLOCK_SIZE) + 2)
#endif // FEATURE_OAD

// Task configuration
#define SBP_TASK_PRIORITY                     1




#ifndef SBP_TASK_STACK_SIZE
#define SBP_TASK_STACK_SIZE                   644
#endif

// Internal Events for RTOS application
#define SBP_STATE_CHANGE_EVT                  0x0001
#define SBP_CHAR_CHANGE_EVT                   0x0002
#define SBP_PERIODIC_EVT                      0x0004
#define SBC_KEY_CHANGE_EVT                    0x0008
#define SBP_CONN_EVT_END_EVT                  0x0010
 
// Company Identifier: Texas Instruments Inc. (13)
#define APPLE_COMPANY_ID                      0x004C

#define IBEACON_DATA_ID_OFS                   7
#define IBEACON_DATA_UUID_OFS                 9
#define IBEACON_DATA_MAJOR_OFS                25
#define IBEACON_DATA_MINOR_OFS                27
#define IBEACON_DATA_TXPWR_OFS                29

// App event passed from profiles.
typedef struct
{
  appEvtHdr_t hdr;  // event header.
} sbpEvt_t;


//bonding 2015.12.09
typedef enum  
{  
  PAIRSTATUS_PAIRED = 0,  
  PAIRSTATUS_NO_PAIRED,  
}PAIRSTATUS;  
static PAIRSTATUS gPairStatus = PAIRSTATUS_NO_PAIRED;//Åä¶Ô×´Ì¬£¬Ä¬ÈÏÊÇÃ»Åä¶Ô 

/*********************************************************************
 * LOCAL VARIABLES
 */

#define delay_ms(i) Task_sleep( ((i) * 1000) / Clock_tickPeriod )

// Global pin resources
PIN_State pinGpioState;
PIN_Handle hGpioPin;



/*********************************************************************
 * LOCAL VARIABLES
 */

// Entity ID globally used to check for source and/or destination of messages
static ICall_EntityID selfEntity;

// Semaphore globally used to post events to the application thread
static ICall_Semaphore sem;

// Clock instances for internal periodic events.
static Clock_Struct periodicClock;

// Queue object used for app messages
static Queue_Struct appMsg;
static Queue_Handle appMsgQueue;

#if defined(FEATURE_OAD)
// Event data from OAD profile.
static Queue_Struct oadQ;
static Queue_Handle hOadQ;
#endif //FEATURE_OAD

// events flag for internal application events.
static uint16_t events;

// Task configuration
Task_Struct sbpTask;
Char sbpTaskStack[SBP_TASK_STACK_SIZE];

// Profile state and parameters
//static gaprole_States_t gapProfileState = GAPROLE_INIT;

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[] =
{
  // complete name
  0x0d,   // length of this data
  GAP_ADTYPE_LOCAL_NAME_COMPLETE,
  'S',
  'm',
  'a',
  'r',
  't',
  0x2d,
  'L',
  'o',
  'c',
  'k',
  'e',
  'r',
  // connection interval range
  0x05,   // length of this data
  GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
  LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),   // 100ms
  HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
  LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),   // 1s
  HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

  // Tx power level
  0x02,   // length of this data
  GAP_ADTYPE_POWER_LEVEL,
  0       // 0dBm
};


// GAP GATT Attributes
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "Smart_Locker";
static uint8 simpleBLEChan1[SIMPLEPROFILE_CHAR1_LEN] = {0};
static uint8 simpleBLEChan2[SIMPLEPROFILE_CHAR2_LEN] = {0};
static uint8 simpleBLEChan3[SIMPLEPROFILE_CHAR3_LEN] = {0};
static uint8 simpleBLEChan4[SIMPLEPROFILE_CHAR4_LEN] = {0};
static uint8 simpleBLEChan5[SIMPLEPROFILE_CHAR5_LEN] = {0};
static uint8 simpleBLEChan6[SIMPLEPROFILE_CHAR6_LEN] = {0};
static uint8 simpleBLEChan7[SIMPLEPROFILE_CHAR7_LEN] = {0};

static uint8 simpleBLEChan9[SIMPLEPROFILE_CHAR9_LEN] = {0};
static uint8 simpleBLEChan10[SIMPLEPROFILE_CHAR10_LEN] = {0};
static uint8 beaconData[] =
{
  // Flags;
  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
	
  // load;
  0x1A,   // length of this data
  0xFF,   // manufacturer spec data
  LO_UINT16( APPLE_COMPANY_ID ), //company ID code
  HI_UINT16( APPLE_COMPANY_ID ),
  0x02, //ibeacon id      7
  0x15,
  0xC1, //16 bytes uuid   9
  0x4D,
  0x2C,
  0x0A,
  0x40,
  0x1F,
  0xB7,
  0xA9,
  0x84,
  0x1F,
  0xE2,
  0xE9,
  0x3B,
  0x80,
  0xF6,
  0x31,
  0x01,//major  25
  0x06,
  0x01,//minor  27
  0x00,
  0xD3//calibrated tx power
};
static uint8 echoData1[20] ={0};
static uint8 echoData2[20] ={0};
static uint8 echoData3[20] ={0};
static uint8 echoData4[20] ={0};
static uint8 echoData5[20] ={0};
static uint8 echoData6[20] ={0};
static uint8 echoData7[20] ={0};
static uint8 echoData9[20] ={0};
static uint8 echoData10[20] ={0};
// Globals used for ATT Response retransmission
static gattMsgEvent_t *pAttRsp = NULL;
static uint8_t rspTxRetry = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void SimpleBLEPeripheral_init( void );
static void SimpleBLEPeripheral_taskFxn(UArg a0, UArg a1);

static uint8_t SimpleBLEPeripheral_processStackMsg(ICall_Hdr *pMsg);
static uint8_t SimpleBLEPeripheral_processGATTMsg(gattMsgEvent_t *pMsg);
static void SimpleBLEPeripheral_processAppMsg(sbpEvt_t *pMsg);
static void SimpleBLEPeripheral_processStateChangeEvt(gaprole_States_t newState);
static void SimpleBLEPeripheral_processCharValueChangeEvt(uint8_t paramID);
static void SimpleBLEPeripheral_performPeriodicTask(void);

static void SimpleBLEPeripheral_sendAttRsp(void);
static void SimpleBLEPeripheral_freeAttRsp(uint8_t status);

static void SimpleBLEPeripheral_stateChangeCB(gaprole_States_t newState);
#ifndef FEATURE_OAD
static void SimpleBLEPeripheral_charValueChangeCB(uint8_t paramID);
#endif //!FEATURE_OAD
static void SimpleBLEPeripheral_enqueueMsg(uint8_t event, uint8_t state);

#ifdef FEATURE_OAD
void SimpleBLEPeripheral_processOadWriteCB(uint8_t event, uint16_t connHandle,
                                           uint8_t *pData);
#endif //FEATURE_OAD

static void SimpleBLEPeripheral_clockHandler(UArg arg);

void Smart_Locker_blinkLed(uint8_t led, uint8_t nBlinks);
void Smart_Locker_blinkLed4(uint8_t nBlinks);
static void SimpleBLECentral_handleKeys(uint8_t shift, uint8_t keys);
void SimpleBLECentral_keyChangeHandler(uint8 keys);

//bonding 2015.12.09
static void SimpleBLEPeripheral_processPasscode(uint8 *deviceAddr,uint16 connectionHandle,uint8 uiInputs,uint8 uiOutputs );
static void ProcessPairStateCB( uint16 connHandle, uint8 state, uint8 status );  

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t SimpleBLEPeripheral_gapRoleCBs =
{
  SimpleBLEPeripheral_stateChangeCB     // Profile State Change Callbacks
};

// GAP Bond Manager Callbacks
static gapBondCBs_t simpleBLEPeripheral_BondMgrCBs =
{
  NULL, // Passcode callback
  ProcessPairStateCB // Pairing state callback
  //NULL, // Passcode callback (not used by application)
  //NULL  // Pairing / Bonding state Callback (not used by application)
};

// Simple GATT Profile Callbacks
#ifndef FEATURE_OAD
static simpleProfileCBs_t SimpleBLEPeripheral_simpleProfileCBs =
{
  SimpleBLEPeripheral_charValueChangeCB // Characteristic value change callback
};
#endif //!FEATURE_OAD

#ifdef FEATURE_OAD
static oadTargetCBs_t simpleBLEPeripheral_oadCBs =
{
  SimpleBLEPeripheral_processOadWriteCB // Write Callback.
};
#endif //FEATURE_OAD

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleBLEPeripheral_createTask
 *
 * @brief   Task creation function for the Simple BLE Peripheral.
 *
 * @param   None.
 *
 * @return  None.
 */
void SimpleBLEPeripheral_createTask(void)
{
  Task_Params taskParams;

  // Configure task
  Task_Params_init(&taskParams);
  taskParams.stack = sbpTaskStack;
  taskParams.stackSize = SBP_TASK_STACK_SIZE;
  taskParams.priority = SBP_TASK_PRIORITY;

  Task_construct(&sbpTask, SimpleBLEPeripheral_taskFxn, &taskParams, NULL);
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_init
 *
 * @brief   Called during initialization and contains application
 *          specific initialization (ie. hardware initialization/setup,
 *          table initialization, power up notification, etc), and
 *          profile initialization/setup.
 *
 * @param   None.
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_init(void)
{
  
  
  // Handling of buttons, relay and MPU interrupt
  hGpioPin = PIN_open(&pinGpioState, BoardGpioInitTable);
  
  
  // ******************************************************************
  // N0 STACK API CALLS CAN OCCUR BEFORE THIS CALL TO ICall_registerApp
  // ******************************************************************
  // Register the current thread as an ICall dispatcher application
  // so that the application can send and receive messages.
  ICall_registerApp(&selfEntity, &sem);

  // Hard code the BD Address till CC2650 board gets its own IEEE address
//  uint8 bdAddress[B_ADDR_LEN] = { 0xAD, 0xD0, 0x0A, 0xAD, 0xD0, 0x0A };
//  HCI_EXT_SetBDADDRCmd(bdAddress);

  // Set device's Sleep Clock Accuracy
  //HCI_EXT_SetSCACmd(40);

  // Create an RTOS queue for message from profile to be sent to app.
  appMsgQueue = Util_constructQueue(&appMsg);

  // Create one-shot clocks for internal periodic events.
  Util_constructClock(&periodicClock, SimpleBLEPeripheral_clockHandler,
                      SBP_PERIODIC_EVT_PERIOD, 0, false, SBP_PERIODIC_EVT);
  
  Board_initKeys(SimpleBLECentral_keyChangeHandler);

#ifndef SENSORTAG_HW
  Board_openLCD();
#endif //SENSORTAG_HW
  
#if SENSORTAG_HW
  // Setup SPI bus for serial flash and Devpack interface
  bspSpiOpen();
#endif //SENSORTAG_HW
  
  
//Smart_Locker
//  //led
//  Smart_Locker_blinkLed(Board_LED1, 10);
//  Smart_Locker_blinkLed(Board_LED2, 10);
//  PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_ON);
//  PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_ON);
//  //motor
//  PIN_setOutputValue(hGpioPin, Board_IA, Board_LED_ON);
//  PIN_setOutputValue(hGpioPin, Board_IB, Board_LED_OFF);
  
  
  
  // Setup the GAP
  GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL);

  // Setup the GAP Peripheral Role Profile
  {
    // For all hardware platforms, device starts advertising upon initialization
    uint8_t initialAdvertEnable = TRUE;

    // By setting this to zero, the device will go into the waiting state after
    // being discoverable for 30.72 second, and will not being advertising again
    // until the enabler is set back to TRUE
    uint16_t advertOffTime = 0;

    uint8_t enableUpdateRequest = DEFAULT_ENABLE_UPDATE_REQUEST;
    uint16_t desiredMinInterval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
    uint16_t desiredMaxInterval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
    uint16_t desiredSlaveLatency = DEFAULT_DESIRED_SLAVE_LATENCY;
    uint16_t desiredConnTimeout = DEFAULT_DESIRED_CONN_TIMEOUT;

    // Set the GAP Role Parameters
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t),
                         &initialAdvertEnable);
    GAPRole_SetParameter(GAPROLE_ADVERT_OFF_TIME, sizeof(uint16_t),
                         &advertOffTime);

    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData),
                         scanRspData);
//    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);

    GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE, sizeof(uint8_t),
                         &enableUpdateRequest);
    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t),
                         &desiredMinInterval);
    GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t),
                         &desiredMaxInterval);
    GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(uint16_t),
                         &desiredSlaveLatency);
    GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(uint16_t),
                         &desiredConnTimeout);
  }
  
  
  {
    osal_snv_read( BLE_NVID_CHAR1, SIMPLEPROFILE_CHAR1_LEN, simpleBLEChan1);
    osal_snv_read( BLE_NVID_CHAR2, SIMPLEPROFILE_CHAR2_LEN, simpleBLEChan2);
    osal_snv_read( BLE_NVID_CHAR3, SIMPLEPROFILE_CHAR3_LEN, simpleBLEChan3);
    osal_snv_read( BLE_NVID_CHAR4, SIMPLEPROFILE_CHAR4_LEN, simpleBLEChan4);
    osal_snv_read( BLE_NVID_CHAR5, SIMPLEPROFILE_CHAR5_LEN, simpleBLEChan5); 
    osal_snv_read( BLE_NVID_CHAR6, SIMPLEPROFILE_CHAR6_LEN, simpleBLEChan6);
    osal_snv_read( BLE_NVID_CHAR7, SIMPLEPROFILE_CHAR7_LEN, simpleBLEChan7);
    osal_snv_read( BLE_NVID_CHAR8, SIMPLEPROFILE_CHAR8_LEN, attDeviceName);
    osal_snv_read( BLE_NVID_CHAR9, SIMPLEPROFILE_CHAR9_LEN, simpleBLEChan9);
    osal_snv_read( BLE_NVID_CHAR10, SIMPLEPROFILE_CHAR10_LEN, simpleBLEChan10);
    
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR1, SIMPLEPROFILE_CHAR1_LEN, simpleBLEChan1 );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR2, SIMPLEPROFILE_CHAR2_LEN, simpleBLEChan2 );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR3, SIMPLEPROFILE_CHAR3_LEN, simpleBLEChan3 );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR4, SIMPLEPROFILE_CHAR4_LEN, simpleBLEChan4 );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR5, SIMPLEPROFILE_CHAR5_LEN, simpleBLEChan5 );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR6, SIMPLEPROFILE_CHAR6_LEN, simpleBLEChan6 );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR7, SIMPLEPROFILE_CHAR7_LEN, simpleBLEChan7 );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR8, SIMPLEPROFILE_CHAR8_LEN, attDeviceName );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR9, SIMPLEPROFILE_CHAR9_LEN, simpleBLEChan9 );
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR10, SIMPLEPROFILE_CHAR10_LEN, simpleBLEChan10 );
    
    //HCI_EXT_SetBDADDRCmd(simpleBLEIbeaconbdAddress);  //ÐÞ¸ÄMACµØÖ·£¬ÈçÐèÐÞ¸Ä´ò¿ª¼´¿É£¨ÐèÖØÆô£©
    
    /**********************************Èç¹ûÑ¡Ôñ·ÇÒ×Ê§´æ´¢Êý¾Ý£¬¿ªÆô´Ë²¿·Ö´úÂë ***********************************/
//    memcpy(&beaconData[IBEACON_DATA_MAJOR_OFS], simpleBLEChan1, SIMPLEPROFILE_CHAR1_LEN);
//    memcpy(&beaconData[IBEACON_DATA_MINOR_OFS], simpleBLEChan1, SIMPLEPROFILE_CHAR2_LEN);
//    memcpy(&beaconData[IBEACON_DATA_UUID_OFS], simpleBLEChan6, SIMPLEPROFILE_CHAR6_LEN);
  }
  
  // Set the GAP Characteristics
  GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);
  
  // Set advertising interval
  {
    uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;

    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);
  }
  

  // Setup the GAP Bond Manager
  {
    uint32_t passkey = 654321; // passkey "000000"
    //uint8_t pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8_t pairMode = GAPBOND_PAIRING_MODE_INITIATE;
    uint8_t mitm = TRUE;
    uint8_t ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
    uint8_t bonding = TRUE;

    GAPBondMgr_SetParameter(GAPBOND_DEFAULT_PASSCODE, sizeof(uint32_t),
                            &passkey);
    GAPBondMgr_SetParameter(GAPBOND_PAIRING_MODE, sizeof(uint8_t), &pairMode);
    GAPBondMgr_SetParameter(GAPBOND_MITM_PROTECTION, sizeof(uint8_t), &mitm);
    GAPBondMgr_SetParameter(GAPBOND_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
    GAPBondMgr_SetParameter(GAPBOND_BONDING_ENABLED, sizeof(uint8_t), &bonding);
  }

   // Initialize GATT attributes
  GGS_AddService(GATT_ALL_SERVICES);           // GAP
  GATTServApp_AddService(GATT_ALL_SERVICES);   // GATT attributes
  DevInfo_AddService();                        // Device Information Service

#ifndef FEATURE_OAD
  SimpleProfile_AddService(GATT_ALL_SERVICES); // Simple GATT Profile
#endif //!FEATURE_OAD

#ifdef FEATURE_OAD
  VOID OAD_addService();                 // OAD Profile
  OAD_register((oadTargetCBs_t *)&simpleBLEPeripheral_oadCBs);
  hOadQ = Util_constructQueue(&oadQ);
#endif

#ifdef IMAGE_INVALIDATE
  Reset_addService();
#endif //IMAGE_INVALIDATE
  
  
#ifndef FEATURE_OAD
  // Setup the SimpleProfile Characteristic Values
  {
    uint8_t charValue1[SIMPLEPROFILE_CHAR5_LEN] = { 0 };
    uint8_t charValue2[SIMPLEPROFILE_CHAR5_LEN] = { 0 };
    uint8_t charValue3[SIMPLEPROFILE_CHAR5_LEN] = { 0 };
    uint8_t charValue4 = 0 ;
    uint8_t charValue5[SIMPLEPROFILE_CHAR5_LEN] = { 0 };
    uint8_t charValue6[SIMPLEPROFILE_CHAR5_LEN] = { 0 };
    uint8_t charValue7[SIMPLEPROFILE_CHAR5_LEN] = { 0 };

    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR1, sizeof(uint8_t),
                               charValue1);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR2, sizeof(uint8_t),
                               charValue2);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR3, sizeof(uint8_t),
                               charValue3);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR4, sizeof(uint8_t),
                               &charValue4);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR5, SIMPLEPROFILE_CHAR5_LEN,
                               charValue5);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR5, SIMPLEPROFILE_CHAR5_LEN,
                               charValue6);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR5, SIMPLEPROFILE_CHAR5_LEN,
                               charValue7);
  }

  // Register callback with SimpleGATTprofile
  SimpleProfile_RegisterAppCBs(&SimpleBLEPeripheral_simpleProfileCBs);
#endif //!FEATURE_OAD

  // Start the Device
  VOID GAPRole_StartDevice(&SimpleBLEPeripheral_gapRoleCBs);

  // Start Bond Manager
  VOID GAPBondMgr_Register(&simpleBLEPeripheral_BondMgrCBs);

  // Register with GAP for HCI/Host messages
  GAP_RegisterForMsgs(selfEntity);
  
  // Register for GATT local events and ATT Responses pending for transmission
  GATT_RegisterForMsgs(selfEntity);
  
#if defined FEATURE_OAD
#if defined (HAL_IMAGE_A)
  LCD_WRITE_STRING("BLE Peripheral A", LCD_PAGE0);
#else
  LCD_WRITE_STRING("BLE Peripheral B", LCD_PAGE0);
#endif // HAL_IMAGE_A
#else
  LCD_WRITE_STRING("BLE Peripheral", LCD_PAGE0);
#endif // FEATURE_OAD
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_taskFxn
 *
 * @brief   Application task entry point for the Simple BLE Peripheral.
 *
 * @param   a0, a1 - not used.
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_taskFxn(UArg a0, UArg a1)
{
  // Initialize application
  SimpleBLEPeripheral_init();

  // Application main loop
  for (;;)
  {
    // Waits for a signal to the semaphore associated with the calling thread.
    // Note that the semaphore associated with a thread is signaled when a
    // message is queued to the message receive queue of the thread or when
    // ICall_signal() function is called onto the semaphore.
    ICall_Errno errno = ICall_wait(ICALL_TIMEOUT_FOREVER);

    if (errno == ICALL_ERRNO_SUCCESS)
    {
      ICall_EntityID dest;
      ICall_ServiceEnum src;
      ICall_HciExtEvt *pMsg = NULL;

      if (ICall_fetchServiceMsg(&src, &dest,
                                (void **)&pMsg) == ICALL_ERRNO_SUCCESS)
      {
        uint8 safeToDealloc = TRUE;
        
        if ((src == ICALL_SERVICE_CLASS_BLE) && (dest == selfEntity))
        {
          ICall_Event *pEvt = (ICall_Event *)pMsg;
          
          // Check for BLE stack events first
          if (pEvt->signature == 0xffff)
          {
            if (pEvt->event_flag & SBP_CONN_EVT_END_EVT)
            {
              // Try to retransmit pending ATT Response (if any)
              SimpleBLEPeripheral_sendAttRsp();
            }
          }
          else
          {
            // Process inter-task message
            safeToDealloc = SimpleBLEPeripheral_processStackMsg((ICall_Hdr *)pMsg);
          }
        }

        if (pMsg && safeToDealloc)
        {
          ICall_freeMsg(pMsg);
        }
      }

      // If RTOS queue is not empty, process app message.
      while (!Queue_empty(appMsgQueue))
      {
        sbpEvt_t *pMsg = (sbpEvt_t *)Util_dequeueMsg(appMsgQueue);
        if (pMsg)
        {
          // Process message.
          SimpleBLEPeripheral_processAppMsg(pMsg);

          // Free the space from the message.
          ICall_free(pMsg);
        }
      }
    }

    if (events & SBP_PERIODIC_EVT)
    {
      events &= ~SBP_PERIODIC_EVT;

      Util_startClock(&periodicClock);

      // Perform periodic application task
      SimpleBLEPeripheral_performPeriodicTask();
    }

#ifdef FEATURE_OAD
    while (!Queue_empty(hOadQ))
    {
      oadTargetWrite_t *oadWriteEvt = Queue_dequeue(hOadQ);

      // Identify new image.
      if (oadWriteEvt->event == OAD_WRITE_IDENTIFY_REQ)
      {
        OAD_imgIdentifyWrite(oadWriteEvt->connHandle, oadWriteEvt->pData);
      }
      // Write a next block request.
      else if (oadWriteEvt->event == OAD_WRITE_BLOCK_REQ)
      {
        OAD_imgBlockWrite(oadWriteEvt->connHandle, oadWriteEvt->pData);
      }

      // Free buffer.
      ICall_free(oadWriteEvt);
    }
#endif //FEATURE_OAD
  }
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_processStackMsg
 *
 * @brief   Process an incoming stack message.
 *
 * @param   pMsg - message to process
 *
 * @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
 */
static uint8_t SimpleBLEPeripheral_processStackMsg(ICall_Hdr *pMsg)
{
  uint8_t safeToDealloc = TRUE;
    
  switch (pMsg->event)
  {
    case GATT_MSG_EVENT:
      // Process GATT message
      safeToDealloc = SimpleBLEPeripheral_processGATTMsg((gattMsgEvent_t *)pMsg);
      break;

    case HCI_GAP_EVENT_EVENT:
      {
        // Process HCI message
        switch(pMsg->status)
        {
          case HCI_COMMAND_COMPLETE_EVENT_CODE:
            // Process HCI Command Complete Event
            break;
            
          default:
            break;
        }
      }
      break;
      
    default:
      // do nothing
      break;
  }
  
  return (safeToDealloc);
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_processGATTMsg
 *
 * @brief   Process GATT messages and events.
 *
 * @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
 */
static uint8_t SimpleBLEPeripheral_processGATTMsg(gattMsgEvent_t *pMsg)
{
  // See if GATT server was unable to transmit an ATT response
  if (pMsg->hdr.status == blePending)
  {
    // No HCI buffer was available. Let's try to retransmit the response
    // on the next connection event.
    if (HCI_EXT_ConnEventNoticeCmd(pMsg->connHandle, selfEntity,
                                   SBP_CONN_EVT_END_EVT) == SUCCESS)
    {
      // First free any pending response
      SimpleBLEPeripheral_freeAttRsp(FAILURE);
      
      // Hold on to the response message for retransmission
      pAttRsp = pMsg;
      
      // Don't free the response message yet
      return (FALSE);
    }
  }
  else if (pMsg->method == ATT_FLOW_CTRL_VIOLATED_EVENT)
  {
    // ATT request-response or indication-confirmation flow control is
    // violated. All subsequent ATT requests or indications will be dropped.
    // The app is informed in case it wants to drop the connection.
    
    // Display the opcode of the message that caused the violation.
    LCD_WRITE_STRING_VALUE("FC Violated:", pMsg->msg.flowCtrlEvt.opcode,
                           10, LCD_PAGE5);
  }    
  else if (pMsg->method == ATT_MTU_UPDATED_EVENT)
  {
    // MTU size updated
    LCD_WRITE_STRING_VALUE("MTU Size:", pMsg->msg.mtuEvt.MTU, 10, LCD_PAGE5);
  }
  
  // Free message payload. Needed only for ATT Protocol messages
  GATT_bm_free(&pMsg->msg, pMsg->method);
  
  // It's safe to free the incoming message
  return (TRUE);
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_sendAttRsp
 *
 * @brief   Send a pending ATT response message.
 *
 * @param   none
 *
 * @return  none
 */
static void SimpleBLEPeripheral_sendAttRsp(void)
{
  // See if there's a pending ATT Response to be transmitted
  if (pAttRsp != NULL)
  {
    uint8_t status;
    
    // Increment retransmission count
    rspTxRetry++;
    
    // Try to retransmit ATT response till either we're successful or
    // the ATT Client times out (after 30s) and drops the connection.
    status = GATT_SendRsp(pAttRsp->connHandle, pAttRsp->method, &(pAttRsp->msg));
    if ((status != blePending) && (status != MSG_BUFFER_NOT_AVAIL))
    {
      // Disable connection event end notice
      HCI_EXT_ConnEventNoticeCmd(pAttRsp->connHandle, selfEntity, 0);
      
      // We're done with the response message
      SimpleBLEPeripheral_freeAttRsp(status);
    }
    else
    {
      // Continue retrying
      LCD_WRITE_STRING_VALUE("Rsp send retry:", rspTxRetry, 10, LCD_PAGE5);
    }
  }
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_freeAttRsp
 *
 * @brief   Free ATT response message.
 *
 * @param   status - response transmit status
 *
 * @return  none
 */
static void SimpleBLEPeripheral_freeAttRsp(uint8_t status)
{
  // See if there's a pending ATT response message
  if (pAttRsp != NULL)
  {
    // See if the response was sent out successfully
    if (status == SUCCESS)
    {
      LCD_WRITE_STRING_VALUE("Rsp sent, retry:", rspTxRetry, 10, LCD_PAGE5);
    }
    else
    {
      // Free response payload
      GATT_bm_free(&pAttRsp->msg, pAttRsp->method);
      
      LCD_WRITE_STRING_VALUE("Rsp retry failed:", rspTxRetry, 10, LCD_PAGE5);
    }
    
    // Free response message
    ICall_freeMsg(pAttRsp);
    
    // Reset our globals
    pAttRsp = NULL;
    rspTxRetry = 0;
  }
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_processAppMsg
 *
 * @brief   Process an incoming callback from a profile.
 *
 * @param   pMsg - message to process
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_processAppMsg(sbpEvt_t *pMsg)
{
  switch (pMsg->hdr.event)
  {
    case SBP_STATE_CHANGE_EVT:
      SimpleBLEPeripheral_processStateChangeEvt((gaprole_States_t)pMsg->
                                                hdr.state);
      break;
      
    case SBC_KEY_CHANGE_EVT:
      SimpleBLECentral_handleKeys(0, pMsg->hdr.state); 
      break;

    case SBP_CHAR_CHANGE_EVT:
      SimpleBLEPeripheral_processCharValueChangeEvt(pMsg->hdr.state);
      break;
      

    default:
      // Do nothing.
      break;
  }
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_stateChangeCB
 *
 * @brief   Callback from GAP Role indicating a role state change.
 *
 * @param   newState - new state
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_stateChangeCB(gaprole_States_t newState)
{
  SimpleBLEPeripheral_enqueueMsg(SBP_STATE_CHANGE_EVT, newState);
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_processStateChangeEvt
 *
 * @brief   Process a pending GAP Role state change event.
 *
 * @param   newState - new state
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_processStateChangeEvt(gaprole_States_t newState)
{
#ifdef PLUS_BROADCASTER
  static bool firstConnFlag = false;
#endif // PLUS_BROADCASTER

  switch ( newState )
  {
    case GAPROLE_STARTED:
      {
        uint8_t ownAddress[B_ADDR_LEN];
        uint8_t systemId[DEVINFO_SYSTEM_ID_LEN];

        GAPRole_GetParameter(GAPROLE_BD_ADDR, ownAddress);

        // use 6 bytes of device address for 8 bytes of system ID value
        systemId[0] = ownAddress[0];
        systemId[1] = ownAddress[1];
        systemId[2] = ownAddress[2];

        // set middle bytes to zero
        systemId[4] = 0x00;
        systemId[3] = 0x00;

        // shift three bytes up
        systemId[7] = ownAddress[5];
        systemId[6] = ownAddress[4];
        systemId[5] = ownAddress[3];

        GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( beaconData ), beaconData );
        DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);

        // Display device address
        LCD_WRITE_STRING(Util_convertBdAddr2Str(ownAddress), LCD_PAGE1);
        LCD_WRITE_STRING("Initialized", LCD_PAGE2);
      }
      break;

    case GAPROLE_ADVERTISING:
      LCD_WRITE_STRING("Advertising", LCD_PAGE2);
      

      break;

#ifdef PLUS_BROADCASTER   
    /* After a connection is dropped a device in PLUS_BROADCASTER will continue
     * sending non-connectable advertisements and shall sending this change of 
     * state to the application.  These are then disabled here so that sending 
     * connectable advertisements can resume.
     */
    case GAPROLE_ADVERTISING_NONCONN:
      {
        uint8_t advertEnabled = FALSE;
      
        // Disable non-connectable advertising.
        GAPRole_SetParameter(GAPROLE_ADV_NONCONN_ENABLED, sizeof(uint8_t),
                           &advertEnabled);
      
        advertEnabled = TRUE;
      
        // Enabled connectable advertising.
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t),
                             &advertEnabled);
        
        // Reset flag for next connection.
        firstConnFlag = false;
        
        SimpleBLEPeripheral_freeAttRsp(bleNotConnected);
      }
      break;
#endif //PLUS_BROADCASTER   

    case GAPROLE_CONNECTED:
      {
        uint8_t peerAddress[B_ADDR_LEN];

        GAPRole_GetParameter(GAPROLE_CONN_BD_ADDR, peerAddress);

        Util_startClock(&periodicClock);
        
        
        LCD_WRITE_STRING("Connected", LCD_PAGE2);
        LCD_WRITE_STRING(Util_convertBdAddr2Str(peerAddress), LCD_PAGE3);

        #ifdef PLUS_BROADCASTER
          // Only turn advertising on for this state when we first connect
          // otherwise, when we go from connected_advertising back to this state
          // we will be turning advertising back on.
          if (firstConnFlag == false)
          {
            uint8_t advertEnabled = FALSE; // Turn on Advertising

            // Disable connectable advertising.
            GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t),
                                 &advertEnabled);
            
            // Set to true for non-connectabel advertising.
            advertEnabled = TRUE;

            // Enable non-connectable advertising.
            GAPRole_SetParameter(GAPROLE_ADV_NONCONN_ENABLED, sizeof(uint8_t),
                                 &advertEnabled);
            firstConnFlag = true;
          }
        #endif // PLUS_BROADCASTER
      }
      break;

    case GAPROLE_CONNECTED_ADV:
      LCD_WRITE_STRING("Connected Advertising", LCD_PAGE2);
      break;

    case GAPROLE_WAITING:
      Util_stopClock(&periodicClock);
      SimpleBLEPeripheral_freeAttRsp(bleNotConnected);

      LCD_WRITE_STRING("Disconnected", LCD_PAGE2);

      // Clear remaining lines
      LCD_WRITE_STRING("", LCD_PAGE3);
      LCD_WRITE_STRING("", LCD_PAGE4);
      LCD_WRITE_STRING("", LCD_PAGE5);
      break;

    case GAPROLE_WAITING_AFTER_TIMEOUT:
      SimpleBLEPeripheral_freeAttRsp(bleNotConnected);
      
      LCD_WRITE_STRING("Timed Out", LCD_PAGE2);
      
      // Clear remaining lines
      LCD_WRITE_STRING("", LCD_PAGE3);
      LCD_WRITE_STRING("", LCD_PAGE4);
      LCD_WRITE_STRING("", LCD_PAGE5);

      #ifdef PLUS_BROADCASTER
        // Reset flag for next connection.
        firstConnFlag = false;
      #endif //#ifdef (PLUS_BROADCASTER)
      break;

    case GAPROLE_ERROR:
      LCD_WRITE_STRING("Error", LCD_PAGE2);
      break;

    default:
      LCD_WRITE_STRING("", LCD_PAGE2);
      break;
  }

  // Update the state
  //gapProfileState = newState;
}

#ifndef FEATURE_OAD
/*********************************************************************
 * @fn      SimpleBLEPeripheral_charValueChangeCB
 *
 * @brief   Callback from Simple Profile indicating a characteristic
 *          value change.
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_charValueChangeCB(uint8_t paramID)
{
  SimpleBLEPeripheral_enqueueMsg(SBP_CHAR_CHANGE_EVT, paramID);
}
#endif //!FEATURE_OAD

/*********************************************************************
 * @fn      SimpleBLEPeripheral_processCharValueChangeEvt
 *
 * @brief   Process a pending Simple Profile characteristic value change
 *          event.
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_processCharValueChangeEvt(uint8_t paramID)
{
#ifndef FEATURE_OAD
  
  switch(paramID)
  {
    case SIMPLEPROFILE_CHAR1:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR1, simpleBLEChan1 );
      osal_snv_write( BLE_NVID_CHAR1, SIMPLEPROFILE_CHAR1_LEN, simpleBLEChan1 );
      memcpy(echoData1, simpleBLEChan1, SIMPLEPROFILE_CHAR1_LEN);
      
      break;
      
    case SIMPLEPROFILE_CHAR2:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR2, simpleBLEChan2 );
      osal_snv_write( BLE_NVID_CHAR2, SIMPLEPROFILE_CHAR2_LEN, simpleBLEChan2 );
      memcpy(echoData2, simpleBLEChan2, SIMPLEPROFILE_CHAR2_LEN);

      break;

    case SIMPLEPROFILE_CHAR3:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR3, simpleBLEChan3 );
      osal_snv_write( BLE_NVID_CHAR3, SIMPLEPROFILE_CHAR3_LEN, simpleBLEChan3 );
      memcpy(echoData3, simpleBLEChan3, SIMPLEPROFILE_CHAR3_LEN);

      break;
      
   case SIMPLEPROFILE_CHAR4:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR4, simpleBLEChan4 );
      osal_snv_write( BLE_NVID_CHAR4, SIMPLEPROFILE_CHAR4_LEN, simpleBLEChan4 );
      memcpy(echoData4, simpleBLEChan4, SIMPLEPROFILE_CHAR4_LEN);

      break;
      
    case SIMPLEPROFILE_CHAR5:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR5, simpleBLEChan5 );
      osal_snv_write( BLE_NVID_CHAR5, SIMPLEPROFILE_CHAR5_LEN, simpleBLEChan5 );
      memcpy(echoData5, simpleBLEChan5, SIMPLEPROFILE_CHAR5_LEN);

      break;

    case SIMPLEPROFILE_CHAR6:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR6, simpleBLEChan6 );
      osal_snv_write( BLE_NVID_CHAR6, SIMPLEPROFILE_CHAR6_LEN, simpleBLEChan6 );
      memcpy(echoData6, simpleBLEChan6, SIMPLEPROFILE_CHAR6_LEN);

      break;
      
    case SIMPLEPROFILE_CHAR7:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR7, simpleBLEChan7 );
      osal_snv_write( BLE_NVID_CHAR7, SIMPLEPROFILE_CHAR7_LEN, simpleBLEChan7 );
      memcpy(echoData7, simpleBLEChan7, SIMPLEPROFILE_CHAR7_LEN);

      break;
      
    case SIMPLEPROFILE_CHAR8:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR8, attDeviceName );
      osal_snv_write( BLE_NVID_CHAR8, SIMPLEPROFILE_CHAR8_LEN, attDeviceName );
      GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );
      break;
      
    case SIMPLEPROFILE_CHAR9:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR9, simpleBLEChan9 );
      osal_snv_write( BLE_NVID_CHAR9, SIMPLEPROFILE_CHAR9_LEN, simpleBLEChan9 );
      memcpy(echoData9, simpleBLEChan9, SIMPLEPROFILE_CHAR9_LEN);

      break;
      
    case SIMPLEPROFILE_CHAR10:
      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR10, simpleBLEChan10 );
      osal_snv_write( BLE_NVID_CHAR10, SIMPLEPROFILE_CHAR10_LEN, simpleBLEChan10 );
      memcpy(echoData10, simpleBLEChan10, SIMPLEPROFILE_CHAR10_LEN);

      break;
      
      
    default:
      // should not reach here!
      break;
  }
#endif //!FEATURE_OAD
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_performPeriodicTask
 *
 * @brief   Perform a periodic application task. This function gets called
 *          every five seconds (SBP_PERIODIC_EVT_PERIOD). In this example,
 *          the value of the third characteristic in the SimpleGATTProfile
 *          service is retrieved from the profile, and then copied into the
 *          value of the the fourth characteristic.
 *
 * @param   None.
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_performPeriodicTask(void)
{

      uint32_t batval=65535;
      uint8 data[3]={0};
      
      //BAT Monitor
      AONBatMonEnable();
      // <int.frac> format size <3.8> in units of volt
      //·µ»ØÖµ32Î»ÖÐ[10:8]´ú±íINT ¡£[7:0]´ú±íFRAC ,¶ÔÓÚÐ¡Êý²¿·Ö£¬Ò»¸öµ¥Î»´ú±í0.00390625v
      batval = AONBatMonBatteryVoltageGet();
      //UART_write(SbpUartHandle, &batval, 4);
      data[0] = (batval >> 16) & 0xFF;
      data[1] = (batval >> 8) & 0xFF;
      data[2] = batval & 0xFF;
      SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR10, SIMPLEPROFILE_CHAR10_LEN,
                             &data[1]);
      

  
  
}


#if defined(FEATURE_OAD)
/*********************************************************************
 * @fn      SimpleBLEPeripheral_processOadWriteCB
 *
 * @brief   Process a write request to the OAD profile.
 *
 * @param   event      - event type:
 *                       OAD_WRITE_IDENTIFY_REQ
 *                       OAD_WRITE_BLOCK_REQ
 * @param   connHandle - the connection Handle this request is from.
 * @param   pData      - pointer to data for processing and/or storing.
 *
 * @return  None.
 */
void SimpleBLEPeripheral_processOadWriteCB(uint8_t event, uint16_t connHandle,
                                           uint8_t *pData)
{
  oadTargetWrite_t *oadWriteEvt = ICall_malloc( sizeof(oadTargetWrite_t) + \
                                             sizeof(uint8_t) * OAD_PACKET_SIZE);
  
  if ( oadWriteEvt != NULL )
  {
    oadWriteEvt->event = event;
    oadWriteEvt->connHandle = connHandle;
    
    oadWriteEvt->pData = (uint8_t *)(&oadWriteEvt->pData + 1);
    memcpy(oadWriteEvt->pData, pData, OAD_PACKET_SIZE);

    Queue_enqueue(hOadQ, (Queue_Elem *)oadWriteEvt);
    
    // Post the application's semaphore.
    Semaphore_post(sem);
  }
  else
  {
    // Fail silently.
  }
}
#endif //FEATURE_OAD

/*********************************************************************
 * @fn      SimpleBLEPeripheral_clockHandler
 *
 * @brief   Handler function for clock timeouts.
 *
 * @param   arg - event type
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_clockHandler(UArg arg)
{
  // Store the event.
  events |= arg;

  // Wake up the application.
  Semaphore_post(sem);
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_enqueueMsg
 *
 * @brief   Creates a message and puts the message in RTOS queue.
 *
 * @param   event - message event.
 * @param   state - message state.
 *
 * @return  None.
 */
static void SimpleBLEPeripheral_enqueueMsg(uint8_t event, uint8_t state)
{
  sbpEvt_t *pMsg;

  // Create dynamic pointer to message.
  if ((pMsg = ICall_malloc(sizeof(sbpEvt_t))))
  {
    pMsg->hdr.event = event;
    pMsg->hdr.state = state;

    // Enqueue the message.
    Util_enqueueMsg(appMsgQueue, sem, (uint8*)pMsg);
  }
}

/*********************************************************************
            led_blink-SINGLE
*********************************************************************/
void Smart_Locker_blinkLed(uint8_t led, uint8_t nBlinks)
{
  uint8_t i;
  
  for (i=0; i<nBlinks; i++)
  {
    PIN_setOutputValue(hGpioPin, led, Board_LED_ON);
    delay_ms(200);
    PIN_setOutputValue(hGpioPin, led, Board_LED_OFF);
    delay_ms(200);
  }
}

/*********************************************************************
            led_blink_FOUR
*********************************************************************/
void Smart_Locker_blinkLed4(uint8_t nBlinks)
{
  uint8_t i;
  
  for (i=0; i<nBlinks; i++)
  {
    PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_ON);
    PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_ON);
    PIN_setOutputValue(hGpioPin, Board_LED3, Board_LED_ON);
    PIN_setOutputValue(hGpioPin, Board_LED4, Board_LED_ON);
    delay_ms(200);
    PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_LED3, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_LED4, Board_LED_OFF);
    delay_ms(200);
  }
}

/*********************************************************************
 * @fn      SimpleBLECentral_handleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void SimpleBLECentral_handleKeys(uint8_t shift, uint8_t keys)
{
  (void)shift;  // Intentionally unreferenced parameter


  if (keys & KEY_LEFT)
  {

    PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_ON);
    PIN_setOutputValue(hGpioPin, Board_IA, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_IB, Board_LED_OFF);
    return;
  }


  if (keys & KEY_RIGHT)
  {
    PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_IA, Board_LED_ON);
    PIN_setOutputValue(hGpioPin, Board_IB, Board_LED_ON);
    return;
  }

  if (keys & KEY_SELECT)
  {
     

    PIN_setOutputValue(hGpioPin, Board_Alarm, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_IA, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_IB, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_LED3, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_LED4, Board_LED_OFF);
    Smart_Locker_blinkLed4(10);
    return;
  }
  
    if (keys & KEY_DOWN)//button2
  {
    
    
    PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_OFF); 
    PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_Doorbell, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_Alarm, Board_LED_ON);
    PIN_setOutputValue(hGpioPin, Board_IA, Board_LED_OFF);
    PIN_setOutputValue(hGpioPin, Board_IB, Board_LED_ON);
    return;
  }
  
    if (keys & KEY_UP)//button1
  {
    

    PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_ON);
    PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_ON); 
    PIN_setOutputValue(hGpioPin, Board_Doorbell, Board_LED_ON); 
    PIN_setOutputValue(hGpioPin, Board_Alarm, Board_LED_OFF); 
    PIN_setOutputValue(hGpioPin, Board_IA, Board_LED_ON);
    PIN_setOutputValue(hGpioPin, Board_IB, Board_LED_OFF);
    return;
  }
}

/*********************************************************************
 * @fn      SimpleBLECentral_keyChangeHandler
 *
 * @brief   Key event handler function
 *
 * @param   a0 - ignored
 *
 * @return  none
 */
void SimpleBLECentral_keyChangeHandler(uint8 keys)
{
  SimpleBLEPeripheral_enqueueMsg(SBC_KEY_CHANGE_EVT, keys);
}

////bonding 2015.12.09
////******************************************************************************    
////name:             ProcessPasscodeCB    
////introduce:        °ó¶¨¹ý³ÌÖÐµÄÃÜÂë¹ÜÀí»Øµ÷º¯Êý  
////parameter:        deviceAddr:Éè±¸µØÖ·  
////                      connectionHandle:Á¬½Ó¾ä±ú  
////                      uiInputs:  
////                      uiOutputs:  
////return:           none  
////author:       dongxianglin
////changetime:       2015.12.8  
////******************************************************************************  
//static void ProcessPasscodeCB(uint8 *deviceAddr,uint16 connectionHandle,uint8 uiInputs,uint8 uiOutputs )  
//{  
//  uint32  passcode;  
//  uint8   str[7];  
//  
//  //ÉèÖÃÃÜÂë  
//  #if 0  
//    LL_Rand( ((uint8 *) &passcode), sizeof( uint32 ));  
//    passcode %= 1000000;  
//  #else  
//    passcode = 456890;          
//  #endif  
//
//  //·¢ËÍÃÜÂëÏìÓ¦¸øÖ÷»ú  
//  GAPBondMgr_PasscodeRsp( connectionHandle, SUCCESS, passcode );  
//}  

//******************************************************************************    
//name:             ProcessPairStateCB  
//introduce:        °ó¶¨¹ý³ÌÖÐµÄ×´Ì¬¹ÜÀíº¯Êý  
//parameter:        connectionHandle:Á¬½Ó¾ä±ú                
//                      state:  
//                      status:  
//return:           none  
//author:       dongxianglin 
//changetime:       2015.12.8  
//******************************************************************************  
static void ProcessPairStateCB( uint16 connHandle, uint8 state, uint8 status )  
{  
  //Ö÷»ú·¢ÆðÁ¬½Ó£¬»á½øÈë¿ªÊ¼Åä¶Ô×´Ì¬  
  if ( state == GAPBOND_PAIRING_STATE_STARTED )  
  {  

    gPairStatus = PAIRSTATUS_NO_PAIRED;  
  }  
    
  //µ±Ö÷»úÌá½»ÃÜÂëºó£¬»á½øÈëÅä¶ÔÍê³É×´Ì¬    
  else if ( state == GAPBOND_PAIRING_STATE_COMPLETE )  
  {  
    //Åä¶Ô³É¹¦  
    if ( status == SUCCESS )      
    {  
      gPairStatus = PAIRSTATUS_PAIRED;  
    }  
    //ÒÑÅä¶Ô¹ý  
    else if(status == SMP_PAIRING_FAILED_UNSPECIFIED)  
    {       
      gPairStatus = PAIRSTATUS_PAIRED;  
    }  
      
    //Åä¶ÔÊ§°Ü  
    else  
    {  
      gPairStatus = PAIRSTATUS_NO_PAIRED;  
    }  
    //Åä¶ÔÊ§°ÜÔò¶Ï¿ªÁ¬½Ó  
    if(gPairStatus == PAIRSTATUS_NO_PAIRED)  
    {  
      GAPRole_TerminateConnection();  
      //HAL_SYSTEM_RESET();
    }  
  }  
  else if ( state == GAPBOND_PAIRING_STATE_BONDED )  
  {  
    if ( status == SUCCESS )  
    {  

    }  
  }  
}  
