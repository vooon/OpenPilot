/*
 * pios_video.c
 *
 *  Created on: 2.10.2011
 *      Author: Samba
 */

#include "pios.h"
#if defined(PIOS_INCLUDE_VIDEO)

extern uint16_t frameBuffer[BUFFER_VERT_SIZE][BUFFER_LINE_LENGTH];

extern GPIO_InitTypeDef			GPIO_InitStructure;
extern TIM_TimeBaseInitTypeDef		TIM_TimeBaseStructure;
extern TIM_OCInitTypeDef			TIM_OCInitStructure;
extern NVIC_InitTypeDef			NVIC_InitStructure;
extern SPI_InitTypeDef				SPI_InitStructure;
extern DMA_InitTypeDef				DMA_InitStructure;
extern USART_InitTypeDef			USART_InitStructure;
extern EXTI_InitTypeDef			EXTI_InitStructure;

volatile uint8_t gLineType = LINE_TYPE_UNKNOWN;
volatile uint8_t gUpdateScreenData = 0;

volatile uint16_t gActiveLine = 0;
volatile uint16_t gActivePixmapLine = 0;

TTime time;

uint8_t getCharData(uint16_t charPos) {
	if (charPos >= CHAR_ARRAY_OFFSET && charPos < CHAR_ARRAY_MAX) {
	  return (oem6x8[charPos - CHAR_ARRAY_OFFSET]);
	}
	else {
		return 0x00;
	}
}

// prints text into framebuffer, 8x2
uint8_t printTextFB(uint16_t x, uint16_t y, const char* str) {
	uint8_t length = strlen(str);
	if (x + length >= TEXT_LINE_MAX_CHARS) {
		length = TEXT_LINE_MAX_CHARS;
	}
	for(uint8_t i = 0; i < TEXT_CHAR_HEIGHT; i++)
	{
		uint8_t c=0;
		uint16_t charPos;
		for(int j=0; j<length; j+=2)
		{
			uint16_t word=0;
			charPos = str[j] * TEXT_CHAR_HEIGHT + i;
			word = getCharData(charPos)<<8;
			charPos = str[j+1] * TEXT_CHAR_HEIGHT + i;
			word |= getCharData(charPos);
			frameBuffer[y+i][x + c] = word;
			c++;
		}
	}
	return length;
}

void printTime(uint16_t x, uint16_t y) {
	char temp[9]={0};
	sprintf(temp,"%02d:%02d:%02d",time.hour,time.min,time.sec);
	printTextFB(x,y,temp);
}




// Graphics vars

static unsigned short logo_bits[] = {
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0700,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3fc0, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfff0, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfff0, 0x0001, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0xfff0, 0x0007, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0xfff8, 0x000f, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0xfff8, 0x001f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0xfff8, 0x007f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0xfffc, 0x00ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfffc,
   0x01ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfffc, 0x07ff,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfffe, 0x07ff, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfffe, 0x1fff, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0xfffe, 0x3fff, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0xfffe, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0xffff, 0xffff, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0xffff, 0xffff, 0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0xffff, 0xfff0, 0x000f, 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x7fff,
   0xffc0, 0x001f, 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x3fff, 0xff80,
   0x003f, 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x3fff, 0xfe00, 0x00ff,
   0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x3fff, 0xf800, 0x00ff, 0x0000,
   0x0000, 0x0000, 0x0003, 0xc000, 0x3fff, 0xf000, 0x03ff, 0x0000, 0x0008,
   0xc000, 0x001f, 0xc000, 0x1fff, 0xe000, 0x07ff, 0x0000, 0x00f8, 0xf800,
   0x003f, 0xc000, 0x1fff, 0x8000, 0x0fff, 0x0000, 0x07e0, 0xfe00, 0x00ff,
   0xe000, 0x1fff, 0x0000, 0x3fff, 0x0000, 0xbfc0, 0xffc0, 0x00ff, 0xe000,
   0x1fff, 0x0000, 0x7ffc, 0x0000, 0xff00, 0xfff7, 0x007f, 0xe000, 0x0fff,
   0x0000, 0xfff0, 0x0000, 0xfe00, 0xffff, 0x007f, 0xf000, 0x0fff, 0x0000,
   0xffe0, 0x0001, 0xf800, 0xffff, 0xc07f, 0xf000, 0x0fff, 0x0000, 0xffc0,
   0x0001, 0xf000, 0xffff, 0xc07f, 0xf003, 0x0fff, 0x0000, 0xff00, 0x0003,
   0xc000, 0xffff, 0xe03f, 0xf00f, 0x0fff, 0x0000, 0xfe00, 0x0007, 0x8000,
   0xffff, 0xe03f, 0xf83f, 0x07ff, 0x0000, 0xf800, 0x0007, 0x0000, 0xffff,
   0xe03f, 0xf87f, 0x07ff, 0x0000, 0xf000, 0x000f, 0x0000, 0xfffc, 0xf03f,
   0xe1ff, 0x03ff, 0x0000, 0xf000, 0x000f, 0x0000, 0xfffc, 0xf07f, 0x87ff,
   0x03ff, 0x0000, 0xf000, 0x0007, 0x0000, 0xfffc, 0xf0ff, 0x1fff, 0x03ff,
   0x0000, 0xe000, 0x000f, 0x0000, 0xfff8, 0xf0ff, 0x3fff, 0x03f8, 0x0000,
   0xe000, 0x001f, 0x0000, 0xfff0, 0xf07f, 0xffff, 0x01e0, 0x0000, 0xe000,
   0x000f, 0x0000, 0xfff0, 0xf87f, 0xffff, 0x03c7, 0x0000, 0xe000, 0x000f,
   0x0000, 0xfff0, 0xf87f, 0xffff, 0x010f, 0x0000, 0xe000, 0x001f, 0x0000,
   0xffe0, 0xf87f, 0xffff, 0x003f, 0x0000, 0xe000, 0x000f, 0x0000, 0xffe0,
   0xfc7f, 0xffff, 0x00ff, 0x0000, 0xe000, 0x000f, 0x0000, 0xffc0, 0xfc0f,
   0xffff, 0x01ff, 0x0000, 0xf000, 0x000f, 0x0000, 0xffc0, 0xfc07, 0xffff,
   0x0fff, 0x0000, 0xf000, 0x001f, 0x0000, 0xffc0, 0xfc07, 0xffff, 0x3fff,
   0x0000, 0xf000, 0x000f, 0x0000, 0xff80, 0xfe07, 0xffff, 0xffff, 0x0000,
   0xf000, 0x000f, 0x0000, 0xff80, 0xfc03, 0xffff, 0xffff, 0x0001, 0xf000,
   0x000f, 0x0000, 0xff00, 0xfe03, 0xffff, 0xffff, 0x0007, 0xf000, 0x001f,
   0x0000, 0xff00, 0xfe03, 0xffff, 0xffff, 0x001f, 0xf000, 0x000f, 0x0000,
   0xfc00, 0xfe01, 0xffff, 0xffff, 0x007f, 0xf000, 0x000f, 0x0000, 0xf000,
   0xff03, 0xffff, 0xffff, 0x01ff, 0xf000, 0x000f, 0x0000, 0x8000, 0xff00,
   0xffff, 0xffff, 0x03ff, 0xf000, 0x001f, 0x0000, 0x0000, 0xff00, 0xffff,
   0xffff, 0x0fff, 0xf000, 0x000f, 0x0000, 0x0000, 0xff80, 0xffff, 0xffff,
   0x3fff, 0xf000, 0x000f, 0x0000, 0x0000, 0xfc00, 0xffff, 0xffff, 0xffff,
   0xf800, 0x000f, 0x0000, 0x0000, 0xf000, 0xffff, 0xffff, 0xffff, 0xf003,
   0x001f, 0x0000, 0x0000, 0x8000, 0xffff, 0xffff, 0xffff, 0xf007, 0x000f,
   0x0000, 0x0000, 0x0000, 0xfffc, 0xffff, 0xffff, 0xf83f, 0x000f, 0x0000,
   0x0000, 0x0000, 0xfff0, 0xffff, 0xffff, 0xf87f, 0x000f, 0x0000, 0x0000,
   0x0000, 0xff00, 0xffff, 0xffff, 0xffff, 0x001f, 0x0000, 0x0000, 0x0000,
   0xf800, 0xffff, 0xffff, 0xffff, 0x000f, 0x0000, 0x0000, 0x0000, 0xe000,
   0xffff, 0xffff, 0xffff, 0x000f, 0x0000, 0x0000, 0x0000, 0x8000, 0xfffe,
   0xffff, 0xffff, 0x000f, 0x0000, 0x0000, 0x0000, 0x0000, 0xfff8, 0xffff,
   0xffff, 0x001f, 0x0000, 0x0000, 0x0000, 0x002e, 0xffe0, 0xffff, 0xffff,
   0x000f, 0x0000, 0x0000, 0x0000, 0x00fc, 0xff00, 0xffff, 0xffff, 0x000f,
   0x0000, 0x0000, 0x0000, 0x07fe, 0xf800, 0xffff, 0xffff, 0x000f, 0x0000,
   0x0000, 0x0000, 0x5ffe, 0xc000, 0xffff, 0xffff, 0x001f, 0x0000, 0x0000,
   0x0000, 0xffff, 0x0001, 0xffff, 0xffff, 0x000f, 0x0000, 0x0000, 0x0000,
   0xffff, 0x0001, 0xfff8, 0xffff, 0x000f, 0x0000, 0x0000, 0x0000, 0xffff,
   0x0001, 0xffc0, 0xffff, 0x000f, 0x0000, 0x0000, 0x8000, 0xffff, 0x0001,
   0xfe00, 0xffff, 0x000f, 0x0000, 0x0000, 0x8000, 0xffff, 0x0000, 0xf800,
   0xffff, 0x0007, 0x0000, 0x0000, 0x8000, 0xffff, 0x0000, 0x8000, 0xffff,
   0x0007, 0x0000, 0x0000, 0x8000, 0xffff, 0x0000, 0x0000, 0xfffe, 0x0003,
   0x0000, 0x0000, 0xc000, 0x7fff, 0x0000, 0x0000, 0xfff0, 0x0003, 0x0000,
   0x0000, 0xc000, 0x7fff, 0x0000, 0x0000, 0xff80, 0x0001, 0x0000, 0x0000,
   0xc000, 0x7fff, 0x0000, 0x0000, 0xfe00, 0x0000, 0x0000, 0x0000, 0xe000,
   0x7fff, 0x0000, 0x0000, 0x1000, 0x0000, 0x0000, 0x0000, 0xe000, 0x7fff,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xe000, 0x3fff, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf000, 0x3fff, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0xf000, 0x3fff, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0xf000, 0x1fff, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0xc000, 0x1fff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x8000, 0x1fff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0ffe, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0ff8, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0fe0,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0fc0, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0f00, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8800, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x9c00, 0x0001, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0xc800, 0x0001, 0x0060, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x8000, 0x0001, 0x0060, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0xc000, 0x0001, 0x07f0, 0xf800, 0xf07f, 0xf07f, 0xf07f, 0xf07f,
   0x8c7f, 0xffc1, 0x07f1, 0xf800, 0xf87f, 0xf87f, 0xf8ff, 0xf8ff, 0xccff,
   0xffc1, 0x00e3, 0x1c00, 0x1ce0, 0x1ce0, 0x1ce0, 0x3ce0, 0x8ce0, 0x00e1,
   0x0067, 0x0c00, 0x0cc0, 0x0cc0, 0x1cc0, 0x1dc0, 0xcdc0, 0x0061, 0x0066,
   0x0c00, 0x0cc0, 0xfdc0, 0x0dff, 0x0dc0, 0x8dc0, 0x0071, 0x0066, 0x0c00,
   0x0cc0, 0xfdc0, 0x0dff, 0x0dc0, 0xcdc0, 0x0071, 0x0066, 0x1c00, 0x1cc0,
   0x1cc0, 0x0c00, 0x0dc0, 0x8dc0, 0x0061, 0x00e6, 0x1c00, 0x0ce8, 0x5ce8,
   0x1c55, 0x1dc0, 0xcce8, 0x20e1, 0x00c7, 0xf800, 0xffff, 0xf8ff, 0x0c7f,
   0xffc0, 0x8cff, 0xffe7, 0x07c3, 0xf000, 0xffff, 0xf07f, 0x0c7f, 0xffc0,
   0x0c7f, 0xff87, 0x0781, 0x0000, 0x0c00, 0x0000, 0x0c00, 0xdf40, 0x000d,
   0x0000, 0x0000, 0x0000, 0x0c00, 0x0000, 0x0000, 0x0c00, 0x0000, 0x0000,
   0x0000, 0x0000, 0x1c00, 0x0000, 0x0000, 0x0c00, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0c00, 0x0000, 0x0000, 0x1c00, 0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };




// Functions

uint16_t mirror(uint16_t source)
{
	int result = ((source & 0x8000) >> 15) | ((source & 0x4000) >> 13) |
			((source & 0x2000) >> 11) | ((source & 0x1000) >> 9) |
			((source & 0x0800) >> 7) | ((source & 0x0400) >> 5) |
			((source & 0x0200) >> 3) | ((source & 0x0100) >> 1) |
			((source & 0x0080) << 1) | ((source & 0x0040) << 3) |
			((source & 0x0020) << 5) | ((source & 0x0010) << 7) |
			((source & 0x0008) << 9) | ((source & 0x0004) << 11) |
			((source & 0x0002) << 13) | ((source & 0x0001) << 15);

	   return result;
}

void clearGraphics() {
	for (uint16_t x = 0; x < GRAPHICS_WIDTH; ++x) {
	  for (uint16_t y = 0; y < GRAPHICS_HEIGHT; ++y) {
		  frameBuffer[y][x] = 0x0000;
		}
	}
}

void copyimage(uint16_t offsetx, uint16_t offsety) {
	offsetx=offsetx/16;
	int i=0;
	  for (uint16_t y = offsety; y < (128+offsety); y++) {
		  for (uint16_t x = offsetx; x < (8+offsetx); x++) {
			  frameBuffer[y][x] = mirror(logo_bits[(y-offsety)*8+(x-offsetx)]);
			  i+=2;
		  }
	}
}

uint8_t validPos(uint16_t x, uint16_t y) {
	if (x >= GRAPHICS_WIDTH_REAL || y >= GRAPHICS_HEIGHT) {
		return 0;
	}
	return 1;
}

void setPixel(uint16_t x, uint16_t y, uint8_t state) {
	if (!validPos(x, y)) {
		return;
	}
	uint8_t bitPos = 15-(x%16);
	uint16_t temp = frameBuffer[y][x/16];
	if (state == 0) {
		temp &= ~(1<<bitPos);
	}
	else if (state == 1) {
		temp |= (1<<bitPos);
	}
	else {
		temp ^= (1<<bitPos);
	}
	frameBuffer[y][x/16] = temp;
}

// Credit for this one goes to wikipedia! :-)
void drawCircle(uint16_t x0, uint16_t y0, uint16_t radius) {
	  int f = 1 - radius;
	  int ddF_x = 1;
	  int ddF_y = -2 * radius;
	  int x = 0;
	  int y = radius;

	  setPixel(x0, y0 + radius,1);
	  setPixel(x0, y0 - radius,1);
	  setPixel(x0 + radius, y0,1);
	  setPixel(x0 - radius, y0,1);

	  while(x < y)
	  {
	    // ddF_x == 2 * x + 1;
	    // ddF_y == -2 * y;
	    // f == x*x + y*y - radius*radius + 2*x - y + 1;
	    if(f >= 0)
	    {
	      y--;
	      ddF_y += 2;
	      f += ddF_y;
	    }
	    x++;
	    ddF_x += 2;
	    f += ddF_x;
	    setPixel(x0 + x, y0 + y,1);
	    setPixel(x0 - x, y0 + y,1);
	    setPixel(x0 + x, y0 - y,1);
	    setPixel(x0 - x, y0 - y,1);
	    setPixel(x0 + y, y0 + x,1);
	    setPixel(x0 - y, y0 + x,1);
	    setPixel(x0 + y, y0 - x,1);
	    setPixel(x0 - y, y0 - x,1);
	  }
}

void swap(uint16_t* a, uint16_t* b) {
	uint16_t temp = *a;
	*a = *b;
	*b = temp;
}

void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	if (x0 > x1) {
		swap(&x0, &x1);
		swap(&y0, &y1);
	}
	int16_t deltax = x1 - x0;
	int16_t deltay = abs(y1 - y0);
	int16_t error = deltax / 2;
	int16_t ystep;
	int16_t y = y0;
	if (y0 < y1) {
		ystep = 1;
	}
	else {
		ystep = -1;
	}
	for (uint16_t x = x0; x <= x1; ++x) {
		if (steep) {
			setPixel(y, x, 1);
		}
		else {
			setPixel(x, y, 1);
		}
		error = error - deltay;
		if (error < 0) {
			y = y + ystep;
			error = error + deltax;
		}
	}
}

const static int8_t sinData[91] = {
  0, 2, 3, 5, 7, 9, 10, 12, 14, 16, 17, 19, 21, 22, 24, 26, 28, 29, 31, 33,
  34, 36, 37, 39, 41, 42, 44, 45, 47, 48, 50, 52, 53, 54, 56, 57, 59, 60, 62,
  63, 64, 66, 67, 68, 69, 71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 82, 83, 84,
  85, 86, 87, 87, 88, 89, 90, 91, 91, 92, 93, 93, 94, 95, 95, 96, 96, 97, 97,
  97, 98, 98, 98, 99, 99, 99, 99, 100, 100, 100, 100, 100, 100};

static int8_t mySin(uint16_t angle) {
	uint16_t pos = 0;
	pos = angle % 360;
	int8_t mult = 1;
	// 180-359 is same as 0-179 but negative.
	if (pos >= 180) {
		pos = pos - 180;
		mult = -1;
	}
	// 0-89 is equal to 90-179 except backwards.
	if (pos >= 90) {
		pos = 180 - pos;
	}
	return mult * (int8_t)(sinData[pos]);
}

static int8_t myCos(uint16_t angle) {
	return mySin(angle + 90);
}

//fill the framebuffer with junk, used for debugging
void fillFrameBuffer(void)
{
	uint16_t i;
	uint16_t j;
	for(i = 0; i<(BUFFER_VERT_SIZE); i++)
	{
		for(j = 0; j<(BUFFER_LINE_LENGTH); j++)
		{
			if(j < LEFT_MARGIN)
			{
				frameBuffer[i][j] = 0;
			} else if (j > 32){
				frameBuffer[i][j] = 0;
			} else {
				frameBuffer[i][j] = i*j;
			}
		}
	}
}


/// Draws four points relative to the given center point.
///
/// \li centerX + X, centerY + Y
/// \li centerX + X, centerY - Y
/// \li centerX - X, centerY + Y
/// \li centerX - X, centerY - Y
///
/// \param centerX the x coordinate of the center point
/// \param centerY the y coordinate of the center point
/// \param deltaX the difference between the centerX coordinate and each pixel drawn
/// \param deltaY the difference between the centerY coordinate and each pixel drawn
/// \param color the color to draw the pixels with.
inline void plotFourQuadrants(int32_t centerX, int32_t centerY, int32_t deltaX, int32_t deltaY)
{
    setPixel(centerX + deltaX, centerY + deltaY,1);      // Ist      Quadrant
    setPixel(centerX - deltaX, centerY + deltaY,1);      // IInd     Quadrant
    setPixel(centerX - deltaX, centerY - deltaY,1);      // IIIrd    Quadrant
    setPixel(centerX + deltaX, centerY - deltaY,1);      // IVth     Quadrant
}

/// Implements the midpoint ellipse drawing algorithm which is a bresenham
/// style DDF.
///
/// \param centerX the x coordinate of the center of the ellipse
/// \param centerY the y coordinate of the center of the ellipse
/// \param horizontalRadius the horizontal radius of the ellipse
/// \param verticalRadius the vertical radius of the ellipse
/// \param color the color of the ellipse border
void ellipse(int centerX, int centerY, int horizontalRadius, int verticalRadius)
{
    int64_t doubleHorizontalRadius = horizontalRadius * horizontalRadius;
    int64_t doubleVerticalRadius = verticalRadius * verticalRadius;

    int64_t error = doubleVerticalRadius - doubleHorizontalRadius * verticalRadius + (doubleVerticalRadius >> 2);

    int x = 0;
    int y = verticalRadius;
    int deltaX = 0;
    int deltaY = (doubleHorizontalRadius << 1) * y;

    plotFourQuadrants(centerX, centerY, x, y);

    while(deltaY >= deltaX)
    {
          x++;
          deltaX += (doubleVerticalRadius << 1);

          error +=  deltaX + doubleVerticalRadius;

          if(error >= 0)
          {
               y--;
               deltaY -= (doubleHorizontalRadius << 1);

               error -= deltaY;
          }
          plotFourQuadrants(centerX, centerY, x, y);
    }

    error = (int64_t)(doubleVerticalRadius * (x + 1 / 2.0) * (x + 1 / 2.0) + doubleHorizontalRadius * (y - 1) * (y - 1) - doubleHorizontalRadius * doubleVerticalRadius);

    while (y>=0)
    {
          error += doubleHorizontalRadius;
          y--;
          deltaY -= (doubleHorizontalRadius<<1);
          error -= deltaY;

          if(error <= 0)
          {
               x++;
               deltaX += (doubleVerticalRadius << 1);
               error += deltaX;
          }

          plotFourQuadrants(centerX, centerY, x, y);
    }
}


void drawArrow(uint16_t x, uint16_t y, uint16_t angle, uint16_t size)
{
	int16_t a = myCos(angle);
	int16_t b = mySin(angle);
	a = (a * (size/2)) / 100;
	b = (b * (size/2)) / 100;
	drawLine((x)-1 - b, (y)-1 + a, (x)-1 + b, (y)-1 - a); //Direction line
	//drawLine((GRAPHICS_SIZE/2)-1 + a/2, (GRAPHICS_SIZE/2)-1 + b/2, (GRAPHICS_SIZE/2)-1 - a/2, (GRAPHICS_SIZE/2)-1 - b/2); //Arrow bottom line
	drawLine((x)-1 + b, (y)-1 - a, (x)-1 - a/2, (y)-1 - b/2); // Arrow "wings"
	drawLine((x)-1 + b, (y)-1 - a, (x)-1 + a/2, (y)-1 + b/2);
}

void drawBox(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	drawLine(x1, y1, x2, y1); //top
	drawLine(x1, y1, x1, y2); //left
	drawLine(x2, y1, x2, y2); //right
	drawLine(x1, y2, x2, y2); //bottom
}


void drawAttitude(uint16_t x, uint16_t y, int16_t pitch, int16_t roll, uint16_t size)
{
	int16_t a = mySin(roll+360);
	int16_t b = myCos(roll+360);
	int16_t c = mySin(roll+90+360)*5/100;
	int16_t d = myCos(roll+90+360)*5/100;

	int16_t k;
	int16_t l;

	int16_t indi30x1=myCos(30)*(size/2+1) / 100;
	int16_t indi30y1=mySin(30)*(size/2+1) / 100;

	int16_t indi30x2=myCos(30)*(size/2+4) / 100;
	int16_t indi30y2=mySin(30)*(size/2+4) / 100;

	int16_t indi60x1=myCos(60)*(size/2+1) / 100;
	int16_t indi60y1=mySin(60)*(size/2+1) / 100;

	int16_t indi60x2=myCos(60)*(size/2+4) / 100;
	int16_t indi60y2=mySin(60)*(size/2+4) / 100;

	pitch=pitch%90;
	if(pitch>90)
	{
		pitch=pitch-90;
	}
	if(pitch<-90)
	{
		pitch=pitch+90;
	}
	a = (a * (size/2)) / 100;
	b = (b * (size/2)) / 100;

	if(roll<-90 || roll>90)
		pitch=pitch*-1;
	k = a*pitch/90;
	l = b*pitch/90;

	// scale
	//0
	drawLine((x)-1-(size/2+4), (y)-1, (x)-1 - (size/2+1), (y)-1);
	drawLine((x)-1+(size/2+4), (y)-1, (x)-1 + (size/2+1), (y)-1);
	//30
	drawLine((x)-1+indi30x1, (y)-1-indi30y1, (x)-1 + indi30x2, (y)-1 - indi30y2);
	drawLine((x)-1-indi30x1, (y)-1-indi30y1, (x)-1 - indi30x2, (y)-1 - indi30y2);
	//60
	drawLine((x)-1+indi60x1, (y)-1-indi60y1, (x)-1 + indi60x2, (y)-1 - indi60y2);
	drawLine((x)-1-indi60x1, (y)-1-indi60y1, (x)-1 - indi60x2, (y)-1 - indi60y2);
	//90
	drawLine((x)-1, (y)-1-(size/2+4), (x)-1, (y)-1 - (size/2+1));


	//roll
	drawLine((x)-1 - b, (y)-1 - a, (x)-1 + b, (y)-1 + a); //Direction line
	//"wingtips"
	drawLine((x)-1 - b, (y)-1 - a, (x)-1 - b - d, (y)-1 - a - c);
	drawLine((x)-1 + b - d, (y)-1 + a - c, (x)-1 + b, (y)-1 + a);

	//pitch
	drawLine((x)-1, (y)-1, (x)-1 + k, (y)-1 - l);


	drawCircle(x-1, y-1, 5);
	drawCircle(x-1, y-1, size/2+4);
}


static uint16_t angleA=0;
static int16_t angleB=90;
static int16_t angleC=0;
static int16_t sum=2;

static int16_t m_pitch=0;
static int16_t m_roll=0;

void setAttitudeOsd(int16_t pitch, int16_t roll)
{
	m_pitch=pitch;
	m_roll=roll;
}


void introText(){
	printTextFB((GRAPHICS_WIDTH_REAL/2 - 40)/16,GRAPHICS_HEIGHT_REAL-10,"ver 0.1");
}

void introGraphics() {
	/* logo */
	copyimage(GRAPHICS_WIDTH_REAL/2-128/2, GRAPHICS_HEIGHT_REAL/2-128/2);

	/* frame */
	drawBox(0,0,GRAPHICS_WIDTH_REAL-2,GRAPHICS_HEIGHT_REAL-1);
}

void updateGrapics() {
	//drawLine(GRAPHICS_WIDTH_REAL-1, GRAPHICS_HEIGHT_REAL-1,(GRAPHICS_WIDTH_REAL/2)-1, GRAPHICS_HEIGHT_REAL-1 );
	//drawCircle((GRAPHICS_WIDTH_REAL/2)-1, (GRAPHICS_HEIGHT_REAL/2)-1, (GRAPHICS_HEIGHT_REAL/2)-1);
	//drawCircle((GRAPHICS_SIZE/2)-1, (GRAPHICS_SIZE/2)-1, (GRAPHICS_SIZE/2)-2);
	//drawLine(0, (GRAPHICS_SIZE/2)-1, GRAPHICS_SIZE-1, (GRAPHICS_SIZE/2)-1);
	//drawLine((GRAPHICS_SIZE/2)-1, 0, (GRAPHICS_SIZE/2)-1, GRAPHICS_SIZE-1);
	angleA++;
	if(angleB<=-90)
	{
	sum=2;
	}
	if(angleB>=90)
	{
	sum=-2;
	}
	angleB+=sum;
	angleC+=2;
	drawArrow(32,GRAPHICS_HEIGHT_REAL/2,angleA,32);
	//drawAttitude(96,GRAPHICS_HEIGHT_REAL/2,90,0,48);
	drawAttitude(GRAPHICS_WIDTH_REAL/2,GRAPHICS_HEIGHT_REAL/2,m_pitch,m_roll,96);
	printTextFB(2,12,"Hello OP-OSD");
	printTextFB(1,21,"Hello OP-OSD");
	printTextFB(0,2,"Hello OP-OSD");
	printTime(10,2);
	printTime((GRAPHICS_WIDTH_REAL/2 - 40)/16,GRAPHICS_HEIGHT_REAL-10);

	drawBox(0,0,GRAPHICS_WIDTH_REAL-2,GRAPHICS_HEIGHT_REAL-1);


	//drawArrow(96,GRAPHICS_HEIGHT_REAL/2,angleB,32);
	//ellipse(50,50,50,30);
}


void initLine() {
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable DRDY GPIO clock */
	RCC_APB2PeriphClockCmd(PIOS_VIDEO_SYNC_CLK | RCC_APB2Periph_AFIO, ENABLE);

	/* Configure EOC pin as input floating */
	GPIO_InitStructure.GPIO_Pin = PIOS_VIDEO_SYNC_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(PIOS_VIDEO_SYNC_GPIO_PORT, &GPIO_InitStructure);

	/* Configure the End Of Conversion (EOC) interrupt */
	GPIO_EXTILineConfig(PIOS_VIDEO_SYNC_PORT_SOURCE, PIOS_VIDEO_SYNC_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = PIOS_VIDEO_SYNC_EXTI_LINE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EOC EXTI Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannel = PIOS_VIDEO_SYNC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PIOS_VIDEO_SYNC_PRIO;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_4);

}

void updateLine() {
	uint16_t currLine = gActivePixmapLine;
	PIOS_DELAY_WaituS(5); // wait 5us to see if H or V sync

	if(((PIOS_VIDEO_SYNC_GPIO_PORT->IDR & PIOS_VIDEO_SYNC_GPIO_PIN))) { // H sync
		if (gActiveLine != 0) {
			if(gLineType == LINE_TYPE_GRAPHICS)
			{
					DMA_DeInit(DMA1_Channel5);
					DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)frameBuffer[currLine];
					DMA_InitStructure.DMA_BufferSize = BUFFER_LINE_LENGTH;
					DMA_Init(DMA1_Channel5, &DMA_InitStructure);
					SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
					DMA_Cmd(DMA1_Channel5, ENABLE);
			}
		}

		// We save some time in beginning of line by pre-calculating next type.
		gLineType = LINE_TYPE_UNKNOWN; // Default case
		gActiveLine++;
		if (gActiveLine == UPDATE_LINE) {
			gUpdateScreenData = 1; // trigger frame update
		}
		else if ((gActiveLine >= GRAPHICS_LINE) && (gActiveLine < (GRAPHICS_LINE + GRAPHICS_HEIGHT))) {
			gLineType = LINE_TYPE_GRAPHICS;
			gActivePixmapLine = (gActiveLine - GRAPHICS_LINE);
		}
	}
	else { // V sync
		if(gActiveLine > 200) {
			gActiveLine = 0;
		}
		//USB_LED_TOGGLE;
	}
}

void updateOnceEveryFrame() {
	if(gActiveLine>=UPDATE_LINE || gActiveLine<GRAPHICS_LINE)
	{
		clearGraphics();
		updateGrapics();
	}
}

void PIOS_Video_IRQHandler(void)
{
	PIOS_IRQ_Disable();
	updateLine();
	PIOS_IRQ_Enable();
}

#endif

