
/*********************************************************************
 * INCLUDES
 */
#include <stdio.h>
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
//#include "simpleGATTprofile.h"

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

#include <driverlib/ioc.h>
#include <driverlib/gpio.h>
#include <driverlib/timer.h>


#include "Board.h"
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/I2C.h>

#include "board_mpu6050.h"
#include "board_LCD.h"
#include <ti/drivers/lcd/LCDDogm1286.h>



static uint8_t rxBuffer[32];            // Receive buffer
static uint8_t txBuffer[32];            // Transmit buffer
//static bool transferDone = false;

static I2C_Handle handle = NULL;
static I2C_Params params;
static I2C_Transaction i2cTrans;

static uint8_t vbuffer[24];

static bool HalSensorRWReg(uint8 addr, uint8 *pWriteBuf, uint8 WriteBytes,uint8 *pReadBuf, uint8 ReadBytes)
{
 memset(txBuffer, 0, sizeof(txBuffer));
 memset(rxBuffer, 0, sizeof(rxBuffer));

 txBuffer[0] = addr; 
 memcpy(txBuffer+1, pWriteBuf, WriteBytes);

 // Initialize master I2C transaction structure
 i2cTrans.writeCount   = 1+WriteBytes;
 i2cTrans.writeBuf     = txBuffer;
 i2cTrans.readCount    = ReadBytes;
 i2cTrans.readBuf      = rxBuffer;
 i2cTrans.slaveAddress = MPU6050_ADDRESS_AD0_LOW;

 //transferDone = false;
 
 // Do I2C transfer (in callback mode)
 return I2C_transfer(handle, &i2cTrans);
}

static void MPU6050_Config_Register(void) {
    uint8 vCmd = 0x01;
    HalSensorRWReg(MPU6050_RA_PWR_MGMT_1, &vCmd, 1, NULL, 0); 
    vCmd = 0;
    HalSensorRWReg(MPU6050_RA_GYRO_CONFIG, &vCmd, 1, NULL, 0);
    HalSensorRWReg(MPU6050_RA_ACCEL_CONFIG,&vCmd, 1, NULL, 0); 
    HalSensorRWReg(MPU6050_RA_PWR_MGMT_1,&vCmd, 1, NULL, 0); 
}

void getMotion6(int16_t* ax, int16_t* ay, int16_t* az, int16_t* aTemp, int16_t* gx, int16_t* gy, int16_t* gz)
{
    HalSensorRWReg(MPU6050_RA_ACCEL_XOUT_H, NULL, 0, vbuffer, 14);  
    memcpy(vbuffer, rxBuffer, 14);
    *ax = (((int16_t)vbuffer[0]) << 8) | vbuffer[1];
    *ay = (((int16_t)vbuffer[2]) << 8) | vbuffer[3];
    *az = (((int16_t)vbuffer[4]) << 8) | vbuffer[5];
    *aTemp = (((int16_t)vbuffer[6]) << 8) | vbuffer[7];
    *gx = (((int16_t)vbuffer[8]) << 8) | vbuffer[9];
    *gy = (((int16_t)vbuffer[10]) << 8) | vbuffer[11];
    *gz = (((int16_t)vbuffer[12]) << 8) | vbuffer[13];
}

static void mpu6050_transferCallback(I2C_Handle handle, I2C_Transaction *transac, bool result)
{
 // Set length bytes
 if (result) {
     //transferDone = true;
     //LCD_WRITE_STRING_VALUE("rx[0]:", rxBuffer[0], 16, LCD_PAGE0);
 } else {
     // Transaction failed, act accordingly...
     //transferDone = false;
 }
}

static uint8 mpu6050_Init()
{
    // Locals
    // I2C_Handle handle;
    // I2C_Params params;
    // I2C_Transaction i2cTrans;

    // Configure I2C parameters.
    I2C_Params_init(&params);
    params.bitRate= I2C_100kHz;
    params.transferMode = I2C_MODE_BLOCKING;//I2C_MODE_CALLBACK;
    params.transferCallbackFxn = mpu6050_transferCallback;  //不是使用  I2C_MODE_CALLBACK， 这个无用的

    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));

    // Open I2C
    handle = I2C_open(Board_I2C, &params);

    HalSensorRWReg(MPU6050_RA_WHO_AM_I, NULL, 0, rxBuffer, 1);

    if(rxBuffer[0] != MPU6050_ADDRESS_AD0_LOW)
    {
        return FALSE;
    }

    return TRUE;
}/* 
void mpu6050_check_data()
{
    uint8 vCmd = 0;
    int16 ax=0, ay=0, az=0, aTemp=0,gx=0, gy=0, gz=0, mx=0, my=0, mz=0;
    float f_temperature = 0.0;
    
    HalSensorRWReg(MPU6050_RA_WHO_AM_I, NULL, 0, rxBuffer, 1);

    if(rxBuffer[0] != MPU6050_ADDRESS_AD0_LOW)
    {
//        LCD_WRITE_STRING("", LCD_PAGE5);
//        LCD_WRITE_STRING("", LCD_PAGE6);
//        LCD_WRITE_STRING("[Not found MPU6050]", LCD_PAGE7);
    }
    else
    {    
        LCD_WRITE_STRING("[MPU6050 OK]", LCD_PAGE7);
    
    
        rxBuffer[0] = 0;
        
        MPU6050_Config_Register();

        getMotion6(&ax, &ay, &az, &aTemp, &gx, &gy, &gz);

        char str[64];

        sprintf(str, "[%5d,%5d,%5d ]", ax, ay, az);    
        LCD_WRITE_STRING(str, LCD_PAGE5);



        sprintf(str, "[%5d,%5d,%5d ]", gx, gy, gz);    
        LCD_WRITE_STRING(str, LCD_PAGE6);

        f_temperature = (float)aTemp/340.0 + 36.53;    

//        sprintf(str, "[%5d,%5d,%5d ] [%.2f] [%5d,%5d,%5d ]\r\n", ax, ay, az, f_temperature, gx, gy, gz);    
//        UART_WriteTransport (str, strlen(str));

        
        rxBuffer[0] = 0;
        memset(rxBuffer, 0, sizeof(rxBuffer));
    }
 
}
*/
bool mpu6050_read16(int16_t *ax, int16_t *ay, int16_t *az, int16_t *aTemp, int16_t *gx, int16_t *gy, int16_t *gz)
{
    if(NULL == handle)
    {
        mpu6050_Init();
        Task_sleep(10*1000/Clock_tickPeriod);
    }
    
    HalSensorRWReg(MPU6050_RA_WHO_AM_I, NULL, 0, rxBuffer, 1);

    if(rxBuffer[0] != MPU6050_ADDRESS_AD0_LOW)
    {
        return FALSE;
    }
    else
    {    
        rxBuffer[0] = 0;
        Task_sleep(10*1000/Clock_tickPeriod);
        MPU6050_Config_Register();
        Task_sleep(10*1000/Clock_tickPeriod);
        getMotion6(ax, ay, az, aTemp, gx, gy, gz);
        
        rxBuffer[0] = 0;
        memset(rxBuffer, 0, sizeof(rxBuffer));
    }

    return TRUE;
}

