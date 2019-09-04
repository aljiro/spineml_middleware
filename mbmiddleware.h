#ifndef MBMIDDLEWARE_H
#define MBMIDDLEWARE_H

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <unistd.h>
#include <deque>

#include <pthread.h>
#include <thread>
#include <mutex>

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

using namespace std;

static std::mutex mtx;

enum InState{ Data_Direction, 
	        Data_Type, 
	        Length, 
	        Name,
	    	Read_Data,
	    	Done};

class Channel;

class InputStateMachine{
private:
	InState state;
	char buffer[16];
	Channel *channel;

public:
	InputStateMachine( Channel* c );
	InState process( int new_socket );
	void reset();
	void processDataDirection( int new_socket );
	void processName( int new_socket );
	void processDataType( int new_socket );
	void processLength( int new_socket );
	void processData( int new_socket );
};

class Channel{
private:
	int sockfd, new_socket, port;
	struct sockaddr_in address;
    bool active; 

    InputStateMachine stateMachine;
protected:
	char *name;
	int channelType;
	double *doubleBuf;
    unsigned int dataSize;
public:
	bool done;
	int elapsed;

	Channel( int port, char *name );
	void start();
	void init();
	bool isActive();
	bool isValid( int type );
	void createBuffer( unsigned int dataSize );

	virtual InState processData( int new_socket ) = 0;

	~Channel();
};

class OutputChannel : public Channel{
private:
	deque<double> data;
	bool ack;
public:
	OutputChannel();
	void notify( double data );
	InState processData( int new_socket );
};


class InputChannel : public Channel{
private:
	OutputChannel *output;
public:
	InputChannel( OutputChannel* output );
	InState processData( int new_socket );
};

class MammalbotMiddleware{
private:
	InputChannel *input;
	OutputChannel *output;
public:
	MammalbotMiddleware();
	void init();
	~MammalbotMiddleware();
};

#endif