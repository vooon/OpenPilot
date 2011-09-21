/*
 * graphicdriver.c
 *
 *  Created on: 16.9.2011
 *      Author: Samba
 */

#include "graphicdriver.h"

// Graphics vars


//static uint8_t pixelData[GRAPHICS_WIDTH][GRAPHICS_HEIGHT];
extern uint16_t frameBuffer[BUFFER_VERT_SIZE][BUFFER_LINE_LENGTH];

extern GPIO_InitTypeDef			GPIO_InitStructure;
extern TIM_TimeBaseInitTypeDef		TIM_TimeBaseStructure;
extern TIM_OCInitTypeDef			TIM_OCInitStructure;
extern NVIC_InitTypeDef			NVIC_InitStructure;
extern SPI_InitTypeDef				SPI_InitStructure;
extern DMA_InitTypeDef				DMA_InitStructure;
extern USART_InitTypeDef			USART_InitStructure;
extern EXTI_InitTypeDef			EXTI_InitStructure;

// Functions

void clearGraphics() {
	for (uint8_t x = 0; x < GRAPHICS_WIDTH; ++x) {
	  for (uint8_t y = 0; y < GRAPHICS_HEIGHT; ++y) {
		  frameBuffer[y][x] = 0x0000;
		}
	}
}

uint8_t validPos(uint8_t x, uint8_t y) {
	if (x >= GRAPHICS_WIDTH_REAL || y >= GRAPHICS_HEIGHT) {
		return 0;
	}
	return 1;
}

void setPixel(uint8_t x, uint8_t y, uint8_t state) {
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
// Some mods done by me ( name, int -> uint8 )
void drawCircle(uint8_t x0, uint8_t y0, uint8_t radius) {
  volatile int8_t f = 1 - radius;
  volatile int8_t ddF_x = 1;
  volatile int8_t ddF_y = -2 * radius;
  volatile int8_t x = 0;
  volatile int8_t y = radius;

  setPixel(x0, y0 + radius, 1);
  setPixel(x0, y0 - radius, 1);
  setPixel(x0 + radius, y0, 1);
  setPixel(x0 - radius, y0, 1);

  while(x < y) {
    // ddF_x == 2 * x + 1;
    // ddF_y == -2 * y;
    // f == x*x + y*y - radius*radius + 2*x - y + 1;
    if(f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    setPixel(x0 + x, y0 + y, 1);
    setPixel(x0 - x, y0 + y, 1);
    setPixel(x0 + x, y0 - y, 1);
    setPixel(x0 - x, y0 - y, 1);
    setPixel(x0 + y, y0 + x, 1);
    setPixel(x0 - y, y0 + x, 1);
    setPixel(x0 + y, y0 - x, 1);
    setPixel(x0 - y, y0 - x, 1);
  }
}

void swap(uint8_t* a, uint8_t* b) {
	uint8_t temp = *a;
	*a = *b;
	*b = temp;
}

void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	if (x0 > x1) {
		swap(&x0, &x1);
		swap(&y0, &y1);
	}
	int8_t deltax = x1 - x0;
	int8_t deltay = abs(y1 - y0);
	int8_t error = deltax / 2;
	int8_t ystep;
	int8_t y = y0;
	if (y0 < y1) {
		ystep = 1;
	}
	else {
		ystep = -1;
	}
	for (uint8_t x = x0; x <= x1; ++x) {
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

static uint16_t angle=0;

void updateGrapics() {
	drawCircle((GRAPHICS_SIZE/2)-1, (GRAPHICS_SIZE/2)-1, (GRAPHICS_SIZE/2)-1);
	//drawCircle((GRAPHICS_SIZE/2)-1, (GRAPHICS_SIZE/2)-1, (GRAPHICS_SIZE/2)-2);
	//drawLine(0, (GRAPHICS_SIZE/2)-1, GRAPHICS_SIZE-1, (GRAPHICS_SIZE/2)-1);
	//drawLine((GRAPHICS_SIZE/2)-1, 0, (GRAPHICS_SIZE/2)-1, GRAPHICS_SIZE-1);
//#ifdef GPS_ENABLED
	angle++;
	int16_t a = myCos(angle);
	int16_t b = mySin(angle);
	a = (a * (GRAPHICS_SIZE / 3)) / 100;
	b = (b * (GRAPHICS_SIZE / 3)) / 100;
	drawLine((GRAPHICS_SIZE/2)-1 - b, (GRAPHICS_SIZE/2)-1 + a, (GRAPHICS_SIZE/2)-1 + b, (GRAPHICS_SIZE/2)-1 - a); //Direction line
	//drawLine((GRAPHICS_SIZE/2)-1 + a/2, (GRAPHICS_SIZE/2)-1 + b/2, (GRAPHICS_SIZE/2)-1 - a/2, (GRAPHICS_SIZE/2)-1 - b/2); //Arrow bottom line
	drawLine((GRAPHICS_SIZE/2)-1 + b, (GRAPHICS_SIZE/2)-1 - a, (GRAPHICS_SIZE/2)-1 - a/2, (GRAPHICS_SIZE/2)-1 - b/2); // Arrow "wings"
	drawLine((GRAPHICS_SIZE/2)-1 + b, (GRAPHICS_SIZE/2)-1 - a, (GRAPHICS_SIZE/2)-1 + a/2, (GRAPHICS_SIZE/2)-1 + b/2);
//#endif //GPS_ENABLED
}



void drawGrapicsLine()
{
	//uint16_t start;

//GPIO_ResetBits(GPIOA,GPIO_Pin_4);
//PIOS_DELAY_WaituS(GRAPHICS_OFFSET);
	PIOS_DELAY_WaituS(GRAPHICS_OFFSET);
	uint16_t currLine = (line - GRAPHICS_LINE);
	//USB_LED_ON;
	//for (uint8_t i = 0; i < GRAPHICS_WIDTH; ++i)
	{
		/*if ((pixelData[i][currLine] != 0)) {
			GPIO_SetBits(GPIOA,GPIO_Pin_4);
		}
		else {
			GPIO_ResetBits(GPIOA,GPIO_Pin_4);
		}*/
		/*for(uint8_t j = 0; j < 8 ; j++)
		{
			if((pixelData[i][currLine]) & (0x80>>j))
				GPIOB->BSRR = GPIO_Pin_15;
			else
				GPIOB->BRR = GPIO_Pin_15;
			//DELAY_10_NOP();
		}*/
		//SPI2->DR = 0xAA;
		DMA_DeInit(DMA1_Channel5);
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)frameBuffer[currLine];
		DMA_InitStructure.DMA_BufferSize = BUFFER_LINE_LENGTH;
		DMA_Init(DMA1_Channel5, &DMA_InitStructure);
		SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
		DMA_Cmd(DMA1_Channel5, ENABLE);

		//GPIO_ResetBits(GPIOA,GPIO_Pin_4);
		//SPI_I2S_SendData(SPI2, pixelData[i][currLine]);
		//DELAY_10_NOP();
		//SPI2->DR = pixelData[i][currLine];
		//start = PIOS_DELAY_TIMER->CNT;

		/* Note that this event works on 16bit counter wrap-arounds */
		//while ((uint16_t) (PIOS_DELAY_TIMER->CNT - start) <= 3) ;
	}
	//USB_LED_OFF;
	//DELAY_1_NOP();
	//SPI2->DR = 0xAA;
	//PIOS_DELAY_WaituS8(10);
	//GPIO_ResetBits(GPIOA,GPIO_Pin_4);

}
