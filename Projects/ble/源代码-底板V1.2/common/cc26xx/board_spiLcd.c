
/*********************************************************************
INCLUDES
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

#include <driverlib/ioc.h>
#include <driverlib/gpio.h>
#include <driverlib/timer.h>


#include "Board.h"
#include <ti/drivers/spi/SPICC26XXDMA.h>
#include <ti/drivers/I2C.h>


#include "board_mpu6050.h"
#include "board_LCD.h"
#include <ti/drivers/lcd/LCDDogm1286.h>




void spiLcd_Init()
{
    SPI_Handle handle;
    SPI_Params params;
    SPI_Transaction transaction;
    //PIN_Id csnPin1  = PIN_ID(Board_CSN_1);
    uint8_t txBuf[] = "Hello World";    // Transmit buffer

    // Init SPI and specify non-default parameters
    SPI_Params_init(&params);
    params.bitRate     = 1000000;
    params.frameFormat = SPI_POL1_PHA1;
    params.mode        = SPI_MASTER;
    
    // Configure the transaction
    transaction.count = sizeof(txBuf);
    transaction.txBuf = txBuf;
    transaction.rxBuf = NULL;

    // Open the SPI and perform transfer to the first slave
    handle = SPI_open(Board_SPI0, &params);
    
    SPI_transfer(handle, &transaction);
    
    // Then switch chip select pin and perform transfer to the second slave
    //SPI_control(handle, SPICC26XXDMA_SET_CSN_PIN, &csnPin1);
    //SPI_transfer(handle, &transaction);

    

}





