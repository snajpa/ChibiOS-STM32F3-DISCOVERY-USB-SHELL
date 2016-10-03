/*
Based on ChibiOS/testhal/STM32F0xx/ADC,

PA2(TX) and PA3(RX) are routed to USART2 38400-8-N-1.

PC0 and PC1 are analog inputs
Connect PC0 to 3.3V and PC1 to GND for analog measurements.

*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

bool madtoggle = false;
gptcnt_t divider = 100000;

/*
 * Blinker thread #1.
 */
THD_WORKING_AREA(waThread1, 128);
THD_FUNCTION(Thread1, arg) {

  (void)arg;

  chRegSetThreadName("blinker");
  while (true) {
    if (madtoggle) {
        palToggleLine(LINE_LED3_RED);
        chThdSleepMilliseconds(100);
        palToggleLine(LINE_LED7_GREEN);
        chThdSleepMilliseconds(100);
        palToggleLine(LINE_LED10_RED);
        chThdSleepMilliseconds(100);
        palToggleLine(LINE_LED6_GREEN);
    };
    chThdSleepMilliseconds(100);
  };
};


static void cmd_led(BaseSequentialStream *chp, int argc, char *argv[]) {
    if (madtoggle) {
        chprintf(chp, "Turbo blinkon mad toggle OFF\n\r");  
        madtoggle = false;
    } else {
        chprintf(chp, "Turbo blinkon mad toggle ON\n\r");  
        madtoggle = true;
    }
}
static void cmd_div(BaseSequentialStream *chp, int argc, char *argv[]) {
    gptcnt_t oldivider = divider;

    if (argc != 1) {
        chprintf(chp, "Usage: div <value>");
        return;
    }

    if ((divider = atoi(argv[0])) > 0) {
        chprintf(chp, "Current rate %d, setting to %d\n\r", oldivider, divider);
        gptChangeInterval(&GPTD1, divider);
    } else {
        chprintf(chp, "Kokot I got %s, but I no understand\n\r", argv[0]);
        chprintf(chp, "Kokot, divider is set to %d and should be %d\n\r", divider,
                oldivider);

    }
}
static void cmd_accel(BaseSequentialStream *chp, int argc, char *argv[]) {
    gptcnt_t oldivider = divider;
    for (int i = 65005; i > 1000; i-=1000) {
        divider = i;
        gptChangeInterval(&GPTD1, divider);
        chThdSleepMilliseconds(100);
    }
    divider = oldivider;
    gptChangeInterval(&GPTD1, divider);
}


static const ShellCommand shCmds[] = {
    {"led",  cmd_led},
    {"div",  cmd_div},
    {"accel",  cmd_accel},
    {NULL, NULL}
};

static const ShellConfig shCfg = {
    (BaseSequentialStream *)&SDU1,
    shCmds
};

static void gpt1cb(GPTDriver *gptp) {
    (void)gptp;
    palToggleLine(LINE_LED5_ORANGE);
}

static const GPTConfig gpt1cfg = {
    1000000,  /* 1MHz timer clock.*/
    gpt1cb,   /* Timer callback.*/
    0,
    0
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

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1, Thread1, NULL);

  gptStart(&GPTD1, &gpt1cfg);
  gptStartContinuous(&GPTD1, divider);

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
