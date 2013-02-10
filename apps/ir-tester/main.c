#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <stm32f10x.h>

//#define USE_TIM17_PWM   1
#define MOD_FREQ        36000
#define PWM_FREQ        50
#define PWM_WIDTH       0.3
#define PWM4_CHANNEL    3

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

#ifdef USE_TIM17_PWM
void prepare_tim17(void) {
    //Enable clock for the timer
    RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
    // Enable ARR preloading
    TIM17->CR1 |= TIM_CR1_ARPE;
    // Enable CCR1 preloading
    TIM17->CCMR1 |= TIM_CCMR1_OC2PE;
    // PWM mode 1: OC1M = 110
    TIM17->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1);
    // Prescaler keeps zero to allow control PWM frequency
    TIM17->PSC = (uint16_t) (STM32_SYSCLK / MOD_FREQ) + 1;
    // PWM Frequency
    TIM17->ARR = (uint16_t) PWM_FREQ;
    // Duty cycle
    TIM17->CCR1 = (uint16_t) (PWM_FREQ * PWM_WIDTH);
    // Uncomment to change polarity
    //TIM2->CCER |= TIM_CCER_CC2P;
    // Enable Capture/Compare output 1, ie PB9
    TIM17->CCER |= TIM_CCER_CC1E;
    // Main Output enable
    TIM17->BDTR |= TIM_BDTR_MOE;
    // Start timer
    TIM17->CR1 |= TIM_CR1_CEN;

    chprintf((BaseSequentialStream *) &SD1, "TIM17->PSC = %d\n", TIM17->PSC);
    chprintf((BaseSequentialStream *) &SD1, "TIM17->ARR = %d\n", TIM17->ARR);
    chprintf((BaseSequentialStream *) &SD1, "TIM17->CCR1 = %d\n", TIM17->CCR1);
}
#else
static PWMConfig pwmcfg = {
    MOD_FREQ,                           /* PWM clock frequency, PSC register. */
    PWM_FREQ,                           /* Initial PWM period. ARR register*/
    NULL,                               /* Periodic callback pointer. */
    {                                   /* Channel configuration set dynamically below, */
        {PWM_OUTPUT_DISABLED, NULL},
        {PWM_OUTPUT_DISABLED, NULL},
        {PWM_OUTPUT_DISABLED, NULL},
        {PWM_OUTPUT_DISABLED, NULL}
    },
    0                                   /* TIM CR2 register initialization data. */
#if STM32_PWM_USE_ADVANCED
    ,0                                  /* TIM BDTR register initialization data. */
#endif
};
#endif

static icucnt_t last_width = 0;
static icucnt_t last_period = 0;

// Callback for pulse width measurement.
static void icu_width_cb(ICUDriver *icup) {
    last_width = icuGetWidth(icup);
}

// Callback for cycle period measurement.
static void icu_period_cb(ICUDriver *icup) {
    last_period = icuGetPeriod(icup);
}

static ICUConfig icucfg = {
    ICU_INPUT_ACTIVE_HIGH,            /* mode: rising edge start the signal */
    MOD_FREQ,                         /* ICU clock modulation frequency */
    icu_width_cb,                     /* callback for pulse width measurement */
    icu_period_cb,                    /* callback for cycle period measurement */
    NULL,                             /* callback for timer overflow */
    ICU_CHANNEL_1                     /* timer input channel to be used, ie PA0 */
};


// Program entry point.
int main(void) {

    // System initialization.
    halInit();
    chSysInit();

    // enable serial interface
    sdStart(&SD1, NULL);

#ifdef USE_TIM17_PWM
    prepare_tim17();
#else
    PWMChannelConfig chcfg = { PWM_OUTPUT_ACTIVE_HIGH, NULL };
    pwmcfg.channels[PWM4_CHANNEL] = chcfg;
    pwmStart(&PWMD4, &pwmcfg);
    pwmEnableChannel(&PWMD4, PWM4_CHANNEL,
        PWM_PERCENTAGE_TO_WIDTH(&PWMD4, PWM_WIDTH * 10000));

    chprintf((BaseSequentialStream *) &SD1, "TIM4->PSC = %d\n", TIM4->PSC);
    chprintf((BaseSequentialStream *) &SD1, "TIM4->ARR = %d\n", TIM4->ARR);
    chprintf((BaseSequentialStream *) &SD1, "TIM4->CCR4 = %d\n", TIM4->CCR4);
#endif /* USE_TIM17_PWM */

    // Run blinker
    Thread* th_blinker = chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, blinker, NULL);

    chprintf((BaseSequentialStream *) &SD1, "\nDevice started\n");

    // Run ICU
    icuStart(&ICUD2, &icucfg);
    icuEnable(&ICUD2);

    palClearPad(GPIOA, GPIOA_ICU_LED); // on
    chThdSleepMilliseconds(500);

    // Main thread.
    while (!chThdShouldTerminate()) {
        chThdSleepMilliseconds(500);

        chprintf((BaseSequentialStream *) &SD1, "Period(%d)\tWidth(%d)\n", last_period, last_width);

        if (palReadPad(GPIOA, GPIOA_IR_RX)) {
            palClearPad(GPIOA, GPIOA_ICU_LED); // on
        } else {
            palSetPad(GPIOA, GPIOA_ICU_LED); // off
        }
    }

    palSetPad(GPIOA, GPIOA_ICU_LED); // off

    icuDisable(&ICUD2);
    icuStop(&ICUD2);

#ifndef USE_TIM17_PWM
    pwmDisableChannel(&PWMD4, 0);
    pwmStop(&PWMD4);
#endif /* USE_TIM17_PWM */

    chThdTerminate(th_blinker);
}
