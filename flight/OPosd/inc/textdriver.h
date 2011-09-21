/*
 * textdriver.h
 *
 *  Created on: 15.9.2011
 *      Author: Samba
 */

#ifndef TEXTDRIVER_H_
#define TEXTDRIVER_H_

#include "videoconf.h"
#include "xconvert.h"
#include "oem6x8.h"

extern uint16_t const textLines[];
extern volatile uint16_t line;

void clearText();
void clearTextPixmap();
uint8_t getCharData(uint16_t charPos);
void updateTextPixmapLine(uint8_t textId, uint8_t line);
void updateTextPixmap(uint8_t textId);
uint8_t printText(uint8_t textId, uint8_t pos, const char* str);
void updateText(uint8_t textId);
void drawTextLine(uint8_t textId);

#endif /* TEXTDRIVER_H_ */
