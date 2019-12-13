// my_pins.h

#ifndef _MY_PINS_h
#define _MY_PINS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#endif
/*
引脚定义
*/
#define MY_PIN_FINGER_RX	2		
#define MY_PIN_FINGER_TX	3
#define MY_PIN_FINGER_PRESSED		4
#define MY_PIN_DOOR					5
#define MY_PIN_ESP_RX				6
#define MY_PIN_ESP_TX				7


#define MY_PASSWORD_FINGERPRINT		1337
#define MY_BAUD_FINGER				57600
#define MY_BAUD_SERIAL				9600
#define MY_BAUD_ESP					9600


/*
定义指纹传感器未添加的指令
*/

#define FINGERPRINT_READSYSPARA		0x0F
#define FINGERPRINT_READINDEXTABLE	0x1F