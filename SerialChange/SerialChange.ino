/*
 Name:		SerialChange.ino
 Created:	2019/9/7 15:27:10
 Author:	TiAmo
*/

// the setup function runs once when you press reset or power the board
#include <SoftwareSerial.h>
SoftwareSerial Serial1(6, 7); // RX, TX
void setup() {
	Serial.begin(9600);
	Serial1.begin(9600);

}

// the loop function runs over and over again until power down or reset
void loop() {
	if (Serial.available()) {
		String temp = Serial.readString();
		Serial1.println(temp);
	}
	if (Serial1.available()) {
		String temp = Serial1.readString();
		Serial.println(temp);
	}
}
