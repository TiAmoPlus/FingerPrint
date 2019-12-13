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
//ָ�ƴ�����ʹ������
SoftwareSerial serialFinger(MY_PIN_FINGER_RX, MY_PIN_FINGER_TX);
Adafruit_Fingerprint finger(&serialFinger);

//ESP Wifiģ��ʹ������
SoftwareSerial serialESP(MY_PIN_ESP_RX, MY_PIN_ESP_TX);
//Wifi��������
String ssid = "522";
String password = "522522522";
//Wifi �ͻ���
WiFiEspClient client;
String serverAddress = "192.168.1.100";
int port = 8234;
/*
����Ϣ��ʾ���û�
*/
void showMessage(String message) {
	Serial.println(message);
}
/*
���ų���
*/
void openDoor() {
	
}

/*
��ȡ��ָ������ ���ʧ�� �򷵻�0
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
		// Ӧ���
		if (receive->data[0] == FINGERPRINT_OK) {
			// ȷ����OK
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
��ȡָ�ƿ����ID����Χ��0����ָ�ƿⷶΧ-1 ����getDataSize() ��ȡָ�ƿ��С 
ʧ�ܷ���-1
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
			// Ӧ���
			if (receive->data[0] == FINGERPRINT_OK) {
				// ȷ����OK
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
�������������Ϣ
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
	ָ�Ƴ�ʼ��
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
	ESP��ʼ��
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
		long rssi = WiFi.RSSI();// �ź�ǿ��
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
¼ָ�ƣ��ɹ�����¼���ID�ţ�ʧ�ܷ���-1
*/
int saveFingerPrint() {
	
	int id = getEmptyID();
	if (id == -1) {
		//showMessage(F("Finger Full"));//ָ�ƿ��������
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
	// ��һ�βɼ�ָ�Ʋ���ת���ɹ�
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
	// �ڶ���ָ�Ʋɼ�����ת���ɹ�

	//��������������ָ�Ƴ�������ģ��
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
��ָ��ѹ�¼������ָ�Ƴɹ���ȡ������ָ�ƿ��бȶԳɹ�������true
ͬʱ���ָ�ƱȶԳɹ��������������JSON��Ϣ��ʧ��Ҳ����JSON��Ϣ
*/
bool fingerEvent() {
	serialFinger.listen();
	serialESP.stopListening();
	uint8_t p = finger.getImage();
	String data;
	switch (p) {
	case FINGERPRINT_OK:
		// �ɹ���ȡ��ͼƬ
		//showMessage(F("Image taken"));
		p = finger.image2Tz();
		switch (p) {
		case FINGERPRINT_OK:
			// �ɹ���ͼƬ����ȡ����
			//showMessage(F("Image converted"));
			p = finger.fingerFastSearch();
			StaticJsonDocument<100> doc;
			switch (p)
			{
			case FINGERPRINT_OK:
				//�ɹ��ҵ�ָ�� ��������Ҫ���͵���������
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
				//ָ�Ʋ���ָ�ƿ���
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
�ж��Ƿ���ָ�ư���
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
�������״̬������ʾ
*/
uint8_t checkWifi() {
	uint8_t status=WiFi.status();
	if (status == WL_NO_SHIELD) {
		//Wifiģ��û����
		showMessage("No Wifi Module");
	}
	else if (status == WL_DISCONNECTED)
	{
		//Wifiû�������� ������
		showMessage("Wifi No Available Reconnecting");
		WiFi.begin(ssid.c_str(), password.c_str());
	}
	else if (status == WL_CONNECTED)
	{
		//����Wifi��
	}
	else if (status == WL_IDLE_STATUS)
	{
		// Ҳ��������Wifi ��������Wifi
	}
	return status;
}
/*
��������������
*/
uint8_t checkServer() {
	uint8_t status=client.status();
	switch (status)
	{
	case ESTABLISHED:
		//�Ѿ���������
		//showMessage("Server OK");
		break;
	case CLOSED:
		//û���Ϸ�����
		showMessage("Server Reconnecting");
		client.connect(serverAddress.c_str(), port);
		break;
	default:
		break;
	}
	return status;
}
/*
���ַ���ת��JSON��ʽ ������
*/
void dealString(String message) {
	StaticJsonDocument<100> doc;
	DeserializationError error = deserializeJson(doc, message);
	if (error) {
		//JSON��������
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