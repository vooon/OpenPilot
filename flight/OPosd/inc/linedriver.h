/*
 * linedriver.h
 *
 *  Created on: 15.9.2011
 *      Author: Samba
 */

#ifndef LINEDRIVER_H_
#define LINEDRIVER_H_
#include <pios.h>

extern volatile uint8_t update;
extern volatile uint8_t activeTextId;
extern volatile uint16_t activeTextLine;
extern volatile uint8_t lineType;
extern volatile uint16_t line;

void initLine();
void updateLine();

#endif /* LINEDRIVER_H_ */
