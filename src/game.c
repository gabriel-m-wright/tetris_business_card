#include <stddef.h>
#include "game.h"
#include "IS31FL3741.h"
#include "ti_msp_dl_config.h"
#include "isr.h"

//----------[Definitions]----------//
#define LEVEL_UP 6 // Lines that need to be cleared to speed up.
#define BAG_SIZE 7 // Size of the randomized bag.

// Initial spawn position of each mino.
#define MINO_SPAWN_POSITION_X  2 + BORDER
#define MINO_SPAWN_POSITION_Y -3 + BOARD_HEIGHT - SCREEN_HEIGHT - BORDER 

#define BORDER       2  // Extra board space around the edges of the screen, except the top.
#define BOARD_HEIGHT 20
#define BOARD_WIDTH  SCREEN_WIDTH + BORDER * 2

// TODO: Set up a randomized seed.
#define SEED 1

#define SPEED_LEVELS (sizeof(speedCurve) / sizeof(speedCurve[0]))     // Number of levels of speed.

#define MINO_CELL(o,y,x) (((o) >> (15 - ((y) * 4 + (x)))) & 1)        // Converts an orientation and position to a 0 or 1.
#define Y_SCREEN_POS(y) (y - (BOARD_HEIGHT - SCREEN_HEIGHT - BORDER)) // Converts a board y location to a screen y location.
#define X_SCREEN_POS(x) (x - BORDER)                                  // Converts a board x location to a screen x location.

// Each orientation corresponds to an index of a Mino's shape array.
typedef enum Orientation{
    NORTH = 0,
    EAST  = 1,
    SOUTH = 2,
    WEST  = 3
} Orientation;

// shape[4] stores the four possible orientations of a mino.
typedef struct Mino{
    uint8_t color;
    uint16_t shape[4];
} Mino;

//----------[Game Data]----------//
// Available gravityTick periods, calculated externally with the formula s = 500 / (1.1 ^ currentSpeed)
const uint16_t speedCurve[] = {500,454,375,282,192,119,67,34,16,6}; 

// Currently selected index of speedCurve[].
uint8_t currentSpeed = 0;


/*
Each mino is encoded as a 4x4 square with a 1 corresponding to the presence of a block.
.X..
.XX.
.X..
....
This mino would become 0b01100011001000000.
*/
static const Mino minos[BAG_SIZE] = {
    {
        .color = CYAN,
        .shape = {0b0000111100000000,0b0010001000100010,0b0000000011110000,0b0100010001000100}
    },
    {
        .color = GREEN,
        .shape = {0b0011011000000000,0b0010001100010000,0b0000001101100000,0b0100011000100000}
    },
    {
        .color = RED,
        .shape = {0b0110001100000000,0b0001001100100000,0b0000011000110000,0b0010011001000000}
    },
    {
        .color = YELLOW,
        .shape = {0b0110011000000000,0b0110011000000000,0b0110011000000000,0b0110011000000000}
    },
    {
        .color = PURPLE,
        .shape = {0b0010011100000000,0b0010001100100000,0b0000011100100000,0b0010011000100000}
    },
    {
        .color = ORANGE,
        .shape = {0b0001011100000000,0b0010001000110000,0b0000011101000000,0b0110001000100000}
    },
    {
        .color = BLUE,
        .shape = {0b0100011100000000,0b0011001000100000,0b0000011100010000,0b0010001001100000}
    }
};

static uint8_t gameBoard[BOARD_HEIGHT][BOARD_WIDTH]; // Stores the color data of every tile on the board.
static uint8_t bag[BAG_SIZE];                        // Stores the indices of the BAG_SIZE upcoming pieces.

static uint32_t lineCount = 0;                       // Number of lines cleared in the current level.
static uint8_t  bagIndex  = BAG_SIZE;                // Initializes the current bag index to force an initial shuffle.
static uint32_t rngState  = SEED;                    // Initializes the RNG to the initial seed.

// Stores the state of the currently selected mino.
static struct {
    const Mino *mino;
    Orientation orientation;
    uint8_t x, y;
} activeMino = { 
    .mino = NULL,
    .orientation = NORTH,
    .x = MINO_SPAWN_POSITION_X,
    .y = MINO_SPAWN_POSITION_Y,
};

//----------[Static Function Declaration]----------//

// Drawing
static void    drawMino(bool clear);  // If clear is set, erase the mino instead.
static void    clearRows(void);       // Checks if any rows are full and need to be cleared. If so, clear them.
static void    reWriteScreen(void);   // Refreshes the entire screen

// Testing
static bool canBePlaced(uint8_t y, uint8_t x, Orientation orientation); // Checks if the mino can be placed at the given location.
static bool rowFull(uint8_t y);                                         // Returns TRUE if the row at y is full, and FALSE if not.
static bool inBounds(uint8_t y, uint8_t x);                             // Determines if the location (y,x) is in the game bounds.
static bool onScreen(uint8_t y, uint8_t x);                             // Determines if the location (y,x) is on screen.

// Gameplay
static bool        createMino(void);  // Creates a random mino. Returns FALSE if it can't.
static const Mino* selectNext(void);  // Selects the next available mino
static uint32_t    rngNext(void);     // Selects a random number
static bool        lowerMino(void);   // Attempts to lower the mino by one. Returns FALSE if it can't.
static void        setMino(void);     // Writes the mino to the game board.
static void        restartGame(void); // Restarts the game

//----------[Definitions]----------//

void moveMino(int8_t direction){
    if (!activeMino.mino) return;
    if (canBePlaced(activeMino.y, activeMino.x + direction, activeMino.orientation)) {
        drawMino(true);
        activeMino.x += direction;
        drawMino(false);
    }
}

void rotateMino(void){
    if (!activeMino.mino) return;
    Orientation next = (Orientation)((activeMino.orientation + 1) & 3);
    if (canBePlaced(activeMino.y, activeMino.x, next)){
        drawMino(true);
        activeMino.orientation = next;
        drawMino(false);
    }
}

void gameTick(void){
    if (activeMino.mino == NULL){
        if (!createMino()) restartGame();
        return;
    }
    
    if (!lowerMino()) {
        setMino();
        clearRows();
        if (!createMino()) restartGame();
        return;
    }
}

static void drawMino(bool clear){
    if (!activeMino.mino) return;

    uint16_t toDraw = activeMino.mino->shape[activeMino.orientation];
    uint8_t color = clear ? BLACK : activeMino.mino->color;

    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            if (onScreen(activeMino.y + j, activeMino.x + i) && MINO_CELL(toDraw, j, i) != BLACK) {
                writePixel(Y_SCREEN_POS(activeMino.y) + j, X_SCREEN_POS(activeMino.x) + i, color);
            }
        }
    }
}

static void clearRows(){
    uint8_t clearCount = 0;
    uint8_t toClear[4];
    for (int i = activeMino.y; i < activeMino.y + 4; i++){
        if (rowFull(i)){
            toClear[clearCount] = i;
            clearCount++;
            if (onScreen(i, BORDER)){
                for (int j = 0; j < SCREEN_WIDTH; j++) {
                    writePixel(Y_SCREEN_POS(i), j, WHITE);
                }
            }
        }
    }
    if (clearCount) {
        uint32_t start = msPassed;
        while ((msPassed - start) < 150) {
            __WFI();
        }
        for (int i = 0; i < clearCount; i++){
            for (int j = toClear[i]; j > 0; j--){
                for (int k = 0; k < BOARD_WIDTH; k++){
                    gameBoard[j][k] = gameBoard[j-1][k];
                }
            }
            for (int j = 0; j < BOARD_WIDTH; j++){
                gameBoard[0][j] = 0;
            }
        }
        lineCount += clearCount;
        reWriteScreen();
        if (lineCount >= LEVEL_UP) {
            lineCount -= LEVEL_UP;
            if (currentSpeed < SPEED_LEVELS - 1)
                currentSpeed++;
            gravityIntervalMs = speedCurve[currentSpeed];
        }
    }
}

static void reWriteScreen(){
    for (int i = 0 + BOARD_HEIGHT - BORDER - SCREEN_HEIGHT; i < BOARD_HEIGHT - BORDER; i++){
        for (int j = 0 + BORDER; j < SCREEN_WIDTH + BORDER; j++){
            writePixel(Y_SCREEN_POS(i), X_SCREEN_POS(j), gameBoard[i][j]);
        }
    }
}

static bool canBePlaced(uint8_t y, uint8_t x, Orientation orientation){
    uint16_t toDraw = activeMino.mino->shape[orientation];
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            if (MINO_CELL(toDraw, j, i) && (!inBounds(y + j, x + i) || (gameBoard[y+j][x+i] != 0))) return false;
        }
    }
    return true;
}

static bool rowFull(uint8_t y){
    for (int i = 0; i < SCREEN_WIDTH; i++){
        if (gameBoard[y][BORDER + i] == BLACK){
            return false;
        }
    }
    return true;
}

static bool inBounds(uint8_t y, uint8_t x){
    if (x >= BORDER && x < BORDER + SCREEN_WIDTH && y < BOARD_HEIGHT - BORDER) {
        return true;
    } else {
        return false;
    }
}

static bool onScreen(uint8_t y, uint8_t x){
    if (x >= BORDER & x < BORDER + SCREEN_WIDTH & y < BOARD_HEIGHT - BORDER & y >= BOARD_HEIGHT - SCREEN_HEIGHT - BORDER){
        return true;
    } else {
        return false;
    }
}

static bool createMino(){
    softDrop               = false;
    activeMino.mino        = selectNext();
    activeMino.orientation = NORTH;
    activeMino.x           = MINO_SPAWN_POSITION_X;
    activeMino.y           = MINO_SPAWN_POSITION_Y;

    if (canBePlaced(activeMino.y, activeMino.x, activeMino.orientation)){
        drawMino(false);
        return true;
    } else {
        return false;
    }
}

static const Mino* selectNext(){
    if (bagIndex >= BAG_SIZE) {
        for (uint8_t i = 0; i < BAG_SIZE; i++) bag[i] = i;
        for (uint8_t i = 6; i > 0; i--){
            uint8_t j = ((rngNext() >> 16) * (i + 1)) >> 16;
            uint8_t t = bag[i]; 
            bag[i] = bag[j]; 
            bag[j] = t;
        }
        bagIndex = 0;
    }
    return &(minos[bag[bagIndex++]]);
}

static uint32_t rngNext(){
    uint32_t x = rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rngState = x;
}

static bool lowerMino(){
    if (!canBePlaced(activeMino.y + 1, activeMino.x, activeMino.orientation)){
        return false;
    } else {
        drawMino(true);
        activeMino.y++;
        drawMino(false);
        return true;
    }
}

static void setMino(){
    uint16_t toDraw = activeMino.mino->shape[activeMino.orientation];
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            if (MINO_CELL(toDraw, j, i) != BLACK) {
                gameBoard[j + activeMino.y][i + activeMino.x] = activeMino.mino->color;
            }
        }
    }
    activeMino.mino = NULL;
}

static void restartGame(){
    for (int j = SCREEN_HEIGHT - 1; j >= 0; j--){
        for (int i = 0; i < SCREEN_WIDTH; i++){
            writePixel(j, i, WHITE);
            delay_cycles(400000);
        }
    }

    for (int i = 0; i < BOARD_WIDTH; i++){
        for (int j = 0; j < BOARD_HEIGHT; j++){
            gameBoard[j][i] = 0;
        }
    }
    for (int j = SCREEN_HEIGHT - 1; j >= 0; j--){
        for (int i = 0; i < SCREEN_WIDTH; i++){
            writePixel(j, i, BLACK);
            delay_cycles(400000);
        }
    }
    
    currentSpeed      = 0;
    gravityIntervalMs = speedCurve[currentSpeed];
    lineCount         = 0;
    activeMino.mino   = NULL;
    moveLeft          = false;
    moveRight         = false;
    rotate            = false;
    gravityTick       = false;
    softDrop          = false;
}