#ifndef MBMIDDLEWARE_H
#define MBMIDDLEWARE_H

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <unistd.h>


#define IN_PORT 50091
#define OUT_PORT 50092

#define RESP_DATA_NUMS      31
#define RESP_DATA_SPIKES    32
#define RESP_DATA_IMPULSES  33
#define RESP_HELLO          41
#define RESP_RECVD          42
#define RESP_ABORT          43
#define RESP_FINISHED       44
#define AM_SOURCE           45
#define AM_TARGET           46
#define NOT_SET             99


enum InState{ Data_Direction, 
	        Data_Type, 
	        Length, 
	        Name,
	    	Read_Data,
	    	Done};

class InputStateMachine{
private:
	InState state;
	char buffer[16];
	unsigned int dataSize;
	double *doubleBuf;
	int elapsed;
public:
	InputStateMachine();
	InState process( int new_socket );
	void processDataDirection( int new_socket );
	void processName( int new_socket );
	void processDataType( int new_socket );
	void processLength( int new_socket );
	void processData( int new_socket );
};

class InputChannel{
private:
	int sockfd, new_socket;
	struct sockaddr_in address;
    bool active; 
    InputStateMachine stateMachine;

public:
	InputChannel();
	void start();
	void init();
	bool isActive();
	~InputChannel();
};

class OutputChannel{

};

class MammalbotMiddleware{
private:
	InputChannel *input;
	OutputChannel *output;

	int startInputChannel();
	int startOutputChannel();
public:
	MammalbotMiddleware();
	void init();
};

#endif