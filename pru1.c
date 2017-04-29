/*
 * This version runs an oscillator on pin XXX to drive the A/D.
 * The period of the oscillator is set by the ARM which sets the
 * oscillator period on start-up.
 *
 */
#include <stdint.h>
#include "pru_cfg.h"
#include "pru_intc.h"
#include "pru_ctrl.h"
#include "resource_table_empty.h"

volatile register uint32_t __R30;
volatile register uint32_t __R31;

/* Mapping Constant table register to variable */
//volatile far uint32_t MEM_BASE __attribute__((cregister("PRU_COMM_RAM1", near), peripheral));

/* PRU-to-ARM interrupt */
#define PRU0_ARM_INTERRUPT (19+16)

// My device tree defines bit 8 as the output bit
#define CLK_OUT 8

// Variables used to set oscillator parameters
unsigned int upcnt;
unsigned int dncnt;

//====================================================
int main(void) {
  uint32_t delay, period;

  // Map MEM into my address space, then get oscillator period
  //uint32_t *pDdr;
  //pDdr = (uint32_t *) &MEM_BASE;

  period = 100000;  //*pDdr;

  while (1) {

    // Now run oscillator
    __R30 = __R30 | (1 << CLK_OUT); // Turn on the bit
    for (delay = 0; delay < period; delay++) {
      // loop delay
    }
    __R30 = __R30 & ~(1 << CLK_OUT); // Turn off the bit
    for (delay = 0; delay < period; delay++) {
      // loop delay
    }
  }
  // We'll never get here
  return 0;
}
