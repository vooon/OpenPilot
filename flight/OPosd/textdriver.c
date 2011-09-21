/*
 * textdriver.c
 *
 *  Created on: 15.9.2011
 *      Author: Samba
 */
#include <pios.h>
#include "textdriver.h"
#include "videoconf.h"

uint16_t const textLines[TEXT_LINES] = {TEXT_TRIG_LINES_LIST};
static char text[TEXT_LINES][TEXT_LINE_MAX_CHARS];
static uint16_t textPixmap[TEXT_LINE_MAX_CHARS*TEXT_CHAR_HEIGHT];
//static uint16_t textPixmap[TEXT_CHAR_HEIGHT][TEXT_LINE_MAX_CHARS/2];
#ifdef TEXT_INVERTED_ENABLED
static uint8_t textInverted[TEXT_LINES][TEXT_LINE_MAX_CHARS/8];
#endif // TEXT_INVERTED_ENABLED

extern GPIO_InitTypeDef			GPIO_InitStructure;
extern TIM_TimeBaseInitTypeDef		TIM_TimeBaseStructure;
extern TIM_OCInitTypeDef			TIM_OCInitStructure;
extern NVIC_InitTypeDef			NVIC_InitStructure;
extern SPI_InitTypeDef				SPI_InitStructure;
extern DMA_InitTypeDef				DMA_InitStructure;
extern USART_InitTypeDef			USART_InitStructure;
extern EXTI_InitTypeDef			EXTI_InitStructure;


TTime time;

void clearText() {
	for (uint8_t i = 0; i < TEXT_LINES; ++i) {
	  for (uint8_t j = 0; j < TEXT_LINE_MAX_CHARS; ++j) {
		  text[i][j] = 0;
	  }
	}
}

void clearTextPixmap() {
	for (uint16_t j = 0; j < TEXT_LINE_MAX_CHARS*TEXT_CHAR_HEIGHT; ++j) {
		textPixmap[j] = 0;
	}
}

uint8_t getCharData(uint16_t charPos) {
	if (charPos >= CHAR_ARRAY_OFFSET && charPos < CHAR_ARRAY_MAX) {
	  return (oem6x8[charPos - CHAR_ARRAY_OFFSET]);
	}
	else {
		return 0x00;
	}
}

void updateTextPixmapLine(uint8_t textId, uint8_t line) {
	/*for (uint8_t j = 0; j < TEXT_LINE_MAX_CHARS; j++) {
		uint8_t val;
		if (text[textId][j] == ' ' || text[textId][j] == 0) {
			val = 0;
		}
		else {
		  uint16_t charPos = (text[textId][j] * TEXT_CHAR_HEIGHT) + line;
		  val = getCharData(charPos);
#ifdef TEXT_INVERTED_ENABLED
		  if (charInverted(textId, j)) {
		    val = ~val;
		  }
#endif // TEXT_INVERTED_ENABLED
		}
		uint16_t bytePos = line*TEXT_LINE_MAX_CHARS + j;
		textPixmap[bytePos] = val;
	}*/
	uint8_t c=0;
	uint16_t charPos;
	for (uint8_t j = 0; j < TEXT_LINE_MAX_CHARS; j+=2) {
		uint8_t val;
		uint16_t word=0;
		if (text[textId][j] == ' ' || text[textId][j] == 0) {
			val = 0;
			if (text[textId][j+1] == ' ' || text[textId][j+1] == 0) {
				word = 0;
			}
			else{
				  charPos = (text[textId][j+1] * TEXT_CHAR_HEIGHT) + line;
				  word |= getCharData(charPos);
			}
		}
		else{
			  charPos = (text[textId][j] * TEXT_CHAR_HEIGHT) + line;
			  word = getCharData(charPos)<<8;
			  charPos = (text[textId][j+1] * TEXT_CHAR_HEIGHT) + line;
			  word |= getCharData(charPos);
		}
		uint16_t bytePos = line*TEXT_LINE_MAX_CHARS + c;
		textPixmap[bytePos] = word;
		c++;
	}
}

void updateTextPixmap(uint8_t textId) {
	for (uint8_t i = 0; i < TEXT_CHAR_HEIGHT; i++) {
	  updateTextPixmapLine(textId, i);
	}
}

uint8_t printText(uint8_t textId, uint8_t pos, const char* str) {
	uint8_t length = strlen(str);
	if (pos + length >= TEXT_LINE_MAX_CHARS) {
    length = TEXT_LINE_MAX_CHARS;
	}
	strncpy(&text[textId][pos], str, length);
	//memcpy((char *)(&text[textId][pos]), str, length);
	return length+pos;
}
static void myReverse(char s[], uint8_t size) {
  uint8_t i;
  char c;
  size -= 1;
  for (i = 0; i <= size/2; i++) {
    c = s[i];
    s[i] = s[size - i];
    s[size - i] = c;
  }
}

static void myItoa(int32_t n, char s[])
{
  int8_t i;
  int8_t sign = 0;

  if (n < 0) {
	  sign = -1; /* record sign */
    n = -n;          /* make n positive */
  }
  i = 0;
  do {       /* generate digits in reverse order */
    s[i++] = n % 10 + '0';   /* get next digit */
  } while ((n /= 10) > 0);     /* delete it */
  if (sign < 0) {
    s[i++] = '-';
  }
  s[i] = '\0';
  myReverse(s, i);
}
static uint8_t printNumber(uint8_t textId, uint8_t pos, int32_t number) {
	uint8_t length = 1;
	int32_t tmp = number;
	while (tmp > 9) {
		tmp /= 10;
		++length;
	}
	if (pos + length >= TEXT_LINE_MAX_CHARS) {
    return TEXT_LINE_MAX_CHARS;
	}
	myItoa(number, &text[textId][pos]);
	return pos+length;
}

static uint8_t printNumberWithUnit(uint8_t textId, uint8_t pos, int32_t number, const char* unit) {
	pos = printNumber(textId, pos, number);
	return printText(textId, pos, unit);
}

uint8_t printTime(uint8_t textId, uint8_t pos) {
	if (time.hour < 10) {
		text[textId][pos++] = '0';
	}
	pos = printNumberWithUnit(textId, pos, time.hour, ":");
	if (time.min < 10) {
		text[textId][pos++] = '0';
	}
	pos = printNumberWithUnit(textId, pos, time.min, ":");
	if (time.sec < 10) {
		text[textId][pos++] = '0';
	}
	return printNumber(textId, pos, time.sec);
}

void updateText(uint8_t textId) {
  //testPrintDebugInfo();
  uint8_t pos = 0;

  if (textId == 0) {
	  pos = printText(textId, pos, "HELLO OP-OSD");
	  pos =  printTime(textId, pos+18);
/*	  pos = printTime(textId, pos);

	  pos = printAdc(textId, pos+1, ANALOG_IN_1);

#if ANALOG_IN_NUMBER == 2
    pos = printRssiLevel(textId, pos+1, ANALOG_IN_2);
#else // ANALOG_IN_NUMBER == 3
    pos = printAdc(textId, pos+1, ANALOG_IN_2);
	  pos = printRssiLevel(textId, pos+1, ANALOG_IN_3);
#endif //ANALOG_IN_NUMBER == 2*/
  }
  else if (textId == 1) {
	  pos = printText(textId, pos, "All work and no play");

#ifdef GPS_ENABLED
		pos = printNumberWithUnit(textId, pos, homeDistance, TEXT_LENGTH_UNIT);
		pos = printNumberWithUnit(textId, pos+1, homeBearing, "DEG");
		pos = printText(textId, pos+1, gpsHomePosSet ? "H-SET" : "");
#endif //GPS_ENABLED
	}
	else if (textId == 2) {
		pos = printText(textId, pos, "All work and no play");
#ifdef GPS_ENABLED
		pos = printGpsNumber(textId, pos, gpsLastValidData.pos.latitude, 1);
		pos = printGpsNumber(textId, TEXT_LINE_MAX_CHARS-1-10, gpsLastValidData.pos.longitude, 0);
#endif //GPS_ENABLED
	}
	else if (textId == 3) {
		pos = printText(textId, pos, "All work and no play");
#ifdef GPS_ENABLED
		pos = printNumberWithUnit(textId, pos, gpsLastValidData.pos.altitude, TEXT_LENGTH_UNIT);
		pos = printNumberWithUnit(textId, pos+1, gpsLastValidData.speed, TEXT_SPEED_UNIT);
		pos = printNumberWithUnit(textId, pos+1, gpsLastValidData.angle, "DEG");
		pos = printNumberWithUnit(textId, pos+1, gpsLastValidData.sats, "S");
		pos = printText(textId, pos+1, gpsLastValidData.fix ? "FIX" : "BAD");
#endif //GPS_ENABLED
	}
	else {
		/*
		pos = printText(textId, pos, "T:");
		pos = printText(textId, TEXT_LINE_MAX_CHARS-1-4, "V:");
		pos = printNumber(textId, pos+1, textId + 1);*/
	}
}

void drawTextLine(uint8_t textId)
{
	//uint16_t start;
	//PIOS_DELAY_WaituS(3);
	uint8_t currLine = ((uint16_t)(line) - textLines[textId]) / TEXT_SIZE_MULT;
	//for (uint8_t i = 0; i < TEXT_LINE_MAX_CHARS; ++i)
	{
		/*if ((text[textId][i] != ' ') && (text[textId][i] != 0)) {
			GPIO_SetBits(GPIOA,GPIO_Pin_4);
		}
		else {
			GPIO_ResetBits(GPIOA,GPIO_Pin_4);
		}*/
		//GPIO_ResetBits(GPIOA,GPIO_Pin_4);
		//SPI2->DR=0xAA;
		//SPI2->DR = textPixmap[(uint16_t)(currLine)*TEXT_LINE_MAX_CHARS + i];
		GPIO_ResetBits(GPIOA,GPIO_Pin_4);
		//SPI_I2S_SendData(SPI2, textPixmap[(uint16_t)(currLine)*TEXT_LINE_MAX_CHARS + i]);
		//GPIO_SetBits(GPIOA,GPIO_Pin_4);
		DMA_DeInit(DMA1_Channel5);
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&textPixmap[currLine*TEXT_LINE_MAX_CHARS];
		DMA_InitStructure.DMA_BufferSize = TEXT_LINE_MAX_CHARS/2;
		DMA_Init(DMA1_Channel5, &DMA_InitStructure);
		SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
		DMA_Cmd(DMA1_Channel5, ENABLE);
		//DELAY_9_NOP();
		//DELAY_5_NOP();
#ifndef TEXT_SMALL_ENABLED
		//DELAY_6_NOP();
		//DELAY_9_NOP();
#endif //TEXT_SMALL_ENABLED
		//start = PIOS_DELAY_TIMER->CNT;

		/* Note that this event works on 16bit counter wrap-arounds */
		//while ((uint16_t) (PIOS_DELAY_TIMER->CNT - start) <= 3) ;
	}
	//DELAY_10_NOP();
	//SPI2->DR = 0x00;
	//start = PIOS_DELAY_TIMER->CNT;

	/* Note that this event works on 16bit counter wrap-arounds */
	//while ((uint16_t) (PIOS_DELAY_TIMER->CNT - start) <= 10) ;
	//GPIO_ResetBits(GPIOA,GPIO_Pin_4);
}
