//Richard Farr (rfarr6)
//My custom GBA library

#ifndef MYLIB_H
#define MYLIB_H

#include "fixedmath.h"
#include <stdlib.h>

#define RGB(r, g, b) ((r) | ((g) << 5) | ((b) << 10))
#define OFFSET(r, c, w) ((r)*(w) + (c))
#define REG_DISPCNT (*(u16*)0x4000000)
#define BG2 0x0400
#define MODE3 0x0003
#define MODE4 0x0004
#define MODE5 0x0005

/* 
* Note --> the order that we put the typdef and the defines does not matter
* because the preprocessor will first replace everything with u16. Then,
* at compile-time, those u16's become unsigned short.
*/

typedef unsigned short u16;
extern u16* const videoBuffer;

#define WHITE RGB(31, 31, 31)
#define BLACK RGB(0, 0, 0)
#define RED RGB(31, 0, 0)
#define GREEN RGB(0, 31, 0)
#define BLUE RGB(0, 0, 31)

#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

#define REG_BUTTONS (*(u16*)0x4000130)
#define KEY_PRESSED(k) (~(REG_BUTTONS) & (k))

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#define SCANCOUNT (*(volatile u16*)0x4000006)

void wait_for_vblank();

void setPixel3(int, int, u16);
void drawRect3(int, int, int, int, u16);
void drawHollowRect3(int, int, int, int, u16);
void drawTriangle3(int, int, int, u16);
void drawImage3(int, int, int, int, const u16*);
void drawImageDelayed3(int, int, int, int, const u16*, int);
void drawString3(int, int, const char*, u16, u16);
void drawChar3(int, int, char, u16, u16);
void rotateImage3(int, int, const u16*, u16*, u16);
void mirrorImage3(int, int, const u16*, u16*, char);

#define GBA_RAND(a, b) (FIXED2INT(FIXED_MULT((rand() >> 15), INT2FIXED((b)-(a)+1))) + (a))

#endif









