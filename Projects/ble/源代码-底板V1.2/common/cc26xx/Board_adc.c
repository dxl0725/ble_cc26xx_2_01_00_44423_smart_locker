#include <inc/hw_memmap.h>
#include <driverlib/aux_wuc.h>
#include <driverlib/aux_adc.h>
#include <inc/hw_aux_evctl.h>
#include <inc/hw_adi.h>
#include <inc/hw_adi_4_aux.h>
#include <inc/hw_aux_anaif.h> // Will change name to anaif


// auxIo, see MUX3 Register (Offset = 3h) [reset = X]
// auxIo, sensor controller engine IO, will map to M3's IO automatilcally
// for DIO23, auxIO7, the value should be 80h, 0x80
uint16_t OneShotADC(uint8_t auxIo)
{
  static __root uint16_t retval = 0xABBA;
  uint32_t turnedOnClocks = 0;
  

  AUXADCSelectInput(ADC_COMPB_IN_AUXIO7);
   
  // Only turn on clocks that are not already enabled. Not thread-safe, obviously.
  turnedOnClocks |= AUXWUCClockStatus(AUX_WUC_ADC_CLOCK)?0:AUX_WUC_ADC_CLOCK;
  turnedOnClocks |= AUXWUCClockStatus(AUX_WUC_ADI_CLOCK)?0:AUX_WUC_ADI_CLOCK;
  turnedOnClocks |= AUXWUCClockStatus(AUX_WUC_SOC_CLOCK)?0:AUX_WUC_SOC_CLOCK;
  
  // Enable clocks and wait for ready
  AUXWUCClockEnable(turnedOnClocks);
  while(false == AUXWUCClockStatus(turnedOnClocks));
  
  // Set ADC Config: Sample mode Synchronous, 3 cycles sample, Reset, Disable.
  HWREGB(AUX_ADI4_BASE + ADI_4_AUX_O_ADC0) = (0 << ADI_4_AUX_ADC0_SMPL_MODE_S) | 
                                            (6 << ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_S) |
                                            (0 << ADI_4_AUX_ADC0_RESET_N_S) |
                                            (0 << ADI_4_AUX_ADC0_EN_S );
  
  // Set reference source: Keep powered when idle, Fixed Src 4.3V, Enable reference system
  HWREGB(AUX_ADI4_BASE + ADI_4_AUX_O_ADCREF0) = (1 << ADI_4_AUX_ADCREF0_REF_ON_IDLE_S) |
                                               (0 << ADI_4_AUX_ADCREF0_SRC_S) |
                                               (1 << ADI_4_AUX_ADCREF0_EN_S );

  // Set TRIM values
  //HWREGB(AUX_ADI_BASE + ADI_4_AUX_O_ADCREF1) = (1 << ADI_4_AUX_ADCREF1_VTRIM_S) |
  //                                             (0 << ADI_4_AUX_ADCREF1_ITRIM_S);

  // Connect the muxed AUX IO Channel, light sensor is DIO23, which is AUXIO7 on the 7x7 package
  //HWREGB(AUX_ADI4_BASE + ADI_4_AUX_O_MUX3) = auxIo;
  HWREGB(AUX_ADI4_BASE + 0x00000003) = auxIo;
  
  // Release ADC from reset and enable
  HWREGB(AUX_ADI4_BASE + ADI_4_AUX_O_ADC0) |= (1 << ADI_4_AUX_ADC0_RESET_N_S) |
                                             (1 << ADI_4_AUX_ADC0_EN_S );

  // Control M3's Analog Interface. Set polarity for trigger, but we don't care. Set trig source to NO_EVENT. Enable ADC interface for the remaining register writes.
  HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL) = (0 << AUX_ANAIF_ADCCTL_START_POL_S) |
                                           (9 << AUX_ANAIF_ADCCTL_START_SRC_S ) | // 9 is NO_EVENT, which is what we want for software trigger
                                           (1 << AUX_ANAIF_ADCCTL_CMD_S); // 

  // Trigger the ADC conversion
  HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCTRIG) = (1 << AUX_ANAIF_ADCTRIG_START_S);
  
  // Wait until it's finished (when fifo is not empty)
  while ( 1 == (HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCFIFOSTAT) & AUX_ANAIF_ADCFIFOSTAT_EMPTY) );
  
  // Read out value from FIFO
  retval = HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCFIFO);
  
  
  // Clean up.
  // Disable reference system
  HWREGB(AUX_ADI4_BASE + ADI_4_AUX_O_ADCREF0) &= ~(1 << ADI_4_AUX_ADCREF0_EN_S );

  // Turn off clocks
  AUXWUCClockDisable(turnedOnClocks);
  // Turn off ADC module
  HWREGB(AUX_ADI4_BASE + ADI_4_AUX_O_ADC0) = (0 << ADI_4_AUX_ADC0_RESET_N_S) |
                                            (0 << ADI_4_AUX_ADC0_EN_S );
  

  return retval;
}