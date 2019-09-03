#include "mbmiddleware.h"

// Input channel
InputChannel::InputChannel():active(true){
	init();
}

void InputChannel::init(){
	int opt = 1; 

	std::cout << "Initializing the socket" << std::endl;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 

     // Forcefully attaching socket to the port 8080 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( IN_PORT ); 
     
    std::cout << "Binding socket to address" << std::endl;

    // Forcefully attaching socket to the port 8080 
    if (bind(sockfd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
}

bool InputChannel::isActive(){
	return this->active;
}

void InputChannel::start(){
	int addrlen = sizeof(address); 
	char buffer[1024] = {0};
	int valread;

	if (listen(sockfd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

    std::cout << "Input channel: waiting for connections"<< std::endl;

    while( isActive() ){
	    if ((new_socket = accept(sockfd, (struct sockaddr *)&address,  
	                       (socklen_t*)&addrlen))<0) 
	    { 
	        perror("accept"); 
	        exit(EXIT_FAILURE); 
	    } 

	    std::cout << "New message arrived!"<< std::endl;

	    while( stateMachine.process( new_socket ) != Done )
	    	usleep(100);
    } 
}

InputChannel::~InputChannel(){

}

// Input state machine

InputStateMachine::InputStateMachine():state(Data_Direction){

}

void InputStateMachine::processDataDirection( int new_socket ){
	int v;

	v = read( new_socket, buffer, 1); 
	
	if( v == 0 ){
		perror("No bytes received in the handshake");
		return;
	}

	std::cout << "Handshake received: " << int(buffer[0]) << std::endl; 

	if( buffer[0] == AM_SOURCE ){
		std::cout << "Correct! we are the target" << std::endl;
		buffer[0] = RESP_HELLO;
		send(new_socket, buffer, 1 , 0 ); 
		state = Data_Type;
	}else{
		perror("Unexpected byte mode. This has to be a target.");
	}
}

void InputStateMachine::processDataType( int new_socket ){
	int v;

	v = read( new_socket, buffer, 1); 

	if( v == 0 ){
		perror("No bytes received in the handshake");
		return;
	}

	std::cout << "Data type received" << std::endl; 

	if( buffer[0] == RESP_DATA_NUMS ){
		std::cout << "Correct data type" << std::endl;
		buffer[0] = RESP_RECVD;
		send( new_socket, buffer, 1, 0);
		state = Length;
	}else
		perror("Unsupported data type");
}

void InputStateMachine::processLength( int new_socket ){
	int v;
	dataSize = 0;
	v = read( new_socket, buffer, 4); 

	if( v != 4 ){
		perror("Incorrect number of bytes read");
		return;
	}

	dataSize = (unsigned char) buffer[0] |
			   (unsigned char) buffer[1] << 8 |
			   (unsigned char) buffer[2] << 16 |
			   (unsigned char) buffer[3] << 24;

	std::cout << "Length received: " << dataSize << std::endl; 

	doubleBuf = new double[dataSize];

	buffer[0] = RESP_RECVD;
	send( new_socket, buffer, 1, 0);
	
	state = Name;
}

void InputStateMachine::processName( int new_socket ){
	int v;
	unsigned int nameSize = 0;
	v = read( new_socket, buffer, 4); 

	if( v != 4 ){
		perror("Incorrect number of bytes read");
		return;
	}

	nameSize = (unsigned char) buffer[0] |
			   (unsigned char) buffer[1] << 8 |
			   (unsigned char) buffer[2] << 16 |
			   (unsigned char) buffer[3] << 24;

	std::cout << "Name Length received (ignoring): " << nameSize << std::endl; 

	v = read( new_socket, buffer, nameSize); 
	std::cout << "name: " << buffer << std::endl;

	buffer[0] = RESP_RECVD;
	send( new_socket, buffer, 1, 0);
	
	state = Read_Data;
	elapsed = 0;
}


void InputStateMachine::processData( int new_socket ){
	std::cout << "Processing data" << std::endl;
	int v;

	v = read( new_socket, doubleBuf, sizeof(double)*dataSize );

	if( v == 0 )
		elapsed++; 

	if( elapsed > 10 )
		state = Done;

	std::cout << "Data received: " << doubleBuf[0] << std::endl;

	buffer[0] = RESP_RECVD;
	send( new_socket, buffer, 1, 0);
}

InState InputStateMachine::process( int new_socket ){
	switch( state ){
		case Data_Direction:
			processDataDirection( new_socket );
			break;
		case Data_Type:
			processDataType( new_socket );
			break;
		case Length:
			processLength( new_socket );
			break;
		case Name:
			processName( new_socket );
			break;
		case Read_Data:
			processData( new_socket );
			break;
	}

	return state;
}


// Middleware
MammalbotMiddleware::MammalbotMiddleware(){

}

void MammalbotMiddleware::init(){
	std::cout << "Starting input channel" << std::endl;
	startInputChannel();
}

int MammalbotMiddleware::startInputChannel(){
	input = new InputChannel();

	input->start();
}

int MammalbotMiddleware::startOutputChannel(){
	// outputChannel = new OutputChannel();

	// outputChannel.start();
}