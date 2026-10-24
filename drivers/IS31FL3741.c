#include "ti_msp_dl_config.h"
#include "IS31FL3741.h"

// Maps positions to their logical addresses
const uint8_t ADDRESS_TABLE[SCREEN_HEIGHT][SCREEN_WIDTH] =
{
    {56, 47, 44, 41, 38, 35, 32, 53, 50},
    {55, 46, 43, 40, 37, 34, 31, 52, 49},
    {54, 45, 42, 39, 36, 33, 30, 51, 48},
    {29, 59, 49, 39, 29, 19,  9, 19,  9},
    {28, 58, 48, 38, 28, 18,  8, 18,  8},
    {27, 57, 47, 37, 27, 17,  7, 17,  7},
    {26, 56, 46, 36, 26, 16,  6, 16,  6},
    {25, 55, 45, 35, 25, 15,  5, 15,  5},
    {24, 54, 44, 34, 24, 14,  4, 14,  4},
    {23, 53, 43, 33, 23, 13,  3, 13,  3},
    {22, 52, 42, 32, 22, 12,  2, 12,  2},
    {21, 51, 41, 31, 21, 11,  1, 11,  1},
    {20, 50, 40, 30, 20, 10,  0, 10,  0}
};

// Write a byte to a register
bool IS31FL3741_writeReg(uint8_t reg, uint8_t data){
    uint8_t txBuffer[2] = {reg, data};

    // Wait until the I2C controller is idle
    while (!(DL_I2C_getControllerStatus(I2C0) & DL_I2C_CONTROLLER_STATUS_IDLE));

    // Flush TX FIFO and load in the data
    DL_I2C_flushControllerTXFIFO(I2C0);
    DL_I2C_fillControllerTXFIFO(I2C0, txBuffer, 2);
    DL_I2C_startControllerTransfer(I2C0, IS31FL3741_ADDR, DL_I2C_CONTROLLER_DIRECTION_TX, 2);

    // Wait for transfer completion
    while (!(DL_I2C_getControllerStatus(I2C0) & DL_I2C_CONTROLLER_STATUS_IDLE));

    // Check for bus errors
    if (DL_I2C_getControllerStatus(I2C0) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        DL_I2C_flushControllerTXFIFO(I2C0);
        return false;
    }
    return true;
}

// Unlock the command register and select a page
bool IS31FL3741_selectPage(uint8_t page){
    if (!IS31FL3741_writeReg(COMMAND_REGISTER_LOCK, UNLOCK)) return false;
    if (!IS31FL3741_writeReg(COMMAND_REGISTER, page)) return false;
    return true;
}

// Initialize 
void IS31FL3741_init(void){

    IS31FL3741_selectPage(PAGE_FUNCTION_REGISTER);
    IS31FL3741_writeReg(RESET_REGISTER, RESET);
    IS31FL3741_selectPage(PAGE_FUNCTION_REGISTER);

    // Disables shutdown
    IS31FL3741_writeReg(CONFIGURATION_REGISTER, NORMAL);
    
    // Sets global current
    IS31FL3741_writeReg(GLOBAL_CURRENT_REGISTER, GLOBAL_CURRENT);

    // Initializes all LEDs on screen
    IS31FL3741_selectPage(0x02);
    for(uint16_t i = 0; i < 180; i++) {
        IS31FL3741_writeReg((uint8_t)i, 0xFF);
    }
    IS31FL3741_selectPage(0x03);
    for(uint16_t i = 0; i < 180; i++) {
        IS31FL3741_writeReg((uint8_t)i, 0xFF);
    }
}

// Writes with a color in 0xRRGGBB format.
bool writePixel(uint8_t y, uint8_t x, Color color){
    uint8_t txBuffer[4];
    txBuffer[0] = ADDRESS_TABLE[y][x] * 3;
    // Fill the buffer with each bit
    uint8_t R = color & red;
    uint8_t G = (color & green) << 3;
    uint8_t B = (color & blue) << 6;

    if (y % 2 == 0 && y >= 2){
        // Pixel is BGR
        txBuffer[1] = B;
        txBuffer[2] = G;
        txBuffer[3] = R;
    } else {
        // Pixel is GRB
        txBuffer[1] = G;
        txBuffer[2] = R;
        txBuffer[3] = B;
    }

    // Selects based on the position of the pixel. These numbers are based on the hardware locations of pixels.
    if (x == 0 | x > 6 | y < 3) IS31FL3741_selectPage(PAGE_PWM_2);
    else IS31FL3741_selectPage(PAGE_PWM_1);

    // Wait until the I2C controller is idle
    while (!(DL_I2C_getControllerStatus(I2C0) & DL_I2C_CONTROLLER_STATUS_IDLE));
    // Flush TX FIFO and load in the data
    DL_I2C_flushControllerTXFIFO(I2C0);
    DL_I2C_fillControllerTXFIFO(I2C0, txBuffer, 4);
    DL_I2C_startControllerTransfer(I2C0, IS31FL3741_ADDR, DL_I2C_CONTROLLER_DIRECTION_TX, 4);

    // Wait for transfer completion
    while (!(DL_I2C_getControllerStatus(I2C0) & DL_I2C_CONTROLLER_STATUS_IDLE));

    // Check for bus errors
    if (DL_I2C_getControllerStatus(I2C0) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        DL_I2C_flushControllerTXFIFO(I2C0);
        return false;
    }
    return true;
}
