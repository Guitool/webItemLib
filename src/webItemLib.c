#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mongoose.h"

// extract from i2c-dev.h
#define I2C_SLAVE       0x0703  /* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706  /* Use this slave address, even if it
                                   is already in use by a driver! */
                                   
const char *i2cBus = "/dev/i2c-1";
                                   
char* http_code_description(int code) {
    switch(code) {
        case 100 : return "Continue";
        case 101 : return "Switching Protocols";
        case 200 : return "OK";
        case 201 : return "Created";
        case 202 : return "Accepted";
        case 203 : return "Non-Authoritative Information";
        case 204 : return "No Content";
        case 205 : return "Reset Content";
        case 206 : return "Partial Content";
        case 300 : return "Multiple Choices";
        case 301 : return "Moved Permanently";
        case 302 : return "Found";
        case 303 : return "See Other";
        case 304 : return "Not Modified";
        case 305 : return "Use Proxy";
        case 307 : return "Temporary Redirect";
        case 400 : return "Bad Request";
        case 401 : return "Unauthorized";
        case 402 : return "Payment Required";
        case 403 : return "Forbidden";
        case 404 : return "Not Found";
        case 405 : return "Method Not Allowed";
        case 406 : return "Not Acceptable";
        case 407 : return "Proxy Authentication Required";
        case 408 : return "Request Time-out";
        case 409 : return "Conflict";
        case 410 : return "Gone";
        case 411 : return "Length Required";
        case 412 : return "Precondition Failed";
        case 413 : return "Request Entity Too Large";
        case 414 : return "Request-URI Too Large";
        case 415 : return "Unsupported Media Type";
        case 416 : return "Requested range not satisfiable";
        case 417 : return "Expectation Failed";
        case 500 : return "Internal Server Error";
        case 501 : return "Not Implemented";
        case 502 : return "Bad Gateway";
        case 503 : return "Service Unavailable";
        case 504 : return "Gateway Time-out";
        case 505 : return "HTTP Version not supported";
    }
    return "Unknown";
}

// function HTTP response
void send_http_response(struct mg_connection *conn, int resultCode, char* content) {
    mg_printf(conn,
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %u\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
            resultCode,
            http_code_description(resultCode),
            (int)strlen(content),
            content);
}

int i2c_data(unsigned int i2cDevice, unsigned char i2ccommand, unsigned short* pdata) {
    int fd;
    
    // i2c bus
    if ((fd = open(i2cBus, O_RDWR)) < 0) {
        return 1;
    }

    // set address                                                                                           
    if (ioctl(fd, I2C_SLAVE, i2cDevice) < 0) {
        close(fd);
        return 2;
    }

    // send command
    if (write(fd, &i2ccommand, 1) != 1) {
        close(fd);
        return 3;
    }

    usleep(500);

     // read back data
     if (read(fd, pdata, 2) != 2) {
        close(fd);
        return 4;
     }

     unsigned char byte1 = (unsigned char)*pdata;
     unsigned char byte2 = (unsigned char)*(((unsigned char*)pdata)+1);
     printf("0x%x (%u), 0x%x : 0x%x\n", *pdata, *pdata, byte1, byte2);

    close(fd);
    
    return 0;
}

// function for GET requests processing
void GET_request_handler(struct mg_connection *conn, char* szI2CNumber, char* szPinType, char* szPinNumber) {
    int i2cDevNumber = atoi(szI2CNumber);
    int pinNumber = atoi(szPinNumber);
    
    // min max i2c value check
    if ( i2cDevNumber < 0 || i2cDevNumber >= 128 ) {
        send_http_response(conn, 416, "I2C device number not supported");
        return;
    }

    // build webItem command
    unsigned char header;
    if ( strcmp(szPinType, "A") == 0 || strcmp(szPinType, "a") == 0 ) {            // analog read request
        // check pin number
        if ( pinNumber < 0 || pinNumber >= 6 ) {
            send_http_response(conn, 416, "pin number unsupported for analog operation");
            return;
        }
        header = 32;
    } else if ( strcmp(szPinType, "D") == 0 || strcmp(szPinType, "d") == 0 ) {     // digital read request
        // check pin number
        if ( pinNumber < 0 || pinNumber >= 14 ) {
            send_http_response(conn, 416, "pin number unsupported for digital operation");
            return;
        }
        header = 0;
    } else {                                        // nothing good here
        send_http_response(conn, 416, "Pin type not supported");
        return;
    }
    
    // send i2c command
    unsigned char i2cCommand = header | pinNumber;
    unsigned short commandResult = 0;
    
    i2c_data(i2cDevNumber, i2cCommand, &commandResult);

    char response[1024];
    sprintf(response, "Arduino value for pin %d(%s), from I2C device #%d : %u\r\n", 
                        pinNumber,
                        szPinType,
                        i2cDevNumber,
                        commandResult);

    send_http_response(conn, 200, response);
}

// funtion for POST request processing
void POST_request_handler(struct mg_connection *conn, char* szI2CNumber, char* szPinType, char* szPinNumber, char* szPinValue) {
    int i2cDevNumber = atoi(szI2CNumber);
    int pinNumber = atoi(szPinNumber);
    int pinValue = atoi(szPinValue);

    // min max i2c value check
    if ( i2cDevNumber < 0 || i2cDevNumber >= 128 ) {
        send_http_response(conn, 416, "I2C device number not supported");
        return;
    }

    // build webItem command
    unsigned char header;
    if ( strcmp(szPinType, "A") == 0 || strcmp(szPinType, "a") == 0 ) {            // analog write request
        // check pin number
        if ( pinNumber < 0 || pinNumber >= 6 ) {
            send_http_response(conn, 416, "pin number unsupported for analog operation");
            return;
        }
        header = 96;
    } else if ( strcmp(szPinType, "D") == 0 || strcmp(szPinType, "d") == 0 ) {     // digital write request
        // check pin number
        if ( pinNumber < 0 || pinNumber >= 14 ) {
            send_http_response(conn, 416, "pin number unsupported for digital operation");
            return;
        }
        if ( pinValue == 0 ) {
            header = 64;
        } else {
            header = 80;
        } 
    } else {                                        // nothing good here
        send_http_response(conn, 416, "Pin type not supported");
        return;
    }
    
    // send i2c command
    unsigned char i2cCommand = header | pinNumber;
    unsigned short commandResult = 0;
    
    i2c_data(i2cDevNumber, i2cCommand, &commandResult);


    char response[1024];
    sprintf(response, "Will send you Arduino value %u for pin %u(%s), from I2C bus #%u\r\n", 
                        pinValue,
                        pinNumber,
                        szPinType,
                        i2cDevNumber);

    send_http_response(conn, 200, response);
}


// This function will be called by mongoose on every new request.
static int begin_request_handler(struct mg_connection *conn) {
    const struct mg_request_info *request_info = mg_get_request_info(conn);
    const char delimiters[] = "/";
    char *token;
    char *szUri = strdup(request_info->uri);

    // parse the URI
    int nParams = 0;
    char uriParams[4][8];
    token = strtok(szUri, delimiters);    
    while ( token && nParams <= 4 ) {
        strcpy(uriParams[nParams], token);
        //printf("%s", uriParams[nParams]);
        //printf("\n");
        
        token = strtok(NULL, delimiters);
        nParams++;
    }
    
    free(szUri);
    
    // check for method
    if ( strcmp(request_info->request_method, "GET") == 0 ) {
    	printf("web method : GET\r\n");
    	// READ port value
    	// need exactly 3 parameters
    	if ( nParams == 3 ) {
    		// call specialized READ function
    		GET_request_handler(conn, uriParams[0], uriParams[1], uriParams[2]);
    	} else {
    		// uncorrect call, return an error
    		send_http_response(conn, 404, "incorrect parameters");
    	}
    } else if ( strcmp(request_info->request_method, "POST") == 0) {
    	printf("web method : POST\r\n");
    	// WRITE port with value
    	// need exactly 4 parameters
    	if ( nParams == 4 ) {
    		// call specialized WRITE function
    		POST_request_handler(conn, uriParams[0], uriParams[1], uriParams[2], uriParams[3]);
    	} else {
    		// uncorrect call, missing or too many params, return an error
    		send_http_response(conn, 406, "incorrect parameters");
    	}
    } else {
    	printf("web method : unsupported");
    	send_http_response(conn, 501, "unsupported web method");
    }
    
    // Returning non-zero tells mongoose that our function has replied to
    // the client, and mongoose should not send client any more data.
    return 1;
}

// Main entry point
int main(void) {
  struct mg_context *ctx;
  struct mg_callbacks callbacks;

  // List of options. Last element must be NULL.
  const char *options[] = {"listening_ports", "8080", NULL};

  // Prepare callbacks structure. We have only one callback, the rest are NULL.
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.begin_request = begin_request_handler;

  // Start the web server.
  ctx = mg_start(&callbacks, NULL, options);

  // Wait until user hits "enter". Server is running in separate thread.
  // Navigating to http://localhost:8080 will invoke begin_request_handler().
  getchar();

  // Stop the server.
  mg_stop(ctx);

  return 0;
}
