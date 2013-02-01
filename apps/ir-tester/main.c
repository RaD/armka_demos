#include <ch.h>
#include <hal.h>
#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>


// Simple thread for green LED blinkng.
static WORKING_AREA(waBlinker, 128);
static msg_t blinker(void *arg) {
    (void) arg;
    chRegSetThreadName("blinker");
    while (!chThdShouldTerminate()) {
        palTogglePad(GPIOB, GPIOB_ARMKA_LED);
        chThdSleepMilliseconds(500);
    }
    return 0;
}

void meandre_tim17(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // port B clock enable
    RCC->APB2ENR |= RCC_APB2ENR_TIM17EN; // enable TIM17
    TIM17->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0);
    TIM17->CCER |= TIM_CCER_CC1E; // 1 output enable
    TIM17->BDTR |= TIM_BDTR_MOE;
    TIM17->ARR = 30000;
    TIM17->CCR1 =15000;
    TIM17->CR1 |= TIM_CR1_CEN ;// TIM17 start
}

// Program entry point.
int main(void) {

    // System initialization.
    halInit();
    chSysInit();

    meandre_tim17();

    // Run blinker
    (void) chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, blinker, NULL);

    // Main thread.
    while (!chThdShouldTerminate()) {
        chThdSleepMilliseconds(MS2ST(500));
    }
}
