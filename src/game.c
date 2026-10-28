#include <stddef.h>
#include "game.h"
#include "IS31FL3741.h"
#include "ti_msp_dl_config.h"
#include "isr.h"

const uint16_t speedCurve[] = {500,454,375,282,192,119,67,34,16,6};
uint8_t currentSpeed = 0;

#define SPEED_LEVELS (sizeof(speedCurve) / sizeof(speedCurve[0]))

// There will be a 2 unit buffer around every playable edge of the board.
static uint8_t gameBoard[BOARD_HEIGHT][SCREEN_WIDTH + 4];

static ActiveMino activeMino = {
    .mino = NULL,
    .orientation = NORTH,
    .x = MINO_SPAWN_POSITION_X,
    .y = MINO_SPAWN_POSITION_Y,
};

static uint32_t lineCount = 0;

// Stores the seven upcoming pieces as indices in minos[].
static uint8_t bag[7];
static uint8_t bagIndex = 7;

static uint32_t rngState = SEED;

const Mino minos[7] = {
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
void gameTick(void){
    if (activeMino.mino == NULL){
        if (!createMino()) restartGame();
        return;
    }
    
    if (!lowerMino()) {
        setMino();
        clearRows();
        return;
    }
}

// TODO: Potential bug when mino off screen. Only draw parts of mino on screen.
void drawMino(bool clear){
    uint16_t toDraw = activeMino.mino->shape[activeMino.orientation];
    uint8_t color;
    if (clear) color = BLACK;
    else color = activeMino.mino->color;

    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            if (toDraw >> (15 - (j * 4 + i)) & 1) {
                writePixel(yScreenPos(activeMino.y) + j, xScreenPos(activeMino.x) + i, color);
            }
        }
    }
}


bool createMino(){
    activeMino.mino        = selectNext();
    activeMino.orientation = NORTH;
    activeMino.x           = MINO_SPAWN_POSITION_X;
    activeMino.y           = MINO_SPAWN_POSITION_Y;

    if (canBePlaced(activeMino.y, activeMino.x, *activeMino.mino, activeMino.orientation)){
        drawMino(false);
        return true;
    } else {
        return false;
    }
}

const Mino* selectNext(){
    if (bagIndex >= 7) {
        for (uint8_t i = 0; i < 7; i++) bag[i] = i;
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

uint32_t rngNext(){
    uint32_t x = rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rngState = x;
}

uint8_t xScreenPos(uint8_t x){
    return x - BORDER;
}

uint8_t yScreenPos(uint8_t y){
    return y - (BOARD_HEIGHT - SCREEN_HEIGHT - BORDER);
}

bool inBounds(uint8_t x, uint8_t y){
    if (x >= BORDER & x < BORDER + SCREEN_WIDTH & y < BOARD_HEIGHT - BORDER) {
        return true;
    } else {
        return false;
    }
}
bool onScreen(uint8_t x, uint8_t y){
    if (x >= BORDER & x < BORDER + SCREEN_WIDTH & y < BOARD_HEIGHT - BORDER & y > BOARD_HEIGHT - SCREEN_HEIGHT - BORDER){
        return true;
    } else {
        return false;
    }
}

bool canBePlaced(uint8_t y, uint8_t x, Mino mino, Orientation orientation){
    uint16_t toDraw = activeMino.mino->shape[orientation];
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            if ((toDraw >> (15 - (j * 4 + i)) & 1) & (!inBounds(x + i, y + j) | (gameBoard[y+j][x+i] != 0))) return false;
        }
    }
    return true;
}

void restartGame(){
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
    currentSpeed = 0;
    gravityIntervalMs = speedCurve[currentSpeed];
}

bool lowerMino(){
    if (!canBePlaced(activeMino.y + 1, activeMino.x, *activeMino.mino, activeMino.orientation)){
        return false;
    } else {
        drawMino(true);
        activeMino.y++;
        drawMino(false);
        return true;
    }
}

void setMino(){
    uint16_t toDraw = activeMino.mino->shape[activeMino.orientation];
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            if (toDraw >> (15 - (j * 4 + i)) & 1) {
                gameBoard[j + activeMino.y][i + activeMino.x] = activeMino.mino->color;
            }
        }
    }
    activeMino.mino = NULL;
}

void moveMino(int8_t direction){
    if (canBePlaced(activeMino.y, activeMino.x + direction, *activeMino.mino, activeMino.orientation)) {
        drawMino(true);
        activeMino.x += direction;
        drawMino(false);
    }
}

void rotateMino(void){
    Orientation next = (Orientation)((activeMino.orientation + 1) % 4);
    if (canBePlaced(activeMino.y, activeMino.x, *activeMino.mino, activeMino.orientation + 1)){
        drawMino(true);
        activeMino.orientation = next;
        drawMino(false);
    }
}

void clearRows(){
    bool full = true;
    uint8_t clearCount = 0;
    uint8_t toClear[4];
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < SCREEN_WIDTH; j++){
            if (gameBoard[i + activeMino.y][BORDER + j] == 0) {
                full = false;
                break;
            }
        }
        if (full == true){
            toClear[clearCount] = i + activeMino.y;
            clearCount++;
            // TODO: If this is above the screen this will break. FIX!
            for (int j = 0; j < SCREEN_WIDTH; j++) {
                writePixel(yScreenPos(i + activeMino.y), j, WHITE);
            }
        } else full = true;
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
            lineCount = 0;
            if (currentSpeed < SPEED_LEVELS - 1)
                currentSpeed++;
            gravityIntervalMs = speedCurve[currentSpeed];
        }
    }
}

void reWriteScreen(){
    for (int i = 0 + BOARD_HEIGHT - BORDER - SCREEN_HEIGHT; i < BOARD_HEIGHT - BORDER; i++){
        for (int j = 0 + BORDER; j < SCREEN_WIDTH + BORDER; j++){
            writePixel(yScreenPos(i), xScreenPos(j), gameBoard[i][j]);
        }
    }
}
