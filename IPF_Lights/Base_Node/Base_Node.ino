
#include <RF24_config.h>
#include <printf.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>


#define CE_PIN   9
#define CSN_PIN 10


const int rem1_Red = 2;
const int rem1_White = 3;
const int rem2_Red = 4;
const int rem2_White = 5;
const int rem3_Red = 6;
const int rem3_White = 7;


const byte slaveAddress[3][5] = {
	// each slave needs a different address
						{'R','x','A','A','A'},
						{'R','x','A','A','B'},
						{'R','x','A','A','C'}
};

RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

int sndData[3] = { 0,0,0 };
int rcvData[3] = { 0,0,0 };
int remData[3] = { 0,0,0 };
int seqState = 0;
int count = 0;
unsigned long currentMillis;
unsigned long pollMillis;
unsigned long timeoutMillis;
unsigned long displayMillis;
unsigned long timeoutLengthMillis = 15000;
unsigned long displayLengthMillis = 15000;
unsigned long txIntervalMillis = 250; // send twice per second

//===============

void setup() {

	pinMode(A0, INPUT);
	pinMode(rem1_White, OUTPUT);
	pinMode(rem1_Red, OUTPUT);
	pinMode(rem2_White, OUTPUT);
	pinMode(rem2_Red, OUTPUT);
	pinMode(rem3_White, OUTPUT);
	pinMode(rem3_Red, OUTPUT);

	Serial.begin(9600);
	Serial.println("BrockTronics IPF Lights Base Station");


	radio.begin();
	radio.setDataRate(RF24_250KBPS);

	radio.enableAckPayload();

	radio.setRetries(3, 5); // delay, count
}

//=============

void loop() {

	currentMillis = millis();
	if (currentMillis - pollMillis >= txIntervalMillis) {
		send();
	}

	switch (seqState) {
	case 0: //Steady state, lights are off and no judgements made
		Serial.println("Sequence state 0");

		if (digitalRead(A0)) { //Statement for entering test mode
			seqState = 99;
			break;
		}

		if (rcvData[0] != 0) { //Judge 1 made a call
			remData[0] = rcvData[0]; //Store the remote value locally
			sndData[0] = 2; //Blue and Green light on remote
			sndData[1] = 1; //Blue light on remote
			sndData[2] = 1; //Blue light on remote
			seqState = 1;
			timeoutMillis = millis();
		}
		else if (rcvData[1] != 0) { //Judge 2 made a call
			remData[1] = rcvData[1]; //Store the remote value locally
			sndData[0] = 1; //Blue light on remote
			sndData[1] = 2; //Blue and Green light on remote
			sndData[2] = 1; //Blue light on remote
			seqState = 1;
			timeoutMillis = millis();
		}
		else if (rcvData[2] != 0) { //Judge 3 made a call
			remData[2] = rcvData[2]; //Store the remote value locally
			sndData[0] = 1; //Blue light on remote
			sndData[1] = 1; //Blue light on remote
			sndData[2] = 2; //Blue and Green light on remote
			seqState = 1;
			timeoutMillis = millis();
		}
		break;

	case 1: //First judement made, blue lights lit on remotes, remaining judges have 15 seconds to make a judgement
		Serial.print("Sequence state 1: ");
		Serial.print(rcvData[0]);
		Serial.print(rcvData[1]);
		Serial.println(rcvData[2]);
		if (currentMillis - timeoutMillis >= timeoutLengthMillis) {
			seqState = 2; //Timeout has occured
			displayMillis = millis();
			break;
		}

		if (remData[0] == 0) remData[0] = rcvData[0]; //Store remote data locally
		if (remData[1] == 0) remData[1] = rcvData[1];
		if (remData[2] == 0) remData[2] = rcvData[2];

		if (remData[0] != 0) sndData[0] = 2; //Blue and Green light on remote
		if (remData[1] != 0) sndData[1] = 2; //Blue and Green light on remote
		if (remData[2] != 0) sndData[2] = 2; //Blue and Green light on remote

		if ((remData[0] != 0) && (remData[1] != 0) && (remData[2] != 0)) {
			seqState = 2; //all judgements recieved
			displayMillis = millis();
		}

		break;

	case 2: //All judgements made, light the corresponding lamps for 15 seconds
		Serial.print("Sequence state 2: ");
		Serial.print(remData[0]);
		Serial.print(remData[1]);
		Serial.println(remData[2]);

		if (currentMillis - displayMillis >= displayLengthMillis) {
			seqState = 3; //Clear all outputs and states
			break;
		}

		//Judge 1's remote
		if (remData[0] == 1) digitalWrite(rem1_White, HIGH);
		else digitalWrite(rem1_White, LOW);
		if (remData[0] == 2) digitalWrite(rem1_Red, HIGH);
		else digitalWrite(rem1_Red, LOW);

		//Judge 2's remote
		if (remData[1] == 1) digitalWrite(rem2_White, HIGH);
		else digitalWrite(rem2_White, LOW);
		if (remData[1] == 2) digitalWrite(rem2_Red, HIGH);
		else digitalWrite(rem2_Red, LOW);

		//Judge 3's remote
		if (remData[2] == 1) digitalWrite(rem3_White, HIGH);
		else digitalWrite(rem3_White, LOW);
		if (remData[2] == 2) digitalWrite(rem3_Red, HIGH);
		else digitalWrite(rem3_Red, LOW);

		break;

	case 3: //Clear all lights and reset remote states
		sndData[0] = 0;
		sndData[1] = 0;
		sndData[2] = 0;
		remData[0] = 0;
		remData[1] = 0;
		remData[2] = 0;
		digitalWrite(rem1_White, LOW);
		digitalWrite(rem1_Red, LOW);
		digitalWrite(rem2_White, LOW);
		digitalWrite(rem2_Red, LOW);
		digitalWrite(rem3_White, LOW);
		digitalWrite(rem3_Red, LOW);
		seqState = 0; //Back to normal state waiting for next lifter
		break;

	case 99: //Test mode, light corresponding lamp while red/white button is pressed for each remote
		Serial.print("Sequence state 99: Data ");
		Serial.print(rcvData[0]);
		Serial.print(rcvData[1]);
		Serial.println(rcvData[2]);

		if (!digitalRead(A0)) { //Statement for exiting test mode
			seqState = 0;
			digitalWrite(rem1_White, LOW);
			digitalWrite(rem1_Red, LOW);
			digitalWrite(rem2_White, LOW);
			digitalWrite(rem2_Red, LOW);
			digitalWrite(rem3_White, LOW);
			digitalWrite(rem3_Red, LOW);
			sndData[0] = 0;
			sndData[1] = 0;
			sndData[2] = 0;
			break;
		}

		sndData[0] = 3;
		sndData[1] = 3;
		sndData[2] = 3;

		//Judge 1's remote
		if (rcvData[0] == 1) digitalWrite(rem1_White, HIGH);
		else digitalWrite(rem1_White, LOW);
		if (rcvData[0] == 2) digitalWrite(rem1_Red, HIGH);
		else digitalWrite(rem1_Red, LOW);


		//Judge 2's remote
		if (rcvData[1] == 1) digitalWrite(rem2_White, HIGH);
		else digitalWrite(rem2_White, LOW);
		if (rcvData[1] == 2) digitalWrite(rem2_Red, HIGH);
		else digitalWrite(rem2_Red, LOW);

		//Judge 3's remote
		if (rcvData[2] == 1) digitalWrite(rem3_White, HIGH);
		else digitalWrite(rem3_White, LOW);
		if (rcvData[2] == 2) digitalWrite(rem3_Red, HIGH);
		else digitalWrite(rem3_Red, LOW);

		break;
	}
}

//================

void send() {

	// call each slave in turn
	for (byte n = 0; n < 3; n++) {

		// open the writing pipe with the address of a slave
		radio.openWritingPipe(slaveAddress[n]);

		bool rslt;
		rslt = radio.write(&sndData[n], sizeof(sndData[n]));

		if (rslt) {
			if (radio.isAckPayloadAvailable()) {
				radio.read(&rcvData[n], sizeof(rcvData[n]));
			}
		}
	}

	pollMillis = millis();
}