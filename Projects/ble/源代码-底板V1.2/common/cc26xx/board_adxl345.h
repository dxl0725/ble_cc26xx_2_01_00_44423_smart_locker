#ifndef BOARD_ADXL345_H
#define BOARD_ADXL345_H

#ifdef __cplusplus
extern "C" {
#endif

#define ADXL345_SDO_LOW         0x53	  //����������IIC�����еĴӵ�ַ,����ALT  ADDRESS��ַ���Ų�ͬ�޸�  p17
#define ADXL345_DAT_ADDR        0x32	  //���ٶȵ�ַ
#define ADXL345_DEVID_ADDR      0x00	//����ID  p22
#define ADXL345_DEVID           0xE5	//����ID  p22



extern uint8 adxl345_Init();

extern bool adxl345_read16(int16_t *ax, int16_t *ay, int16_t *az);


#ifdef __cplusplus
}
#endif

#endif /* BOARD_ADXL345_H */
