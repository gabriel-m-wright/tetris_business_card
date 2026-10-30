#include "ti_msp_dl_config.h"
#include "isr.h"
#include "game.h"

#define SOFT_DROP_MS 80

volatile uint32_t msPassed = 0;
volatile bool gravityTick = false;
static   uint16_t gravityPassed = 0;
volatile uint16_t gravityIntervalMs = 500;

volatile bool moveLeft = false;
volatile bool moveRight = false;
volatile bool rotate = false;
volatile bool softDrop = false;

static volatile bool     leftLocked;
static volatile uint32_t leftLockedMs;
static volatile bool     rightLocked;
static volatile uint32_t rightLockedMs;
static volatile bool     upLocked;
static volatile uint32_t upLockedMs;
static volatile bool     downLocked;
static volatile uint32_t downLockedMs;

// Helper function for the timer. If a pin has been HIGH for longer than DEBOUNCE_MS it will arm itself.
static bool rearmPin(uint32_t pin, volatile bool *locked, volatile uint32_t *lockMs);

// Triggers on 1ms ticks
void TIMER_0_INST_IRQHandler(void){
    if (DL_TimerA_getPendingInterrupt(TIMER_0_INST) == DL_TIMER_IIDX_ZERO){
        msPassed++;
        gravityIntervalMs = softDrop && SOFT_DROP_MS < speedCurve[currentSpeed] ? SOFT_DROP_MS : speedCurve[currentSpeed];
        if (++gravityPassed >= gravityIntervalMs){
            gravityPassed = 0;
            gravityTick = true;
        }
    }
    rearmPin(GPIO_GRP_0_PIN_LEFT_PIN, &leftLocked, &leftLockedMs);
    rearmPin(GPIO_GRP_0_PIN_RIGHT_PIN, &rightLocked, &rightLockedMs);
    rearmPin(GPIO_GRP_0_PIN_UP_PIN, &upLocked, &upLockedMs);
    if (rearmPin(GPIO_GRP_0_PIN_DOWN_PIN, &downLocked, &downLockedMs)){
        softDrop = false;
    }
}

// Triggers on button inputs
void GPIOA_IRQHandler(void){
    switch (DL_GPIO_getPendingInterrupt(GPIOA)){
        case GPIO_GRP_0_PIN_LEFT_IIDX:
            moveLeft = true;
            DL_GPIO_disableInterrupt(GPIOA, GPIO_GRP_0_PIN_LEFT_PIN);
            leftLockedMs = msPassed;
            leftLocked = true;
            break;
        case GPIO_GRP_0_PIN_RIGHT_IIDX:
            moveRight = true;
            DL_GPIO_disableInterrupt(GPIOA, GPIO_GRP_0_PIN_RIGHT_PIN);
            rightLockedMs = msPassed;
            rightLocked = true;
            break;
        case GPIO_GRP_0_PIN_UP_IIDX:
            rotate = true;
            DL_GPIO_disableInterrupt(GPIOA, GPIO_GRP_0_PIN_UP_PIN);
            upLockedMs = msPassed;
            upLocked = true;
            break;
        case GPIO_GRP_0_PIN_DOWN_IIDX:
            softDrop = true;
            DL_GPIO_disableInterrupt(GPIOA, GPIO_GRP_0_PIN_DOWN_PIN);
            downLockedMs = msPassed;
            downLocked = true;
            break;
        default:
            break;
    }
}

// Helper function for the timer. If a pin has been HIGH for longer than DEBOUNCE_MS it will arm itself.
static bool rearmPin(uint32_t pin, volatile bool *locked, volatile uint32_t *lockMs){
    if (!*locked) return false;
    if (DL_GPIO_readPins(GPIOA, pin) == 0) {
        *lockMs = msPassed;
        return false;
    } else if ((msPassed - *lockMs) >= DEBOUNCE_MS) {
        DL_GPIO_clearInterruptStatus(GPIOA, pin);
        DL_GPIO_enableInterrupt(GPIOA, pin);
        *locked = false;
        return true;
    }
    return false;
}