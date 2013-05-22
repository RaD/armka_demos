#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <stm32f10x.h>

// Simple thread for green LED blinkng.
static WORKING_AREA(waIdleThread, 128);
static msg_t idleThread(void *arg) {
    (void) arg;
    chRegSetThreadName("idleThread");

    while (!chThdShouldTerminate()) {
        palTogglePad(GPIOB, GPIOB_ARMKA_LED);
        chThdSleepMilliseconds(500);
    }
    return 0;
}

// Program entry point.
int main(void) {

    // System initialization.
    halInit();
    chSysInit();

    // enable serial interface
    sdStart(&SD1, NULL);

    Thread* th_series = chThdCreateStatic(waIdleThread, sizeof(waIdleThread), NORMALPRIO, idleThread, NULL);

    chprintf((BaseSequentialStream *) &SD1, "\nDevice started\n");

    palSetPad(GPIOA, GPIOA_ICU_LED); // off
    chThdSleepMilliseconds(500);

    // Main thread.
    while (!chThdShouldTerminate()) {
        if (palReadPad(GPIOA, GPIOA_IR_RX)) {
            palSetPad(GPIOA, GPIOA_ICU_LED); // off
        } else {
            palClearPad(GPIOA, GPIOA_ICU_LED); // on
        }
    }

    palSetPad(GPIOA, GPIOA_ICU_LED); // off

    chThdTerminate(th_series);
}
