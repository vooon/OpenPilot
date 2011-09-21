/*
 * videoconf.h
 *
 *  Created on: 15.9.2011
 *      Author: Samba
 */

#ifndef VIDEOCONF_H_
#define VIDEOCONF_H_

#include <pios.h>

#define GRAPICSENABLED
#define TEXT_ENABLED
// *****************************************************************************

// Text
#ifndef TEXT_SMALL_ENABLED
#define TEXT_LINE_MAX_CHARS 40
#else
#define TEXT_LINE_MAX_CHARS 58
#endif //TEXT_SMALL_ENABLED
#define TEXT_CHAR_HEIGHT 8
#define TEXT_LINES 4
#define TEXT_TRIG_LINES_LIST 31, 31+25, 260, 260+25
#define TEXT_INVERTED_OFF 0
#define TEXT_INVERTED_ON 1
#define TEXT_INVERTED_FLIP 2
#ifndef TEXT_SMALL_ENABLED
#define TEXT_SIZE_MULT 2
#else
#define TEXT_SIZE_MULT 1
#endif //TEXT_SMALL_ENABLED


// Graphics
#define GRAPHICS_SIZE 64 // Multiple of 8
#define GRAPHICS_WIDTH_REAL GRAPHICS_SIZE
#define GRAPHICS_WIDTH (GRAPHICS_SIZE/16)
#define GRAPHICS_HEIGHT GRAPHICS_SIZE
#define GRAPHICS_LINE 135
#define GRAPHICS_OFFSET 18

#define BUFFER_LINE_LENGTH         GRAPHICS_WIDTH  //Yes, in 16 bit halfwords.
#define BUFFER_VERT_SIZE           GRAPHICS_HEIGHT
#define LEFT_MARGIN 3
#define TOP_MARGIN 2

// Line triggering
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define LAST_LINE 100 //240+25+16+2

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

#endif /* VIDEOCONF_H_ */
