#ifndef _GAME_H
#define _GAME_H

#include <stdint.h>
#include "IS31FL3741.h"

// Distance moved when the left and right buttons are pressed, respectively.
#define LEFT -1
#define RIGHT 1

const extern uint16_t speedCurve[10]; // Stores a list of gravityTick periods to progress upon level-up.
extern uint8_t        currentSpeed;   // Stores the current selected index of speedCurve[].

void moveMino(int8_t); // Moves the mino LEFT or RIGHT
void rotateMino(void); // Rotates mino CLOCKWISE
void gameTick(void);   // Performs all actions needed each game tick.

#endif