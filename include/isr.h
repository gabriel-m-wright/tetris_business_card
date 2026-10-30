#ifndef _ISR_H
#define _ISR_H

// Debounce time for button presses
#define DEBOUNCE_MS 10

// Tracks the state of button presses
extern volatile bool moveLeft;
extern volatile bool moveRight;
extern volatile bool rotate;
extern volatile bool softDrop;
// Becomes TRUE when it's time for the current mino to move down
extern volatile bool gravityTick;

extern volatile uint32_t msPassed;          // Tracks global milliseconds passed
extern volatile uint32_t tickSpeed;         // For overall speed up
extern volatile uint16_t gravityIntervalMs; // Tracks the interval (in milliseconds) between gravityTicks

#endif