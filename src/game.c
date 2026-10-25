#include "game.h"
#include "IS31FL3741.h"
#include "ti_msp_dl_config.h"
#include "isr.h"

// There will be a 2 unit buffer around every playable edge of the board.
static uint8_t gameBoard[BOARD_HEIGHT][SCREEN_WIDTH + 4];
static ActiveMino currentMino = NONE;
static Orientation currentOrientation;
static uint8_t currentMinoX = MINO_SPAWN_POSITION_X;
static uint8_t currentMinoY = MINO_SPAWN_POSITION_Y;

static uint32_t lineCount = 0;

// Stores the seven upcoming pieces
static uint8_t bag[7];
static uint8_t bagIndex = 7;

static uint32_t rngState = SEED;

const Mino i_block = {
    .color = CYAN,
    .rotationNorth = 0b0000111100000000,
    .rotationEast  = 0b0010001000100010,
    .rotationSouth = 0b0000000011110000,
    .rotationWest  = 0b0100010001000100
};

const Mino s_block = {
    .color = GREEN,
    .rotationNorth = 0b0011011000000000,
    .rotationEast  = 0b0010001100010000,
    .rotationSouth = 0b0000001101100000,
    .rotationWest  = 0b0100011000100000
};

const Mino z_block = {
    .color = RED,
    .rotationNorth = 0b0110001100000000,
    .rotationEast  = 0b0001001100100000,
    .rotationSouth = 0b0000011000110000,
    .rotationWest  = 0b0010011001000000
};

const Mino o_block = {
    .color = YELLOW,
    .rotationNorth = 0b0110011000000000,
    .rotationEast  = 0b0110011000000000,
    .rotationSouth = 0b0110011000000000,
    .rotationWest  = 0b0110011000000000
};

const Mino t_block = {
    .color = PURPLE,
    .rotationNorth = 0b0010011100000000,
    .rotationEast  = 0b0010001100100000,
    .rotationSouth = 0b0000011100100000,
    .rotationWest  = 0b0010011000100000
};

const Mino l_block = {
    .color = ORANGE,
    .rotationNorth = 0b0001011100000000,
    .rotationEast  = 0b0010001000110000,
    .rotationSouth = 0b0000011101000000,
    .rotationWest  = 0b0110001000100000
};

const Mino j_block = {
    .color = BLUE,
    .rotationNorth = 0b0100011100000000,
    .rotationEast  = 0b0011001000100000,
    .rotationSouth = 0b0000011100010000,
    .rotationWest  = 0b0010001001100000
};

void gameTick(void){
    if (currentMino == NONE){
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
void drawMino(uint8_t y, uint8_t x, Mino mino, Orientation orientation, bool clear){
    uint16_t toDraw;
    switch (orientation) {
        case NORTH:
            toDraw = mino.rotationNorth;
            break;
        case EAST:
            toDraw = mino.rotationEast;
            break;
        case SOUTH:
            toDraw = mino.rotationSouth;
            break;
        case WEST:
            toDraw = mino.rotationWest;
            break;
    }
    uint8_t color;
    if (clear) color = BLACK;
    else color = mino.color;

    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            if (toDraw >> (15 - (j * 4 + i)) & 1) {
                writePixel(y + j, x + i, color);
            }
        }
    }
}

Mino minoType(){
    Mino newMino;
    switch (currentMino) {
        case NONE:
        case I_BLOCK:
            newMino = i_block;
            break;
        case S_BLOCK:
            newMino = s_block;
            break;
        case Z_BLOCK:
            newMino = z_block;
            break;
        case O_BLOCK:
            newMino = o_block;
            break;
        case T_BLOCK:
            newMino = t_block;
            break;
        case L_BLOCK:
            newMino = l_block;
            break;
        case J_BLOCK:
            newMino = j_block;
            break;
    }
    return newMino;
}

bool createMino(){
    currentMino = selectNext();
    currentOrientation = NORTH;
    currentMinoX = MINO_SPAWN_POSITION_X;
    currentMinoY = MINO_SPAWN_POSITION_Y;
    Mino newMino = minoType();
    

    if (canBePlaced(currentMinoY, currentMinoX, newMino, currentOrientation)){
        drawMino(yScreenPos(currentMinoY), xScreenPos(currentMinoX), newMino, currentOrientation, 0);
        return true;
    } else {
        return false;
    }
}

ActiveMino selectNext(){
    if (bagIndex >= 7) {
        for (uint8_t i = 0; i < 7; i++) bag[i] = (ActiveMino)(i + 1);
        for (uint8_t i = 6; i > 0; i--){
            uint8_t j = rngNext() % (i + 1);
            ActiveMino t = bag[i]; 
            bag[i] = bag[j]; 
            bag[j] = t;
        }
        bagIndex = 0;
    }
    return bag[bagIndex++];
}

uint32_t rngNext(){
    uint32_t x = rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rngState = x;
}

uint8_t xScreenPos(uint8_t x){
    return x - (BOARD_WIDTH - SCREEN_WIDTH)/2;
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
    uint16_t toDraw;
    switch (orientation) {
        case NORTH:
            toDraw = mino.rotationNorth;
            break;
        case EAST:
            toDraw = mino.rotationEast;
            break;
        case SOUTH:
            toDraw = mino.rotationSouth;
            break;
        case WEST:
            toDraw = mino.rotationWest;
            break;
    }
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
    tickSpeed = 1;
}

bool lowerMino(){
    if (!canBePlaced(currentMinoY + 1, currentMinoX, minoType(), currentOrientation)){
        return false;
    } else {
        drawMino(yScreenPos(currentMinoY), xScreenPos(currentMinoX), minoType(), currentOrientation, true);
        currentMinoY++;
        drawMino(yScreenPos(currentMinoY), xScreenPos(currentMinoX), minoType(), currentOrientation, false);
        return true;
    }
}

void setMino(){
    Mino mino = minoType();
    uint16_t toDraw;
    switch (currentOrientation) {
        case NORTH:
            toDraw = mino.rotationNorth;
            break;
        case EAST:
            toDraw = mino.rotationEast;
            break;
        case SOUTH:
            toDraw = mino.rotationSouth;
            break;
        case WEST:
            toDraw = mino.rotationWest;
            break;
    }

    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            if (toDraw >> (15 - (j * 4 + i)) & 1) {
                gameBoard[j + currentMinoY][i + currentMinoX] = mino.color;
            }
        }
    }
    currentMino = NONE;
}

void moveMino(int8_t direction){
    if (canBePlaced(currentMinoY, currentMinoX + direction, minoType(), currentOrientation)) {
        drawMino(yScreenPos(currentMinoY), xScreenPos(currentMinoX), minoType(), currentOrientation, true);
        currentMinoX += direction;
        drawMino(yScreenPos(currentMinoY), xScreenPos(currentMinoX), minoType(), currentOrientation, false);
    }
}

void rotateMino(void){
    if (canBePlaced(currentMinoY, currentMinoX, minoType(), currentOrientation + 1)){
        drawMino(yScreenPos(currentMinoY), xScreenPos(currentMinoX), minoType(), currentOrientation, true);
        currentOrientation = (currentOrientation + 1) % 4;
        drawMino(yScreenPos(currentMinoY), xScreenPos(currentMinoX), minoType(), currentOrientation, false);
    }
}

void clearRows(){
    bool full = true;
    uint8_t clearCount = 0;
    uint8_t toClear[4];
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < SCREEN_WIDTH; j++){
            if (gameBoard[i + currentMinoY][BORDER + j] == 0) {
                full = false;
                break;
            }
        }
        if (full == true){
            toClear[clearCount] = i + currentMinoY;
            clearCount++;
            // TODO: If this is above the screen this will break. FIX!
            for (int j = 0; j < SCREEN_WIDTH; j++) {
                writePixel(yScreenPos(i + currentMinoY), j, WHITE);
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
        tickSpeed = (uint32_t)((lineCount / LEVEL_UP + 1) * MULTIPLIER_LEVEL);
    }
}

void reWriteScreen(){
    for (int i = 0 + BOARD_HEIGHT - BORDER - SCREEN_HEIGHT; i < BOARD_HEIGHT - BORDER; i++){
        for (int j = 0 + BORDER; j < SCREEN_WIDTH + BORDER; j++){
            writePixel(yScreenPos(i), xScreenPos(j), gameBoard[i][j]);
        }
    }
}