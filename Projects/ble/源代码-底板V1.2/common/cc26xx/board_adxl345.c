
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

#include "board_adxl345.h"
#include "board_LCD.h"
#include <ti/drivers/lcd/LCDDogm1286.h>

// ���߷���
// p0.5 = sda, p0.6 = scl,�����ٽ� vcc = 3.3v�� gnd���ɡ�

#ifdef SENSOR_ADXL345

static uint8_t rxBuffer[12];            // Receive buffer
static uint8_t txBuffer[12];            // Transmit buffer
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
 i2cTrans.slaveAddress = ADXL345_SDO_LOW;

 //transferDone = false;
 
 // Do I2C transfer (in callback mode)
 return I2C_transfer(handle, &i2cTrans);
}

static void Single_Write_ADXL345(uint8 addr, uint8 dat)
{
    HalSensorRWReg(addr, &dat, 1, NULL, 0); 
}

static void adxl345_Config_Register(void) 
{
   //Single_Write_ADXL345(0x31,0x0B);   //������Χ,����16g��13λģʽ   p25
   Single_Write_ADXL345(0x31,0x08);   //������Χ,����2g��13λģʽ   p25
   
   Single_Write_ADXL345(0x31,0x0B);   //������Χ,����16g��13λģʽ
   Single_Write_ADXL345(0x2C,0x08);   //�����趨Ϊ12.5 �ο�pdf13ҳ
   Single_Write_ADXL345(0x2D,0x08);   //ѡ���Դģʽ   �ο�pdf24ҳ
   Single_Write_ADXL345(0x2E,0x80);   //ʹ�� DATA_READY �ж�
   Single_Write_ADXL345(0x1E,0x00);   //X ƫ���� ���ݲ��Դ�������״̬д��pdf29ҳ
   Single_Write_ADXL345(0x1F,0x00);   //Y ƫ���� ���ݲ��Դ�������״̬д��pdf29ҳ
   Single_Write_ADXL345(0x20,0x05);   //Z ƫ���� ���ݲ��Դ�������״̬д��pdf29ҳ
}

//*********************************************************
//
//��������ADXL345�ڲ����ٶ����ݣ���ַ��Χ0x32~0x37
//
//*********************************************************
//void Multiple_read_ADXL345(void)
//{   
//    uchar i;
//    ADXL345_Start();                          //��ʼ�ź�
//    ADXL345_SendByte(SlaveAddress);           //�����豸��ַ+д�ź�
//    ADXL345_SendByte(0x32);                   //���ʹ洢��Ԫ��ַ����0x32��ʼ	
//    ADXL345_Start();                          //��ʼ�ź�
//    ADXL345_SendByte(SlaveAddress+1);         //�����豸��ַ+���ź�
//	for (i=0; i<6; i++)                      //������ȡ6����ַ���ݣ��洢��BUF
//    {
//        BUF[i] = ADXL345_RecvByte();          //BUF[0]�洢0x32��ַ�е�����
//        if (i == 5)
//        {
//           ADXL345_SendACK(1);                //���һ��������Ҫ��NOACK
//        }
//        else
//        {
//          ADXL345_SendACK(0);                //��ӦACK
//       }
//   }
//    ADXL345_Stop();                          //ֹͣ�ź�
//    Delay5ms();
//}



static void getMotion6(int16_t* ax, int16_t* ay, int16_t* az)
{
    HalSensorRWReg(ADXL345_DAT_ADDR, NULL, 0, vbuffer, 6);  
    memcpy(vbuffer, rxBuffer, 6);
    *ax = (((int16_t)vbuffer[1]) << 8) | vbuffer[0];
    *ay = (((int16_t)vbuffer[3]) << 8) | vbuffer[2];
    *az = (((int16_t)vbuffer[5]) << 8) | vbuffer[4];
}

static void adxl345_transferCallback(I2C_Handle handle, I2C_Transaction *transac, bool result)
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

uint8 adxl345_Init()
{
    // Locals
    // I2C_Handle handle;
    // I2C_Params params;
    // I2C_Transaction i2cTrans;

    // Configure I2C parameters.
    I2C_Params_init(&params);
    params.bitRate= I2C_100kHz;
    params.transferMode = I2C_MODE_BLOCKING;//I2C_MODE_CALLBACK;
    params.transferCallbackFxn = adxl345_transferCallback;  //����ʹ��  I2C_MODE_CALLBACK�� ������õ�

    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));

    // Open I2C
    handle = I2C_open(Board_I2C, &params);

#if 0// test    
    while(1)
    {
        int16_t ax, ay, az;
        bool ret = false;
        ret = HalSensorRWReg(ADXL345_DEVID_ADDR, NULL, 0, rxBuffer, 1);  
        if(rxBuffer[0] != ADXL345_DEVID)// ��֪Ϊ�β���
        {
            adxl345_Config_Register();
        }        
        
        adxl345_Config_Register();        
        adxl345_read16(&ax, &ay, &az);   
    }
    return TRUE;
#else
    bool ret = false;
    ret = HalSensorRWReg(ADXL345_DEVID_ADDR, NULL, 0, rxBuffer, 1);  
    return ret;
#endif          

}

bool adxl345_read16(int16_t *ax, int16_t *ay, int16_t *az)
{
    bool ret = false;

    if(NULL == handle)
    {
        adxl345_Init();
        Task_sleep(10*1000/Clock_tickPeriod);
    }
    
    ret = HalSensorRWReg(ADXL345_DEVID_ADDR, NULL, 0, rxBuffer, 1);

    if(!ret)
    {
        return FALSE;
    }
    else
    {    
        rxBuffer[0] = 0;
        Task_sleep(10*1000/Clock_tickPeriod);
        adxl345_Config_Register();
        Task_sleep(10*1000/Clock_tickPeriod);
        getMotion6(ax, ay, az);
        
        rxBuffer[0] = 0;
        memset(rxBuffer, 0, sizeof(rxBuffer));
    }

    return TRUE;
}


#endif
