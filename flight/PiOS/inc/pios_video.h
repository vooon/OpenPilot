/*
 * videoconf.h
 *
 *  Created on: 15.9.2011
 *      Author: Samba
 */

#ifndef VIDEOCONF_H_
#define VIDEOCONF_H_

#include <pios.h>

// *****************************************************************************

// Text
#define TEXT_CHAR_HEIGHT 8
#define TEXT_INVERTED_OFF 0
#define TEXT_INVERTED_ON 1
#define TEXT_INVERTED_FLIP 2
//#define TEXT_SIZE_MULT 2



// Graphics
//#define GRAPHICS_SIZE 64 // Multiple of 8
//#define GRAPHICS_LINE 135
//#define GRAPHICS_OFFSET 18

#define GRAPHICS_LINE 30
#define GRAPHICS_OFFSET 10

#define GRAPHICS_WIDTH_REAL 336
#define GRAPHICS_WIDTH (GRAPHICS_WIDTH_REAL/16)
#define GRAPHICS_HEIGHT_REAL 272
#define GRAPHICS_HEIGHT GRAPHICS_HEIGHT_REAL

#define BUFFER_LINE_LENGTH         GRAPHICS_WIDTH  //Yes, in 16 bit halfwords.
#define BUFFER_VERT_SIZE           GRAPHICS_HEIGHT
#define TEXT_LINE_MAX_CHARS 		GRAPHICS_WIDTH_REAL/8

#define LEFT_MARGIN 3
#define TOP_MARGIN 2

// Line triggering
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define LAST_LINE 312 //625/2 //PAL
#define UPDATE_LINE GRAPHICS_LINE+GRAPHICS_HEIGHT_REAL+1
//#define LAST_LINE 525/2 //NTSC


#define LINE_TYPE_UNKNOWN 0
#define LINE_TYPE_TEXT 1
#define LINE_TYPE_GRAPHICS 2

// Global vars

#define DELAY_1_NOP() asm("nop\r\n")
#define DELAY_2_NOP() asm("nop\r\nnop\r\n")
#define DELAY_3_NOP() asm("nop\r\nnop\r\nnop\r\n")
#define DELAY_4_NOP() asm("nop\r\nnop\r\nnop\r\nnop\r\n")
#define DELAY_5_NOP() asm("nop\r\nnop\r\nnop\r\nnop\r\nnop\r\n")
#define DELAY_6_NOP() asm("nop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\n")
#define DELAY_7_NOP() asm("nop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\n")
#define DELAY_8_NOP() asm("nop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\n")
#define DELAY_9_NOP() asm("nop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\n")
#define DELAY_10_NOP() asm("nop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\nnop\r\n")

// Time vars
typedef struct {
  uint8_t sec;
  uint8_t min;
  uint8_t hour;
} TTime;

extern TTime time;


#include "xconvert.h"
#include "oem6x8.h"


uint8_t getCharData(uint16_t charPos);
void introText();

void clearGraphics();
uint8_t validPos(uint16_t x, uint16_t y);
void setPixel(uint16_t x, uint16_t y, uint8_t state);
void drawCircle(uint16_t x0, uint16_t y0, uint16_t radius);
void swap(uint16_t* a, uint16_t* b);
void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void drawBox(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void drawArrow(uint16_t x, uint16_t y, uint16_t angle, uint16_t size);
void drawAttitude(uint16_t x, uint16_t y, int16_t pitch, int16_t roll, uint16_t size);
void introGraphics();
void updateGrapics();
void drawGrapicsLine();

extern volatile uint16_t gActiveLine;
extern volatile uint16_t gActivePixmapLine;

extern volatile uint8_t gUpdateScreenData;
extern volatile uint8_t gLineType;

void initLine();
void updateLine();

void setAttitudeOsd(int16_t pitch, int16_t roll);
void updateOnceEveryFrame();

#endif /* VIDEOCONF_H_ */
