/*
Based on ChibiOS/testhal/STM32F0xx/ADC,

PA2(TX) and PA3(RX) are routed to USART2 38400-8-N-1.

PC0 and PC1 are analog inputs
Connect PC0 to 3.3V and PC1 to GND for analog measurements.

*/
#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "usbcfg.h"

/*
 * DP resistor control is not possible on the STM32F3-Discovery, using stubs
 * for the connection macros.
 */

#define usb_lld_connect_bus(usbp)
#define usb_lld_disconnect_bus(usbp)

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static void cmd_led(BaseSequentialStream *chp, int argc, char *argv[]) {
    chprintf(chp, "Toggle LINE_LED3_RED\n\r");  
    palToggleLine(LINE_LED3_RED);
}

static const ShellCommand shCmds[] = {
    {"led",  cmd_led},
    {NULL, NULL}
};

static const ShellConfig shCfg = {
    (BaseSequentialStream *)&SDU1,
    shCmds
};



/*
 * Application entry point.
 */
int main(void) {

  halInit();
  chSysInit();

  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  shellInit();

  while (true) {
    if (SDU1.config->usbp->state == USB_ACTIVE) {
      thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                              "shell", NORMALPRIO + 1,
                                              shellThread, (void *)&shCfg);
      chThdWait(shelltp);               /* Waiting termination.             */
    }
    chThdSleepMilliseconds(1000);
  }
}
