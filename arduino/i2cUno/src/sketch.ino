#include <Wire.h>

#define SLAVE_ADDRESS 0x05

unsigned char number = 0;
int state = 0;

struct commandStruct {
	bool			needData;
	unsigned char	commandNumber;
	unsigned char	pinNumber;
	unsigned char	data;
	int 			response;
} request;

void cleanRequest() {
	// initialize the command struct
	request.needData = false;
	request.commandNumber = 0;
	request.pinNumber = 0;
}

void setup() {
    pinMode(13, OUTPUT);
    Serial.begin(9600);         // start serial for output
    // initialize i2c as slave
    Wire.begin(SLAVE_ADDRESS);

	// initialize the command struct
	cleanRequest();
	request.response = 0;
	
    // define callbacks for i2c communication
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);

    Serial.println("Ready!");
}

void loop() {
    delay(100);
}

// callback for received data
void receiveData(int byteCount){

    while(Wire.available()) {
        number = Wire.read();
        Serial.print("data received: ");
        Serial.println(number);

		// 
		if ( !request.needData ) {
			unsigned char commandId = number & B11100000;
			request.commandNumber = commandId >> 5;
			Serial.println(request.commandNumber);
			
			switch( request.commandNumber ) {
				case 0 :		// Read Numeric Port - 0x0n
					request.pinNumber = number & B00011111;
					Serial.println(request.pinNumber);
					pinMode(request.pinNumber, INPUT);
					if ( digitalRead(request.pinNumber) == HIGH ) {
						request.response = 1;
					} else {
						request.response = 0;
					}
					break;
				case 1 :		// Read Analog Port - 0x2n
					request.pinNumber = ( number & B00011111 );
					Serial.println(request.pinNumber);
					request.response = analogRead(request.pinNumber);
					Serial.println(request.response);
					break;
				case 2 :		// Write Numeric Port - 0x4n
					request.pinNumber = ( number & B00001111 );
					request.data = ( number & B00010000 );
					pinMode(request.pinNumber, OUTPUT);
					digitalWrite(request.pinNumber, (request.data==0?LOW:HIGH));
					Serial.println(request.pinNumber);
					break;
				case 3 :		// Write Analog Port / PWM - 0x6n
					request.pinNumber = ( number & B00011111 );
					request.needData = true;
					Serial.println(request.pinNumber);
					break;
			}
		} else {
			request.data = number;
			analogWrite(request.pinNumber, request.data);
			request.needData = false;
		}
	}
}

// callback for sending data
void sendData(){
	byte byte1 = (byte)request.response;
	byte byte2 = *(((byte*)&request.response)+1);
	Serial.println(byte1);
	Serial.println(byte2);
	//Wire.write(byte1);
	//Wire.write(byte2);
	//Wire.write((int)request.response);
	Wire.write((byte*)&request.response, 2);
}

