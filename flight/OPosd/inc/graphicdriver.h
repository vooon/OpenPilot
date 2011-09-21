/*
 * graphicdriver.h
 *
 *  Created on: 16.9.2011
 *      Author: Samba
 */

#ifndef GRAPHICDRIVER_H_
#define GRAPHICDRIVER_H_

#include "videoconf.h"

extern volatile uint16_t line;

void clearGraphics();
uint8_t validPos(uint8_t x, uint8_t y);
void setPixel(uint8_t x, uint8_t y, uint8_t state);
void drawCircle(uint8_t x0, uint8_t y0, uint8_t radius);
void swap(uint8_t* a, uint8_t* b);
void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void updateGrapics();
void drawGrapicsLine();

#endif /* GRAPHICDRIVER_H_ */
