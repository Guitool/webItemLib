# WebItemLib

The project is about a simple C framework for communication between a RaspberryPi and an Arduino chip. Communications between boards use I2C two wires.


## Specification

What's the protocol about :

* read request
	* for a analog port from 0 to 6
		* return value as word, 10 bits (0,1023)
	* for a numeric port from 0 to 13
		* return value as byte, 8 bits (0,254)
* write request
	* to an analog port from 0 to 6
		* value as 8 bits (0, 254)
	* to a digital port from 0 to 13
		* value as 8 bits (0, 254) for PWM
		* value as 1 bit
* extended mode
	* everything that cannot fit in previous mode

### Protocol

Request use one byte, only followed by a data one for PWM / analog write :

	| 7 6 5 4 3 2 1 0 |
	  ^ . . . . . . . : mode
	    ^ . . . . . . : direction
	      ^ . . . . . : port type
	        ^ ^ ^ ^ ^ : port number
	        ^ . . . . : state         ( numeric write only )
	          ^ ^ ^ ^ : port number   ( numeric write only )
	
* Bit **7** : operating mode
	* 0 - basic read / write operation
	* 1 - extended mode, reserved
* Bit **6** : operation direction
	* 0 - read
	* 1 - write
* Bit **5** : port type
	* 0 - numeric
	* 1 - analog
* all op. except numeric write
	* Bit **4** to **0** : port number on 5 bits
* numeric write
	* Bit **4** : value for numeric write operation (High=1 or Low=0)
	* Bit **3** to **0** : port number on 4 bits

#### Operations

##### Read Numeric Port

	// | 7 6 5 4 3 2 1 0 |
	//   0 0 0 0 0 1 0 0  sample for port number 4
	unsigned char operation = 0 << 5;
	unsigned char portNumber = 4;
	unsigned char command = operation & portNumber; // = 4, 0x04

##### Read Analog Port

	// | 7 6 5 4 3 2 1 0 |
	//   0 0 1 0 0 1 0 0
	unsigned char operation = 1 << 5;
	unsigned char portNumber = 4;
	unsigned char command = operation & portNumber; // = 36, 0x24

##### Write Numeric Port

	// | 7 6 5 4 3 2 1 0 |
	//   0 1 0 0 0 1 0 0  sample for numeric port number 4 to LOW
	//   0 1 0 1 0 1 0 0  sample for numeric port number 4 to HIGH
	unsigned char opLow = 4 << 4;
	unsigned char opHigh = 5 << 4;
	unsigned char portNumber = 4;
	unsigned char cmdLow = opLow & portNumber; // = 68, 0x44
	unsigned char cmdHigh = opHigh & portNumber; // = 84, 0x54

##### Write Analog Port / PWM

	// | 7 6 5 4 3 2 1 0 |
	//   0 1 1 0 0 1 0 0
	unsigned char operation = 3 << 5;
	unsigned char portNumber = 4;
	unsigned char command = operation & portNumber; // = 100, 0x64
	unsigned char data = 254;
