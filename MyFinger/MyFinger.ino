/*
 Name:		MyFinger.ino
 Created:	2019/9/5 11:42:35
 Author:	TiAmo
*/


#include <EEPROM.h>
#include <WiFiEspUdp.h>
#include <WiFiEspServer.h>
#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include "my_pins.h"
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>
//指纹传感器使用软串口
SoftwareSerial serialFinger(MY_PIN_FINGER_RX, MY_PIN_FINGER_TX);
Adafruit_Fingerprint finger(&serialFinger);

//ESP Wifi模块使用软串口
SoftwareSerial serialESP(MY_PIN_ESP_RX, MY_PIN_ESP_TX);
//Wifi名和密码
String ssid = "522";
String password = "522522522";
//Wifi 客户端
WiFiEspClient client;
String serverAddress = "192.168.1.100";
int port = 8234;
/*
将信息显示给用户
*/
void showMessage(String message) {
	Serial.println(message);
}
/*
开门程序
*/
void openDoor() {
	
}

/*
获取到指纹容量 如果失败 则返回0
*/
uint16_t getDataBaseSize() {
	serialFinger.listen();
	serialESP.stopListening();
	uint8_t data[] = { FINGERPRINT_READSYSPARA };
	uint8_t length = sizeof(data);
	Adafruit_Fingerprint_Packet packet(FINGERPRINT_COMMANDPACKET, length, data);
	finger.writeStructuredPacket(packet);
	Adafruit_Fingerprint_Packet *receive = (Adafruit_Fingerprint_Packet*)malloc(sizeof(Adafruit_Fingerprint_Packet));
	finger.getStructuredPacket(receive);
	if (receive->type == FINGERPRINT_ACKPACKET) {
		// 应答包
		if (receive->data[0] == FINGERPRINT_OK) {
			// 确认码OK
			uint16_t size = ((receive->data[5] << 8) | receive->data[6]);
			return size;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
/*
获取指纹库可用ID，范围从0——指纹库范围-1 利用getDataSize() 获取指纹库大小 
失败返回-1
*/
int getEmptyID() {
	serialFinger.listen();
	serialESP.stopListening();
	uint16_t size = getDataBaseSize();
	for (int i = 0; i < size; i++)
	{
		uint8_t data[] = { FINGERPRINT_READINDEXTABLE };
		uint8_t length = sizeof(data);
		Adafruit_Fingerprint_Packet packet(FINGERPRINT_COMMANDPACKET, length, data);
		finger.writeStructuredPacket(packet);
		Adafruit_Fingerprint_Packet *receive = (Adafruit_Fingerprint_Packet*)malloc(sizeof(Adafruit_Fingerprint_Packet));
		finger.getStructuredPacket(receive);
		if (receive->type == FINGERPRINT_ACKPACKET) {
			// 应答包
			if (receive->data[0] == FINGERPRINT_OK) {
				// 确认码OK
				for (int dataIndex = 1; dataIndex <= 32; dataIndex++) {
					for (int j = 0; j < 8; j++) {
						if ((receive->data[dataIndex] & (1 << j)) == 0) {
							return i;
						}
						i++;
						if (i >= size) {
							return -1;
						}
						
					}
				}
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
}
/*
向服务器发送信息
*/
void sendDataToServer(String data) {
	//serialFinger.stopListening();
	serialESP.listen();
	serialFinger.stopListening();
	client.println(data);
	//Serial.println(data);
}
bool Init() {
	/*
	指纹初始化
	*/
	Serial.begin(MY_BAUD_SERIAL);
	while (!Serial);  // For Yun/Leo/Micro/Zero/...
	finger.begin(MY_BAUD_FINGER);
	if (finger.verifyPassword()) {
		//showMessage("Found fingerprint sensor!");
		/*uint16_t size = getDataBaseSize();
		Serial.print("Size:"); Serial.println(String(size));
		getEmptyID();*/
	}
	else {
		showMessage(F("No fingerprint sensor"));
		return false;
	}
	//serialFinger.stopListening();
	/*
	ESP初始化
	*/
	serialESP.begin(MY_BAUD_ESP);
	WiFi.init(&serialESP);
	if (WiFi.status() == WL_NO_SHIELD) {
		showMessage(F("WiFi shield not present"));
		return false;
	}
	//showMessage("Connect to WPA SSID: " + ssid);
	if (WiFi.begin(ssid.c_str(), password.c_str()) == WL_CONNECTED) {
		//showMessage(F("You're connected to the network"));
		IPAddress ip = WiFi.localIP();
		long rssi = WiFi.RSSI();// 信号强度
		//showMessage("IP Address: "+String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3]));
		//showMessage("Signal strength (RSSI):"+String(rssi)+" dBm");
	}
	else
	{
		showMessage("Network :" + ssid + "Error");
		return false;
	}
	//showMessage("Starting connection to server..." + serverAddress + ":" + String(port));
	if (client.connect(serverAddress.c_str(), port)) {
		//showMessage("Succeed to connect to server" + serverAddress + ":" + String(port));
	}
	else
	{
		showMessage("Server" + serverAddress + ":" + String(port)+"Error");
		return false;
	}
	return true;
	
}
/*
录指纹，成功返回录入的ID号，失败返回-1
*/
int saveFingerPrint() {
	
	int id = getEmptyID();
	if (id == -1) {
		//showMessage(F("Finger Full"));//指纹库可能已满
		return -1;
	}
	int p = -1;
	showMessage("Saving:" + String(id));
	serialFinger.listen();
	serialESP.stopListening();
	while (p != FINGERPRINT_OK) {
		p = finger.getImage();
		switch (p) {
		case FINGERPRINT_OK:
			/*showMessage("Image taken");*/
			p = finger.image2Tz(1);
			switch (p) {
			case FINGERPRINT_OK:
				break;
			/*case FINGERPRINT_IMAGEMESS:
				Serial.println("Image too messy");
				return -1;
			case FINGERPRINT_PACKETRECIEVEERR:
				Serial.println("Communication error");
				return p;
			case FINGERPRINT_FEATUREFAIL:
				Serial.println("Could not find fingerprint features");
				return p;
			case FINGERPRINT_INVALIDIMAGE:
				Serial.println("Could not find fingerprint features");
				return p;*/
			default:
				/*Serial.println("Unknown error");
				return p;*/
				break;
			}
			break;
		case FINGERPRINT_NOFINGER:
			break;
		case FINGERPRINT_PACKETRECIEVEERR:
			break;
		case FINGERPRINT_IMAGEFAIL:
			break;
		default:
			/*Serial.println("Unknown error");*/
			break;
		}
	}
	// 第一次采集指纹并且转换成功
	//showMessage(F("First"));

	showMessage(F("Remove finger"));
	delay(2000);
	p = 0;
	while (p != FINGERPRINT_NOFINGER) {
		p = finger.getImage();
	}
	p = -1;
	showMessage(F("Place same finger again"));
	while (p != FINGERPRINT_OK) {
		p = finger.getImage();
		switch (p) {
		case FINGERPRINT_OK:
			/*showMessage("Image taken");*/
			p = finger.image2Tz(2);
			switch (p) {
			case FINGERPRINT_OK:
				break;
				/*case FINGERPRINT_IMAGEMESS:
					Serial.println("Image too messy");
					return -1;
				case FINGERPRINT_PACKETRECIEVEERR:
					Serial.println("Communication error");
					return p;
				case FINGERPRINT_FEATUREFAIL:
					Serial.println("Could not find fingerprint features");
					return p;
				case FINGERPRINT_INVALIDIMAGE:
					Serial.println("Could not find fingerprint features");
					return p;*/
			default:
				/*Serial.println("Unknown error");
				return p;*/
				break;
			}
			break;
		case FINGERPRINT_NOFINGER:
			break;
		case FINGERPRINT_PACKETRECIEVEERR:
			break;
		case FINGERPRINT_IMAGEFAIL:
			break;
		default:
			/*Serial.println("Unknown error");*/
			break;
		}
	}
	// 第二次指纹采集并且转换成功

	//接下来根据两次指纹尝试生成模型
	//showMessage("Creating model for #" + String(id));

	p = finger.createModel();

	if (p == FINGERPRINT_OK) {
		//showMessage(F("Prints matched!"));
	}
	/*else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		Serial.println("Communication error");
		return p;
	}
	else if (p == FINGERPRINT_ENROLLMISMATCH) {
		Serial.println("Fingerprints did not match");
		return p;
	}*/
	else {
		showMessage(F("Error"));
		return -1;
	}
	p = finger.storeModel(id);

	if (p == FINGERPRINT_OK) {
		showMessage("Stored Finger in ID"+String(id));
		return id;
	}
	//else if (p == FINGERPRINT_PACKETRECIEVEERR) {
	//	//Serial.println(F("Communication error"));
	//	//return p;
	//	return -1;
	//}
	//else if (p == FINGERPRINT_BADLOCATION) {
	//	Serial.println(F("Could not store in that location"));
	//	return p;
	//}
	//else if (p == FINGERPRINT_FLASHERR) {
	//	Serial.println(F("Error writing to flash"));
	//	return p;
	//}
	else {
		showMessage(F("Error"));
		return -1;
	}
}
/*
手指按压事件，如果指纹成功获取到并在指纹库中比对成功，返回true
同时如果指纹比对成功，向服务器发送JSON消息，失败也发送JSON消息
*/
bool fingerEvent() {
	serialFinger.listen();
	serialESP.stopListening();
	uint8_t p = finger.getImage();
	String data;
	switch (p) {
	case FINGERPRINT_OK:
		// 成功获取到图片
		//showMessage(F("Image taken"));
		p = finger.image2Tz();
		switch (p) {
		case FINGERPRINT_OK:
			// 成功从图片中提取特征
			//showMessage(F("Image converted"));
			p = finger.fingerFastSearch();
			StaticJsonDocument<100> doc;
			switch (p)
			{
			case FINGERPRINT_OK:
				//成功找到指纹 并且数据要发送到服务器上
				showMessage(F("Found a print match!"));
				showMessage("ID:" + String(finger.fingerID));
				showMessage("Confidence:" + String(finger.confidence));
				
				doc["type"] = "finger";
				doc["action"] = "login";
				doc["result"] = "success";
				doc["id"] = finger.fingerID;
				doc["confidence"] = finger.confidence;

				serializeJson(doc, data);
				sendDataToServer(data);
				return true;
			case FINGERPRINT_NOTFOUND:
				//指纹不在指纹库中
				showMessage(F("Did not find a match"));
				doc["type"] = "finger";
				doc["action"] = "login";
				doc["result"] = "fail";
				serializeJson(doc, data);
				sendDataToServer(data);
				return false;
			default:
				break;
			}
			break;
		case FINGERPRINT_IMAGEMESS:
			//showMessage(F("Image too messy"));
			return false;
		case FINGERPRINT_PACKETRECIEVEERR:
			//showMessage(F("Communication error"));
			return false;
		case FINGERPRINT_FEATUREFAIL:
			//showMessage("Could not find fingerprint features");
			return false;
		case FINGERPRINT_INVALIDIMAGE:
			//showMessage("Could not find fingerprint features");
			return false;
		default:
			//showMessage("Unknown error");
			return false;
		}
		break;
	case FINGERPRINT_NOFINGER:
		//showMessage("No finger detected");
		return false;
	case FINGERPRINT_PACKETRECIEVEERR:
		//showMessage("Communication error");
		return false;
	case FINGERPRINT_IMAGEFAIL:
		//showMessage("Imaging error");
		return false;
	default:
		//showMessage("Unknown error");
		return false;
	}
}
/*
判断是否有指纹按下
*/
bool isFingerPressed() {
	if (digitalRead(MY_PIN_FINGER_PRESSED) == HIGH) {
		return true;
	}
	else
	{
		return false;
	}
}
/*
检测网络状态，并显示
*/
uint8_t checkWifi() {
	uint8_t status=WiFi.status();
	if (status == WL_NO_SHIELD) {
		//Wifi模块没连上
		showMessage("No Wifi Module");
	}
	else if (status == WL_DISCONNECTED)
	{
		//Wifi没连上网络 重新连
		showMessage("Wifi No Available Reconnecting");
		WiFi.begin(ssid.c_str(), password.c_str());
	}
	else if (status == WL_CONNECTED)
	{
		//连上Wifi了
	}
	else if (status == WL_IDLE_STATUS)
	{
		// 也是正在连Wifi 算是连上Wifi
	}
	return status;
}
/*
检测与服务器连接
*/
uint8_t checkServer() {
	uint8_t status=client.status();
	switch (status)
	{
	case ESTABLISHED:
		//已经建立连接
		//showMessage("Server OK");
		break;
	case CLOSED:
		//没连上服务器
		showMessage("Server Reconnecting");
		client.connect(serverAddress.c_str(), port);
		break;
	default:
		break;
	}
	return status;
}
/*
将字符串转成JSON格式 并处理
*/
void dealString(String message) {
	StaticJsonDocument<100> doc;
	DeserializationError error = deserializeJson(doc, message);
	if (error) {
		//JSON解析出错
		showMessage(F("message Wrong"));
		return;
	}
	String action = doc["action"];
	if (action.equals("changeWifi")) {
		ssid = doc["ssid"].as<String>();
		password = doc["password"].as<String>();
		WiFi.begin(ssid.c_str(), password.c_str());
	}
}
// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(MY_BAUD_SERIAL);
	while (!Serial);  // For Yun/Leo/Micro/Zero/...
	
	finger.begin(MY_BAUD_FINGER);
	if (finger.verifyPassword()) {
	}
	else {
		showMessage(F("No fingerprint sensor"));
	}
	serialESP.begin(MY_BAUD_ESP);
	WiFi.init(&serialESP);

	WiFi.begin(ssid.c_str(), password.c_str());
	if (client.connect(serverAddress.c_str(), port)) {
		//showMessage("Succeed to connect to server" + serverAddress + ":" + String(port));
	}
	else
	{
		showMessage("Server" + serverAddress + ":" + String(port) + "Error");
	}
	//int id = getEmptyID();
	//saveFingerPrint();
}

// the loop function runs over and over again until power down or reset
void loop() {
	if (isFingerPressed() == true) {
		//showMessage("Finger Pressed");
		fingerEvent();
	}
	checkWifi();
	checkServer();
	if (Serial.available()) {
		String message = Serial.readString();
		dealString(message);
	}
	delay(100);
}