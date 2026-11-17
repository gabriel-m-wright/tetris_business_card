# Tetris Business Card
The goal of this project is to develop a Tetris-like game written in C for the TI-MSPM0C1103 (Arm Cortex-M0+) which is displayed using an RGB LED matrix.
The components to run this game will be placed on a PCB business card, with my information on one side, and the game on the other.

<img width="3072" height="4080" alt="tetris" src="https://github.com/user-attachments/assets/e54155d4-514f-4ae4-a0e3-17f5fe560e86" />
Current prototype.

## Status
At present, the game is functional as a breadboard prototype.

## Features
- [x] LED matrix driver controls
- [x] Basic game logic
- [x] Interrupt-driven gravity tick
- [ ] Custom PCB design
- [ ] Finalize the game software to place gameplay in-line with modern Tetris standards

## Hardware
MCU: TI-MSPM0C1103\
LED Driver Chip: IS31FL3741A

## Software
### Logic
1. Timer interrupt fires every 1ms and increments a counter. Once the counter reaches the game tick period, this sets a flag to lower the block on-screen by 1.
2. Main loop reads interrupt flags from button presses and this timer and calls gameTick.
3. Game logic handles side-to-side movement, collision, and clearing rows.
4. The display driver writes the updated pixels to the matrix over I2C.
### Footprint
| Resource | Used | Available |
| -------- | ---- | --------- |
| Flash | USEDFLASH | 8KB |
| SRAM | USEDSRAM | 1KB |

<!-- TODO: Update used SRAM and Flash-->
