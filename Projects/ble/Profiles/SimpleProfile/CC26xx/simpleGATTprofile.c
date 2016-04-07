/**************************************************************************************************
  Filename:       simpleGATTprofile.c
  Revised:        $Date: 2015-07-20 11:31:07 -0700 (Mon, 20 Jul 2015) $
  Revision:       $Revision: 44370 $

  Description:    This file contains the Simple GATT profile sample GATT service 
                  profile for use with the BLE sample application.

  Copyright 2010 - 2015 Texas Instruments Incorporated. All rights reserved.

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

#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"

#include "simpleGATTprofile.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED        32

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Simple GATT Profile Service UUID: 0xFFF0
CONST uint8 simpleProfileServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_SERV_UUID), HI_UINT16(SIMPLEPROFILE_SERV_UUID)
};

// Characteristic 1 UUID: 0xFFF1
CONST uint8 simpleProfilechar1UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR1_UUID), HI_UINT16(SIMPLEPROFILE_CHAR1_UUID)
};

// Characteristic 2 UUID: 0xFFF2
CONST uint8 simpleProfilechar2UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR2_UUID), HI_UINT16(SIMPLEPROFILE_CHAR2_UUID)
};

// Characteristic 3 UUID: 0xFFF3
CONST uint8 simpleProfilechar3UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR3_UUID), HI_UINT16(SIMPLEPROFILE_CHAR3_UUID)
};

// Characteristic 4 UUID: 0xFFF4
CONST uint8 simpleProfilechar4UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR4_UUID), HI_UINT16(SIMPLEPROFILE_CHAR4_UUID)
};

// Characteristic 5 UUID: 0xFFF5
CONST uint8 simpleProfilechar5UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR5_UUID), HI_UINT16(SIMPLEPROFILE_CHAR5_UUID)
};

// Characteristic 6 UUID: 0xFFF6
CONST uint8 simpleProfilechar6UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR6_UUID), HI_UINT16(SIMPLEPROFILE_CHAR6_UUID)
};

// Characteristic 7 UUID: 0xFFF7
CONST uint8 simpleProfilechar7UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR7_UUID), HI_UINT16(SIMPLEPROFILE_CHAR7_UUID)
};

// Characteristic 8 UUID: 0xFFF8
CONST uint8 simpleProfilechar8UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR8_UUID), HI_UINT16(SIMPLEPROFILE_CHAR8_UUID)
};

// Characteristic 9 UUID: 0xFFF9
CONST uint8 simpleProfilechar9UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR9_UUID), HI_UINT16(SIMPLEPROFILE_CHAR9_UUID)
};

// Characteristic 10 UUID: 0xFFFA
CONST uint8 simpleProfilechar10UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR10_UUID), HI_UINT16(SIMPLEPROFILE_CHAR10_UUID)
};
/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
/*
  ÖÇÄÜËø¹ÜÀíÔ±ÉèÖÃ(ÓÉTypeÎ»¶ÎÇø·Ö)
  ²½Öè£º
      £¨1£©ÓÉÊÖ»úAPP·¢ËÍ°üº¬£ºAPPÖ§³ÖµÄ¹Ì¼þµÄ°æ±¾ºÅ¡¢Ê±¼ä¡¢×ÔÉíUUID£¨hash£©ÇëÇó¡£
      £¨2£©ÖÇÄÜËøÐ£×¼Ê±¼ä£¬ºË¶Ô°æ±¾ºÅ£¬¼ì²é¿ªËø°ó¶¨°´Å¥£¬Éú³ÉCK£¬µç×ÓËøÐ¾£¬
           ·¢ËÍ°üº¬£º³ö³§ÐòÁÐºÅ£¨ºÍUUIDÇø·Ö¿ªÀ´£©¡¢CK¡¢µç×ÓËøÐ¾£¬¹Ì¼þ°æ±¾ºÅµÄÏìÓ¦ÇëÇó¡£


      PS:Èç¹ûÊÇ¹ÜÀíÔ±¿ªËø£¬ÔòCK¼ÓÃÜ£ºµç×ÓÃØÔ¿¡¢¹ÜÀíÔ±ÊÖ»úID¡¢DATE£¬Èç¹ûÊÇ·ÖÏíÕß¿ªËø£¬
         ÔòÖ±½ÓÄÃ·ÖÏíµ½µÄ¼ÓÃÜ´®¿ªËø¡£
*/
/*********************************************************************
 *  Developer     : dongxianglin
 *  Release_Date  : 2016.04.07
 *
 *  APP->Locker DATA
              Name             Length    datavalue

              Type         :  1  byte    0x01
              Length       :  2  byte
              Segment_id   :  1  byte
              Smartphone_id:  16 byte    MD5(00000000-54b3-e7c7-0000-000046bffd97) 128bit
              Data         :  2  byte    128138277==2010Äê 08ÔÂ 10ÈÕ ÐÇÆÚ¶þ 03:39:21 CST
              Verson_1     :  8  byte    4byte´ó°æ±¾ºÅ+2byte×Ó°æ±¾ºÅ+2byteÊ±¼ä
              Verson_2     :  8  byte    4byte´ó°æ±¾ºÅ+2byte×Ó°æ±¾ºÅ+2byteÊ±¼ä
              Verson_3     :  8  byte    4byte´ó°æ±¾ºÅ+2byte×Ó°æ±¾ºÅ+2byteÊ±¼ä
 */
static uint8 App_Locker1_Config[20] = {0x00};
static uint8 App_Locker2_Config[20] = {0x00};
static uint8 App_Locker3_Config[20] = {0x00};
static uint8 App_Locker4_Config[20] = {0x00};
static uint8 App_Locker5_Config[20] = {0x00};
static uint8 App_Locker6_Config[20] = {0x00};
static uint8 App_Locker7_Config[20] = {0x00};
static uint8 App_Locker8_Config[20] = {0x00};
static uint8 App_Locker9_Config[20] = {0x00};
static uint8 App_Locker10_Config[20] = {0x00};


/*********************************************************************
 *  Developer     : dongxianglin
 *  Release_Date  : 2016.04.07
 *
 * Locker->APP DATA (SUCCESS)
              Name             Length    DataValue

              Type         :  1  byte    0x01
              Length       :  2  byte
              Segment_id   :  1  byte
              Factory_id   :  16 byte    MD5(00000000-54b3-e7c7-0000-000046bffd97) 128bit
              Ele_cylinder :  16 byte    MD5(00000000-54b3-e7c7-0000-000046bffd97) 128bit
              CK1          :  16  byte   AES128ÃØÔ¿£¨ÓÃÓÚÁ¬½ÓÖÇÄÜËø¼øÈ¨£©
              CK2          :  16  byte   AES128ÃØÔ¿£¨ÓÃÓÚÍ¨ÐÅ¼ÓÃÜ£©
              Verson_2     :  8  byte    4byte´ó°æ±¾ºÅ+2byte×Ó°æ±¾ºÅ+2byteÊ±¼ä

 */
static uint8 S_Locker_App1_Config[20] = {0x00};
static uint8 S_Locker_App2_Config[20] = {0x00};
static uint8 S_Locker_App3_Config[20] = {0x00};
static uint8 S_Locker_App4_Config[20] = {0x00};
static uint8 S_Locker_App5_Config[20] = {0x00};
static uint8 S_Locker_App6_Config[20] = {0x00};
static uint8 S_Locker_App7_Config[20] = {0x00};
static uint8 S_Locker_App8_Config[20] = {0x00};
static uint8 S_Locker_App9_Config[20] = {0x00};
static uint8 S_Locker_App10_Config[20] = {0x00};


/*********************************************************************
 *  Developer     : dongxianglin
 *  Release_Date  : 2016.04.07
 *
 * Locker->APP DATA (FAILED)
              Name             Length    DataValue

              Type         :  1  byte    0x01
              Length       :  2  byte
              Segment_id   :  1  byte
              Smartphone_id:  16 byte    MD5(00000000-54b3-e7c7-0000-000046bffd97) 128bit
              Error_Code   :  4  byte    ´íÎóÂë£¨¿ª·¢Ê±¹éÄÉ´íÎóÔ­Òò£¬²»¶ÏÀ©³ä£©
              Verson_2     :  8  byte    4byte´ó°æ±¾ºÅ+2byte×Ó°æ±¾ºÅ+2byteÊ±¼ä
 */
static uint8 F_Locker_App1_Config[20] = {0x00};
static uint8 F_Locker_App2_Config[20] = {0x00};
static uint8 F_Locker_App3_Config[20] = {0x00};
static uint8 F_Locker_App4_Config[20] = {0x00};
static uint8 F_Locker_App5_Config[20] = {0x00};
static uint8 F_Locker_App6_Config[20] = {0x00};
static uint8 F_Locker_App7_Config[20] = {0x00};
static uint8 F_Locker_App8_Config[20] = {0x00};
static uint8 F_Locker_App9_Config[20] = {0x00};
static uint8 F_Locker_App10_Config[20] = {0x00};


/*
  *¹ÜÀíÔ±¿ªËø(ÓÉTypeÎ»¶ÎÇø·Ö)
     Ç°Ìá£º
        £¨1£©ÖÇÄÜËø¹ã²¥×Ô¼ºµÄUUIDºÍÉè±¸ÐÅÏ¢+Ê±¼ä´Á£¬Í¨¹ýCK1¼ÓÃÜ¹ã²¥£»
        £¨2£©ÊÖ»úAPP¸ù¾Ý¹ã²¥£¬Í¨¹ýCK1½âÎöUUID£¬ÅÐ¶ÏÊÇ·ñÁ¬½Ó£»
        £¨3£©ÊÖ»úAPPÁ¬½ÓÖÇÄÜËø£¬ÖÇÄÜËøÍ£Ö¹¹ã²¥£»
    ²½Öè£º
        £¨1£©ÊÖ»úAPP·¢ËÍ·ÖÏíÕßÔ¿³×/¹ÜÀíÕßÔ¿³×¿ªËø
        £¨2£©ÖÇÄÜËø¼ì²âµ½ÕýÈ·¿ªËøÐÅÏ¢£¬¿ªËø£¬²¢»Ø¸´ok
*/
/*********************************************************************
 *  Developer     : dongxianglin
 *  Release_Date  : 2016.04.07
 *
 *  APP->Locker DATA
              Name             Length    datavalue

              Type           :  1  byte    0x02
              Length         :  2  byte
              Segment_id     :  1  byte
              Electronic_key :  16 byte    ÒÔÏÂ°üÀ¨´ËÏî¾ù¾­¹ýCK¼ÓÃÜ
              Smartphone_id  :  16 byte    MD5(00000000-54b3-e7c7-0000-000046bffd97) 128bit
              Data           :  2  byte    128138277==2010Äê 08ÔÂ 10ÈÕ ÐÇÆÚ¶þ 03:39:21 CST
              Verson_2       :  8  byte    4byte´ó°æ±¾ºÅ+2byte×Ó°æ±¾ºÅ+2byteÊ±¼ä

 */
static uint8 App_Locker1_Open[20] = {0x00};
static uint8 App_Locker2_Open[20] = {0x00};
static uint8 App_Locker3_Open[20] = {0x00};
static uint8 App_Locker4_Open[20] = {0x00};
static uint8 App_Locker5_Open[20] = {0x00};
static uint8 App_Locker6_Open[20] = {0x00};
static uint8 App_Locker7_Open[20] = {0x00};
static uint8 App_Locker8_Open[20] = {0x00};
static uint8 App_Locker9_Open[20] = {0x00};
static uint8 App_Locker10_Open[20] = {0x00};


/*********************************************************************
 *  Developer     : dongxianglin
 *  Release_Date  : 2016.04.07
 *
 * Locker->APP DATA (SUCCESS)
              Name             Length    DataValue

              Type         :  1  byte    0x02
              Length       :  2  byte
              Segment_id   :  1  byte
              Verson_2     :  8  byte    4byte´ó°æ±¾ºÅ+2byte×Ó°æ±¾ºÅ+2byteÊ±¼ä

 */
static uint8 S_Locker_App1_Open[20] = {0x00};
static uint8 S_Locker_App2_Open[20] = {0x00};
static uint8 S_Locker_App3_Open[20] = {0x00};
static uint8 S_Locker_App4_Open[20] = {0x00};
static uint8 S_Locker_App5_Open[20] = {0x00};
static uint8 S_Locker_App6_Open[20] = {0x00};
static uint8 S_Locker_App7_Open[20] = {0x00};
static uint8 S_Locker_App8_Open[20] = {0x00};
static uint8 S_Locker_App9_Open[20] = {0x00};
static uint8 S_Locker_App10_Open[20] = {0x00};



/*********************************************************************
 * LOCAL VARIABLES
 */

static simpleProfileCBs_t *simpleProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Simple Profile Service attribute
static CONST gattAttrType_t simpleProfileService = { ATT_BT_UUID_SIZE, simpleProfileServUUID };


// Simple Profile Characteristic 1 Properties
static uint8 simpleProfileChar1Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 1 Value
//static uint8 simpleProfileChar1 = 0;
static uint8 simpleProfileChar1[SIMPLEPROFILE_CHAR1_LEN] = { 0, 0 };

// Simple Profile Characteristic 1 User Description
//static uint8 simpleProfileChar1UserDesp[17] = "Characteristic 1";
static uint8 simpleProfileChar1UserDesp[13] = "Echo_Test_1";


// Simple Profile Characteristic 2 Properties
static uint8 simpleProfileChar2Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 2 Value
static uint8 simpleProfileChar2[SIMPLEPROFILE_CHAR2_LEN] = { 0, 0 };

// Simple Profile Characteristic 2 User Description
static uint8 simpleProfileChar2UserDesp[13] = "Echo_Test_2";

// Simple Profile Characteristic 3 Properties
static uint8 simpleProfileChar3Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 3 Value
static uint8 simpleProfileChar3[SIMPLEPROFILE_CHAR3_LEN] = {0};

// Simple Profile Characteristic 3 User Description
static uint8 simpleProfileChar3UserDesp[13] = "Echo_Test_3";

// Simple Profile Characteristic 4 Properties
static uint8 simpleProfileChar4Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 4 Value
static uint8 simpleProfileChar4[SIMPLEPROFILE_CHAR4_LEN] = { 0 };
                                        
// Simple Profile Characteristic 4 User Description
static uint8 simpleProfileChar4UserDesp[13] = "Echo_Test_4";


// Simple Profile Characteristic 5 Properties
static uint8 simpleProfileChar5Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 5 Value
static uint8 simpleProfileChar5[SIMPLEPROFILE_CHAR5_LEN] = {0} ;

// Simple Profile Characteristic 5 User Description
//static uint8 simpleProfileChar5UserDesp[17] = "Characteristic 5";
static uint8 simpleProfileChar5UserDesp[13] = "Echo_Test_5";

// Simple Profile Characteristic 6 Properties
static uint8 simpleProfileChar6Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 6 Value
static uint8 simpleProfileChar6[SIMPLEPROFILE_CHAR6_LEN] = { 0 };

// Simple Profile Characteristic 6 User Description
static uint8 simpleProfileChar6UserDesp[13] = "Echo_Test_6";

// Simple Profile Characteristic 7 Properties
static uint8 simpleProfileChar7Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 7 Value
static uint8 simpleProfileChar7[SIMPLEPROFILE_CHAR7_LEN] = { 0 };

// Simple Profile Characteristic 7 User Description
static uint8 simpleProfileChar7UserDesp[13] = "Echo_Test_7";

// Simple Profile Characteristic 8 Properties
static uint8 simpleProfileChar8Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 8 Value
static uint8 simpleProfileChar8[SIMPLEPROFILE_CHAR8_LEN] = { 0 };

// Simple Profile Characteristic 8 User Description
static uint8 simpleProfileChar8UserDesp[12] = "DeviceName";

// Simple Profile Characteristic 9 Properties
static uint8 simpleProfileChar9Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 9 Value
static uint8 simpleProfileChar9[SIMPLEPROFILE_CHAR9_LEN] = { 0 };

// Simple Profile Characteristic 9 User Description
static uint8 simpleProfileChar9UserDesp[13] = "Echo_Test_9";

// Simple Profile Characteristic 10 Properties
static uint8 simpleProfileChar10Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 10 Value
static uint8 simpleProfileChar10[SIMPLEPROFILE_CHAR7_LEN] = { 0 };

// Simple Profile Characteristic 10 User Description
static uint8 simpleProfileChar10UserDesp[14] = "Battery_Value";

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t simpleProfileAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = 
{
  // Simple Profile Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&simpleProfileService            /* pValue */
  },

    // Characteristic 1 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar1Props 
    },

      // Characteristic Value 1
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar1UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        simpleProfileChar1 
      },

      // Characteristic 1 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar1UserDesp 
      },      

    // Characteristic 2 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar2Props 
    },

      // Characteristic Value 2
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar2UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,  
        0, 
        simpleProfileChar2 
      },

      // Characteristic 2 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar2UserDesp 
      },           
      
    // Characteristic 3 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar3Props 
    },

      // Characteristic Value 3
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar3UUID },
         GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0, 
        simpleProfileChar3 
      },

      // Characteristic 3 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar3UserDesp 
      },

    // Characteristic 4 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar4Props 
    },

      // Characteristic Value 4
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar4UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        simpleProfileChar4 
      },

      // Characteristic 4 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar4UserDesp 
      },
      
    // Characteristic 5 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar5Props 
    },

      // Characteristic Value 5
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar5UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        simpleProfileChar5 
      },

      // Characteristic 5 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar5UserDesp 
      },
      
      // Characteristic 6 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar6Props 
    },

      // Characteristic Value 6
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar6UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        simpleProfileChar6 
      },

      // Characteristic 6 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar6UserDesp 
      }, 
      
      // Characteristic 7 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar7Props 
    },

      // Characteristic Value 7
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar7UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        simpleProfileChar7 
      },

      // Characteristic 7 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar7UserDesp 
      },    
      
       // Characteristic 8 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar8Props 
    },

      // Characteristic Value 8
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar8UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        simpleProfileChar8 
      },

      // Characteristic 8 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar8UserDesp 
      }, 
      
          // Characteristic 9 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar9Props 
    },

      // Characteristic Value 9
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar9UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        simpleProfileChar9 
      },

      // Characteristic 9 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar9UserDesp 
      }, 
      
          // Characteristic 10 Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &simpleProfileChar10Props 
    },

      // Characteristic Value 10
      { 
        { ATT_BT_UUID_SIZE, simpleProfilechar10UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        simpleProfileChar10 
      },

      // Characteristic 10 User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        simpleProfileChar10UserDesp 
      }, 
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr, 
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method);
static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method);



/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile Service Callbacks
CONST gattServiceCBs_t simpleProfileCBs =
{
  simpleProfile_ReadAttrCB,  // Read callback function pointer
  simpleProfile_WriteAttrCB, // Write callback function pointer
  NULL                       // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleProfile_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t SimpleProfile_AddService( uint32 services )
{
  uint8 status;

//  // Allocate Client Characteristic Configuration table
//  simpleProfileChar4Config = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) *
//                                                            linkDBNumConns );
//  if ( simpleProfileChar4Config == NULL )
//  {     
//    return ( bleMemAllocError );
//  }
//  
//  // Initialize Client Characteristic Configuration attributes
//  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, simpleProfileChar4Config );
  
  if ( services & SIMPLEPROFILE_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( simpleProfileAttrTbl, 
                                          GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE,
                                          &simpleProfileCBs );
  }
  else
  {
    status = SUCCESS;
  }

  return ( status );
}

/*********************************************************************
 * @fn      SimpleProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call 
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t SimpleProfile_RegisterAppCBs( simpleProfileCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    simpleProfile_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

/*********************************************************************
 * @fn      SimpleProfile_SetParameter
 *
 * @brief   Set a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to write
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SIMPLEPROFILE_CHAR1:
     if ( len == SIMPLEPROFILE_CHAR1_LEN ) 
      {
        VOID memcpy( simpleProfileChar1, value, SIMPLEPROFILE_CHAR1_LEN );  
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SIMPLEPROFILE_CHAR2:
      if ( len == SIMPLEPROFILE_CHAR2_LEN ) 
      {
        VOID memcpy( simpleProfileChar2, value, SIMPLEPROFILE_CHAR2_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SIMPLEPROFILE_CHAR3:
      if ( len == SIMPLEPROFILE_CHAR3_LEN ) 
      {
        VOID memcpy( simpleProfileChar3, value, SIMPLEPROFILE_CHAR3_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SIMPLEPROFILE_CHAR4:
      if ( len == sizeof ( uint8 ) ) 
      {
        VOID memcpy( simpleProfileChar5, value, SIMPLEPROFILE_CHAR5_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SIMPLEPROFILE_CHAR5:
      if ( len == SIMPLEPROFILE_CHAR5_LEN ) 
      {
        VOID memcpy( simpleProfileChar5, value, SIMPLEPROFILE_CHAR5_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
      case SIMPLEPROFILE_CHAR6:
      if ( len == SIMPLEPROFILE_CHAR6_LEN ) 
      {
        VOID memcpy( simpleProfileChar6, value, SIMPLEPROFILE_CHAR6_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
      case SIMPLEPROFILE_CHAR7:
      if ( len == SIMPLEPROFILE_CHAR7_LEN ) 
      {
        VOID memcpy( simpleProfileChar7, value, SIMPLEPROFILE_CHAR7_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
      case SIMPLEPROFILE_CHAR8:
      if ( len == SIMPLEPROFILE_CHAR8_LEN ) 
      {
        VOID memcpy( simpleProfileChar8, value, SIMPLEPROFILE_CHAR8_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
     case SIMPLEPROFILE_CHAR9:
     if ( len == SIMPLEPROFILE_CHAR9_LEN ) 
      {
        VOID memcpy( simpleProfileChar9, value, SIMPLEPROFILE_CHAR9_LEN );  
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
     
     case SIMPLEPROFILE_CHAR10:
     if ( len == SIMPLEPROFILE_CHAR10_LEN ) 
      {
        VOID memcpy( simpleProfileChar10, value, SIMPLEPROFILE_CHAR10_LEN );  
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
     
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      SimpleProfile_GetParameter
 *
 * @brief   Get a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SIMPLEPROFILE_CHAR1:
      memcpy( value, simpleProfileChar1, SIMPLEPROFILE_CHAR1_LEN );
      break;

    case SIMPLEPROFILE_CHAR2:
       memcpy( value, simpleProfileChar2, SIMPLEPROFILE_CHAR2_LEN );
      break;      

    case SIMPLEPROFILE_CHAR3:
      memcpy( value, simpleProfileChar3, SIMPLEPROFILE_CHAR3_LEN );
      break;  

    case SIMPLEPROFILE_CHAR4:
      memcpy( value, simpleProfileChar4, SIMPLEPROFILE_CHAR4_LEN );
      break;

    case SIMPLEPROFILE_CHAR5:
      memcpy( value, simpleProfileChar5, SIMPLEPROFILE_CHAR5_LEN );
      break;   
      
    case SIMPLEPROFILE_CHAR6:
      memcpy( value, simpleProfileChar6, SIMPLEPROFILE_CHAR6_LEN );
      break; 
      
    case SIMPLEPROFILE_CHAR7:
      memcpy( value, simpleProfileChar7, SIMPLEPROFILE_CHAR7_LEN );
      break; 
      
    case SIMPLEPROFILE_CHAR8:
      memcpy( value, simpleProfileChar8, SIMPLEPROFILE_CHAR8_LEN );
      break; 
      
    case SIMPLEPROFILE_CHAR9:
      memcpy( value, simpleProfileChar9, SIMPLEPROFILE_CHAR9_LEN );
      break;
      
    case SIMPLEPROFILE_CHAR10:
      memcpy( value, simpleProfileChar10, SIMPLEPROFILE_CHAR10_LEN );
      break;
      
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          simpleProfile_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method)
{
  bStatus_t status = SUCCESS;

  // If attribute permissions require authorization to read, return error
  if ( gattPermitAuthorRead( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
      // gattserverapp handles those reads

      case SIMPLEPROFILE_CHAR1_UUID:
         *pLen = SIMPLEPROFILE_CHAR1_LEN;
          memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR1_LEN );
          break;
      case SIMPLEPROFILE_CHAR2_UUID:
         *pLen = SIMPLEPROFILE_CHAR2_LEN;
          memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR2_LEN );
          break;
      case SIMPLEPROFILE_CHAR3_UUID:
         *pLen = SIMPLEPROFILE_CHAR3_LEN;
          memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR3_LEN );
          break;
         
      case SIMPLEPROFILE_CHAR4_UUID:
        *pLen = SIMPLEPROFILE_CHAR4_LEN;
        memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR4_LEN );
        break;

      case SIMPLEPROFILE_CHAR5_UUID:
        *pLen = SIMPLEPROFILE_CHAR5_LEN;
        memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR5_LEN );
        break;
        
      case SIMPLEPROFILE_CHAR6_UUID:
        *pLen = SIMPLEPROFILE_CHAR6_LEN;
        memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR6_LEN );
        break;
        
      case SIMPLEPROFILE_CHAR7_UUID:
        *pLen = SIMPLEPROFILE_CHAR7_LEN;
        memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR7_LEN );
        break;
        
      case SIMPLEPROFILE_CHAR8_UUID:
        *pLen = SIMPLEPROFILE_CHAR8_LEN;
        memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR8_LEN );
        break;
        
      case SIMPLEPROFILE_CHAR9_UUID:
         *pLen = SIMPLEPROFILE_CHAR9_LEN;
          memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR9_LEN );
          break;
          
      case SIMPLEPROFILE_CHAR10_UUID:
         *pLen = SIMPLEPROFILE_CHAR10_LEN;
          memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR10_LEN );
          break;
          
          
      default:
        // Should never get here! 
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    *pLen = 0;
    status = ATT_ERR_INVALID_HANDLE;
  }

  return ( status );
}

/*********************************************************************
 * @fn      simpleProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method)
{
  bStatus_t status = SUCCESS;
  uint8 notifyApp = 0xFF;
  uint8 writeCB[20] = {0xcc,0xcc,0xcc,0xcc,0xcc};  
  
  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      case SIMPLEPROFILE_CHAR1_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR1_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
           VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR1_LEN);
           memcpy(S_Locker_App1_Config, pValue, len);
          for(int i=0; i<len; i++)
          {
            S_Locker_App1_Config[i] = S_Locker_App1_Config[i]+1;
          }
           memcpy(pAttr->pValue, &S_Locker_App1_Config[4], len-4);
           notifyApp = SIMPLEPROFILE_CHAR1;        
        }    
        break;
        
     case SIMPLEPROFILE_CHAR2_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR2_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
           VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR2_LEN);
           memcpy(S_Locker_App1_Config, pValue, len);
          for(int i=0; i<len; i++)
          {
            S_Locker_App1_Config[i] = S_Locker_App1_Config[i]+1;
          }
           memcpy(pAttr->pValue, &S_Locker_App1_Config[4], len-4);
           notifyApp = SIMPLEPROFILE_CHAR2;        
        }    
        break;   
        
      case SIMPLEPROFILE_CHAR3_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR3_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
          VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR3_LEN);
          memcpy(S_Locker_App1_Config, pValue, len);
          for(int i=0; i<len; i++)
          {
            S_Locker_App1_Config[i] = S_Locker_App1_Config[i]+1;
          }
          memcpy(pAttr->pValue, &S_Locker_App1_Config[4], len-4);
          notifyApp = SIMPLEPROFILE_CHAR3;        
        }    
        break;
        
      case SIMPLEPROFILE_CHAR4_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR4_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
           VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR4_LEN);
           memcpy(S_Locker_App1_Config, pValue, len);
          for(int i=0; i<len; i++)
          {
            S_Locker_App1_Config[i] = S_Locker_App1_Config[i]+1;
          }
           memcpy(pAttr->pValue, &S_Locker_App1_Config[4], len-4);
           notifyApp = SIMPLEPROFILE_CHAR4;        
        }    
        break;  
        
      case SIMPLEPROFILE_CHAR5_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR5_LEN)
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
          VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR5_LEN);
          memcpy(S_Locker_App1_Config, pValue, len);
          for(int i=0; i<len; i++)
          {
            S_Locker_App1_Config[i] = S_Locker_App1_Config[i]+1;
          }
          memcpy(pAttr->pValue, &S_Locker_App1_Config[4], len-4);
          notifyApp = SIMPLEPROFILE_CHAR5;        
        }    
        break;

      case SIMPLEPROFILE_CHAR6_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR6_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
           VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR6_LEN);
           memcpy(pAttr->pValue, pValue, len);
           notifyApp = SIMPLEPROFILE_CHAR6;        
        }    
        break;
        
        case SIMPLEPROFILE_CHAR7_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR7_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
           VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR7_LEN);
           memcpy(pAttr->pValue, pValue, len);
           notifyApp = SIMPLEPROFILE_CHAR7;        
        }    
        break;
        
        case SIMPLEPROFILE_CHAR8_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR8_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
           VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR8_LEN);
           memcpy(pAttr->pValue, pValue, len);
           notifyApp = SIMPLEPROFILE_CHAR8;        
        }    
        break;
        
      case SIMPLEPROFILE_CHAR9_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR9_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
           VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR9_LEN);
           memcpy(pAttr->pValue, pValue, len);
           notifyApp = SIMPLEPROFILE_CHAR9;        
        }    
        break;
        
      case SIMPLEPROFILE_CHAR10_UUID:
        if ( offset == 0 )
        {
          if ( len > SIMPLEPROFILE_CHAR10_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
           VOID memset(pAttr->pValue,0x00,SIMPLEPROFILE_CHAR10_LEN);
           memcpy(pAttr->pValue, pValue, len);
           notifyApp = SIMPLEPROFILE_CHAR10;        
        }    
        break;
        
      case GATT_CLIENT_CHAR_CFG_UUID:
        status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                                 offset, GATT_CLIENT_CFG_NOTIFY );
        break;
        
      default:
        // Should never get here! 
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    status = ATT_ERR_INVALID_HANDLE;
  }

  // If a characteristic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && simpleProfile_AppCBs && simpleProfile_AppCBs->pfnSimpleProfileChange )
  {
    simpleProfile_AppCBs->pfnSimpleProfileChange( notifyApp );  
  }
  
  return ( status );
}

/*********************************************************************
*********************************************************************/


/*********************************************************************

 *
 *  Developer     : dongxianglin
 *  Release_Date  : 2016.04.07
 *  Fuction       : 
 *
 *


 */


