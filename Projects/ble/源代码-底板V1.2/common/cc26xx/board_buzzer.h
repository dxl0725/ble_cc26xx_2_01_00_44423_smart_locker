
#ifndef BOARD_BUZZER_H
#define BOARD_BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#define BUZZER_EVT          0x01
// Semaphore globally used to post events to the application thread
//extern ICall_Semaphore BuzzerSem;


extern void Board_createTask4Buzzer();

extern void Board_Buzzer_Semaphore_post(uint16_t message);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_BUZZER_H */
