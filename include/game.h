#ifndef _GAME_H
#define _GAME_H

#include <stdint.h>
#include "IS31FL3741.h"

#define BORDER 2
#define BOARD_HEIGHT 20
#define BOARD_WIDTH SCREEN_WIDTH + BORDER * 2

#define MINO_SPAWN_POSITION_X 2 + BORDER
#define MINO_SPAWN_POSITION_Y -3 + BOARD_HEIGHT - SCREEN_HEIGHT - BORDER

#define LEFT -1
#define RIGHT 1

#define BASE_TICK_TIME_MS 500

#define BAG_SIZE 7

#define MULTIPLIER_SPEED_UP 6
#define MULTIPLIER_LEVEL    12
#define LEVEL_UP            6
#define SOFT_DROP_MS        80

// Determines the value of a mino. Pass it an orientation, and the y and x values you want to read.
#define MINO_CELL(o,y,x) (((o) >> (15 - ((y) * 4 + (x)))) & 1)

// TODO: Set up a randomized seed.
#define SEED 1

typedef enum Orientation{
    NORTH,
    EAST,
    SOUTH,
    WEST
} Orientation;

/*
Each mino is enclosed in a 4x4 square. Each rotation is encoded as follows:
.X..
.XX.
.X..
....
This mino would become 0b01100011001000000, with each 1 corresponding to the presence of a square. This fits neatly in 16 bits.
*/

typedef struct Mino{
    uint8_t color;
    uint16_t shape[4];
} Mino;

typedef struct ActiveMino{
    const Mino* mino;
    Orientation orientation;
    uint8_t x;
    uint8_t y;
} ActiveMino;

extern const Mino i_block;
extern const Mino s_block;
extern const Mino z_block;
extern const Mino o_block;
extern const Mino t_block;
extern const Mino l_block;
extern const Mino j_block;

const extern uint16_t speedCurve[10];
extern uint8_t currentSpeed;

void moveMino(int8_t); // Moves the mino LEFT or RIGHT
void rotateMino(void); // Rotates mino CLOCKWISE
void gameTick(void);   // Performs all actions needed each game tick.

#endif