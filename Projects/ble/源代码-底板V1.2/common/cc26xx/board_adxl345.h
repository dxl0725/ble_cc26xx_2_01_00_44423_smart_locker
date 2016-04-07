#ifndef BOARD_ADXL345_H
#define BOARD_ADXL345_H

#ifdef __cplusplus
extern "C" {
#endif

#define ADXL345_SDO_LOW         0x53	  //定义器件在IIC总线中的从地址,根据ALT  ADDRESS地址引脚不同修改  p17
#define ADXL345_DAT_ADDR        0x32	  //加速度地址
#define ADXL345_DEVID_ADDR      0x00	//器件ID  p22
#define ADXL345_DEVID           0xE5	//器件ID  p22



extern uint8 adxl345_Init();

extern bool adxl345_read16(int16_t *ax, int16_t *ay, int16_t *az);


#ifdef __cplusplus
}
#endif

#endif /* BOARD_ADXL345_H */
