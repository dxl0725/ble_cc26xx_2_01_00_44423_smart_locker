#ifndef BOARD_ADC_H
#define BOARD_ADC_H

#ifdef __cplusplus
extern "C" {
#endif
// auxIo, see MUX3 Register (Offset = 3h) [reset = X]
// auxIo, sensor controller engine IO, will map to M3's IO automatilcally
// for DIO23, auxIO7, the value should be 80h, 0x80
extern uint16_t OneShotADC(uint8_t auxIo);



#ifdef __cplusplus
}
#endif

#endif /* BOARD_ADC_H */

