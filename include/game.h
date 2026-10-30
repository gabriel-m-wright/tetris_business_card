#ifndef _GAME_H
#define _GAME_H

#include <stdint.h>
#include "IS31FL3741.h"

#define BORDER 2
#define BOARD_HEIGHT 20
#define BOARD_WIDTH SCREEN_WIDTH + BORDER * 2

#define MINO_SPAWN_POSITION_X 2 + BORDER
#define MINO_SPAWN_POSITION_Y 0 + BOARD_HEIGHT - SCREEN_HEIGHT - BORDER

#define LEFT -1
#define RIGHT 1

#define BASE_TICK_TIME_MS 500

#define BAG_SIZE 7

#define MULTIPLIER_SPEED_UP 6
#define MULTIPLIER_LEVEL    12
#define LEVEL_UP            6
#define SOFT_DROP_MS 80

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

// Performs all actions needed each game tick.
void gameTick(void);

// Creates a random mino. Returns FALSE if it can't.
bool createMino(void);

// Clears the board and restarts the game.
void restartGame(void);

// Attempts to lower the mino by one. Returns FALSE if it can't.
bool lowerMino(void);

// Writes the mino to the game board.
void setMino(void);

// Checks if any rows are full and need to be cleared. If so, clear them.
void clearRows(void);

// If clear is set, erase the mino instead.
void drawMino(bool clear);

// Selects the next available mino
const Mino* selectNext(void);

// Selects a random number
uint32_t rngNext(void);

// Converts the board position to its screen position
uint8_t yScreenPos(uint8_t y);
uint8_t xScreenPos(uint8_t x);

// Determines whether a logical location is in game bounds or on the screen, respectively.
bool inBounds(uint8_t y, uint8_t x);
bool onScreen(uint8_t y, uint8_t x);

// Checks if the current mino can be placed at the given location.
bool canBePlaced(uint8_t y, uint8_t x, Orientation orientation);

// Restarts the game
void restartGame(void);

// Attempts to lower the mino by one. If it can't, it returns FALSE and does nothing else.
bool lowerMino(void);

// Moves the mino LEFT or RIGHT
void moveMino(int8_t);

// Rotates mino CLOCKWISE
void rotateMino(void);

// Refreshes the entire screen
void reWriteScreen(void);


#endif