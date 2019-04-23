

// SimpleRxAckPayload- the slave or the receiver

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN   9
#define CSN_PIN 10

const byte slaveAddress[3][5] = {
	// each slave needs a different address
						{'R','x','A','A','A'},
						{'R','x','A','A','B'},
						{'R','x','A','A','C'}
};
const int grnLed = 5;
const int bluLed = 4;
const int whtBtn = 3;
const int redBtn = 2;

unsigned long currentMillis;
unsigned long prevMillis;
unsigned long blinkMillis = 250;

RF24 radio(CE_PIN, CSN_PIN);

int rcvData; // this must match dataToSend in the TX
int ackData = 0; // the two values to be sent to the master
int node = 0;

//==============

void setup() {

	pinMode(A0, INPUT);
	pinMode(A1, INPUT);
	pinMode(redBtn, INPUT);
	pinMode(whtBtn, INPUT);
	pinMode(grnLed, OUTPUT);
	pinMode(bluLed, OUTPUT);

	Serial.begin(9600);

	Serial.println("BrockTronics IPF Lights Remote node starting");

	bool mode_sel1 = digitalRead(A0);
	bool mode_sel2 = digitalRead(A1);

	//Use the DIP switch to set which judge the remote is assigned to
	if ((mode_sel1 == LOW) && (mode_sel2 == LOW)) {
		node = 0;
		setup_blink(1);
	}
	if ((mode_sel1 == LOW) && (mode_sel2 == HIGH)) {
		node = 1;
		setup_blink(2);
	}
	if ((mode_sel1 == HIGH) && (mode_sel2 == LOW)) {
		node = 1;
		setup_blink(2);
	}
	if ((mode_sel1 == HIGH) && (mode_sel2 == HIGH)) {
		node = 3;
		setup_blink(3);
	}
	Serial.print("Remote node set to address ");
	Serial.println(node);

	radio.begin();
	radio.setDataRate(RF24_250KBPS);
	radio.openReadingPipe(1, slaveAddress[node]);

	radio.enableAckPayload();

	radio.startListening();

	radio.writeAckPayload(1, &ackData, sizeof(ackData)); // pre-load data
}

//==========

void loop() {

	currentMillis = millis();

	switch (rcvData) {
	case 0: //Steady state waiting for a call
		if (digitalRead(redBtn)) ackData = 1;
		if (digitalRead(whtBtn)) ackData = 2;
		digitalWrite(bluLed, LOW);
		digitalWrite(grnLed, LOW);
		break;
	case 1: //Call receieved by other remote, waiting for call
		digitalWrite(bluLed, HIGH);
		digitalWrite(grnLed, LOW);
		break;
	case 2: //Call recieved and confirmed
		digitalWrite(bluLed, HIGH);
		digitalWrite(grnLed, HIGH);
		ackData = 0;
		break;
	case 3: //Test mode
		if (digitalRead(redBtn)) ackData = 1;
		else if (digitalRead(whtBtn)) ackData = 2;
		else ackData = 0;
		if (currentMillis - prevMillis >= blinkMillis) {
			digitalWrite(bluLed, !digitalRead(bluLed));
			digitalWrite(grnLed, !digitalRead(grnLed));
			prevMillis = millis();
		}
		break;
	}

	Serial.print(rcvData);
	Serial.print(" , ");
	Serial.println(ackData);
	radio.writeAckPayload(1, &ackData, sizeof(ackData)); //prepares ack payload for when polled by master
	getData();
}

//============

void getData() {
	if (radio.available()) {
		radio.read(&rcvData, sizeof(rcvData));
	}
}

void setup_blink(int judgeNumber) {
	for (int i = 0; i < judgeNumber; i++) {
		digitalWrite(bluLed, HIGH);
		digitalWrite(grnLed, HIGH);
		delay(1000);
		digitalWrite(bluLed, LOW);
		digitalWrite(grnLed, LOW);
		if (i != judgeNumber)  delay(500);
	}
}