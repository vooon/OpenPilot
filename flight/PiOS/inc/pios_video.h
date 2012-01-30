/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_VIDEO Code for OSD video generator
 * @brief OSD generator, Parts from CL-OSD and SUPEROSD project
 * @{
 *
 * @file       pios_video.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      OSD generator, Parts from CL-OSD and SUPEROSD projects
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef PIOS_VIDEO_H
#define PIOS_VIDEO_H

#include <pios.h>

// *****************************************************************************
#if defined(PIOS_INCLUDE_VIDEO)

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

#define GRAPHICS_LINE 25
#define GRAPHICS_OFFSET 10

//#define GRAPHICS_WIDTH_REAL 336
//#define GRAPHICS_HEIGHT_REAL 272

#define GRAPHICS_WIDTH_REAL 336
#define GRAPHICS_HEIGHT_REAL 272

#define GRAPHICS_WIDTH (GRAPHICS_WIDTH_REAL/16)
#define GRAPHICS_HEIGHT GRAPHICS_HEIGHT_REAL

#define BUFFER_LINE_LENGTH         GRAPHICS_WIDTH  //Yes, in 16 bit halfwords.
#define BUFFER_VERT_SIZE           GRAPHICS_HEIGHT
#define TEXT_LINE_MAX_CHARS 		GRAPHICS_WIDTH_REAL/8

#define LEFT_MARGIN 3
#define TOP_MARGIN 2

// Size of an array (num items.)
#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))


#define DISP_HEIGHT GRAPHICS_HEIGHT
#define DISP_WIDTH GRAPHICS_WIDTH_REAL
#define HUD_VSCALE_FLAG_CLEAR                   1
#define HUD_VSCALE_FLAG_NO_NEGATIVE             2

// Macros for computing addresses and bit positions.
// NOTE: /16 in y is because we are addressing by word not byte.
#define CALC_BUFF_ADDR(x, y)    (((x) / 16) + ((y) * (DISP_WIDTH / 16)))
#define CALC_BIT_IN_WORD(x)             ((x) & 15)
#define DEBUG_DELAY
// Macro for writing a word with a mode (NAND = clear, OR = set, XOR = toggle)
// at a given position
#define WRITE_WORD_MODE(buff, addr, mask, mode) \
        switch(mode) { \
                case 0: buff[addr] &= ~mask; break; \
                case 1: buff[addr] |= mask; break; \
                case 2: buff[addr] ^= mask; break; }

#define WRITE_WORD_NAND(buff, addr, mask) { buff[addr] &= ~mask; DEBUG_DELAY; }
#define WRITE_WORD_OR(buff, addr, mask)   { buff[addr] |= mask; DEBUG_DELAY; }
#define WRITE_WORD_XOR(buff, addr, mask)  { buff[addr] ^= mask; DEBUG_DELAY; }

// Horizontal line calculations.
// Edge cases.
#define COMPUTE_HLINE_EDGE_L_MASK(b) ((1 << (16 - (b))) - 1)
#define COMPUTE_HLINE_EDGE_R_MASK(b) (~((1 << (15 - (b))) - 1))
// This computes an island mask.
#define COMPUTE_HLINE_ISLAND_MASK(b0, b1) (COMPUTE_HLINE_EDGE_L_MASK(b0) ^ COMPUTE_HLINE_EDGE_L_MASK(b1));

// Macro for initializing stroke/fill modes. Add new modes here
// if necessary.
#define SETUP_STROKE_FILL(stroke, fill, mode) \
        stroke = 0; fill = 0; \
        if(mode == 0) { stroke = 0; fill = 1; } \
        if(mode == 1) { stroke = 1; fill = 0; } \

// Line endcaps (for horizontal and vertical lines.)
#define ENDCAP_NONE             0
#define ENDCAP_ROUND    1
#define ENDCAP_FLAT     2

#define DRAW_ENDCAP_HLINE(e, x, y, s, f, l) \
        if((e) == ENDCAP_ROUND) /* single pixel endcap */ \
        { write_pixel_lm(x, y, f, l); } \
        else if((e) == ENDCAP_FLAT) /* flat endcap: FIXME, quicker to draw a vertical line(?) */ \
        { write_pixel_lm(x, y - 1, s, l); write_pixel_lm(x, y, s, l); write_pixel_lm(x, y + 1, s, l); }

#define DRAW_ENDCAP_VLINE(e, x, y, s, f, l) \
        if((e) == ENDCAP_ROUND) /* single pixel endcap */ \
        { write_pixel_lm(x, y, f, l); } \
        else if((e) == ENDCAP_FLAT) /* flat endcap: FIXME, quicker to draw a horizontal line(?) */ \
        { write_pixel_lm(x - 1, y, s, l); write_pixel_lm(x, y, s, l); write_pixel_lm(x + 1, y, s, l); }

// Macros for writing pixels in a midpoint circle algorithm.
#define CIRCLE_PLOT_8(buff, cx, cy, x, y, mode) \
        CIRCLE_PLOT_4(buff, cx, cy, x, y, mode); \
        if((x) != (y)) CIRCLE_PLOT_4(buff, cx, cy, y, x, mode);

#define CIRCLE_PLOT_4(buff, cx, cy, x, y, mode) \
        write_pixel(buff, (cx) + (x), (cy) + (y), mode); \
        write_pixel(buff, (cx) - (x), (cy) + (y), mode); \
        write_pixel(buff, (cx) + (x), (cy) - (y), mode); \
        write_pixel(buff, (cx) - (x), (cy) - (y), mode);



// Font flags.
#define FONT_BOLD               1               // bold text (no outline)
#define FONT_INVERT             2               // invert: border white, inside black

// Text alignments.
#define TEXT_VA_TOP             0
#define TEXT_VA_MIDDLE  1
#define TEXT_VA_BOTTOM  2
#define TEXT_HA_LEFT    0
#define TEXT_HA_CENTER  1
#define TEXT_HA_RIGHT   2

// Text dimension structures.
struct FontDimensions
{
        int width, height;
};


// Max/Min macros.
//#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MAX3(a, b, c)   MAX(a, MAX(b, c))
#define MIN3(a, b, c)   MIN(a, MIN(b, c))

// Check if coordinates are valid. If not, return.
#define CHECK_COORDS(x, y) if(x < 0 || x >= DISP_WIDTH || y < 0 || y >= DISP_HEIGHT) return;
#define CHECK_COORD_X(x) if(x < 0 || x >= DISP_WIDTH) return;
#define CHECK_COORD_Y(y) if(y < 0 || y >= DISP_HEIGHT) return;

// Clip coordinates out of range.
#define CLIP_COORD_X(x) { x = MAX(0, MIN(x, DISP_WIDTH)); }
#define CLIP_COORD_Y(y) { y = MAX(0, MIN(y, DISP_HEIGHT)); }
#define CLIP_COORDS(x, y) { CLIP_COORD_X(x); CLIP_COORD_Y(y); }

// Macro to swap buffers given a temporary pointer.
#define SWAP_BUFFS(tmp, a, b) { tmp = a; a = b; b = tmp; }

// Macro to swap two variables using XOR swap.
#define SWAP(a, b) { a ^= b; b ^= a; a ^= b; }





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
void updateGraphics();
void drawGraphicsLine();

void write_string(char *str, unsigned int x, unsigned int y, unsigned int xs, unsigned int ys, int va, int ha, int flags, int font);


extern volatile uint16_t gActiveLine;
extern volatile uint16_t gActivePixmapLine;

extern volatile uint8_t gUpdateScreenData;
extern volatile uint8_t gLineType;

void initLine();
void updateLine();

void setAttitudeOsd(int16_t pitch, int16_t roll, int16_t yaw);
void setGpsOsd(uint8_t status, int32_t lat, int32_t lon, float alt, float spd);
void updateOnceEveryFrame();
bool isOutOfGraphics(void);

#endif
#endif /* PIOS_VIDEO_H */
