#include "mbmiddleware.h"

// Input channel
Channel::Channel( int port, char *name ):active(true), stateMachine(this), port(port){
	init();
	done = false;
	this->name = name;
}

void Channel::init(){
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
    address.sin_port = htons( port ); 
     
    std::cout << "Binding socket to address" << std::endl;

    // Forcefully attaching socket to the port 8080 
    if (bind(sockfd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
}

bool Channel::isActive(){
	return this->active;
}

void Channel::start(){
	int addrlen = sizeof(address); 
	int valread;

	if (listen(sockfd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

    std::cout << "Channel: waiting for connections on port "<< port << std::endl;

    while( isActive() ){
	    if ((new_socket = accept(sockfd, (struct sockaddr *)&address,  
	                       (socklen_t*)&addrlen))<0) 
	    { 
	    	std::cout << ">>> Error when accepting connections" << std::endl;
	        perror("accept"); 
	        exit(EXIT_FAILURE); 
	    } 

	    done = false;
	    stateMachine.reset();

	    while( stateMachine.process( new_socket ) != Done )
	    	usleep(100);

	    close(new_socket);
    	std::cout << "Done with processing in " << this->name << std::endl;
	    done = true;
    } 
}

void Channel::createBuffer( unsigned int dataSize ){
	this->doubleBuf = new double[dataSize];
	this->dataSize = dataSize;
}

bool Channel::isValid( int type ){
	if( type == this->channelType )
		return true;
	return false;
}

Channel::~Channel(){

}

// Input state machine

InputStateMachine::InputStateMachine( Channel *c ):state(Data_Direction){
	this->channel = c;
}

void InputStateMachine::reset(){
	state = Data_Direction;
}

void InputStateMachine::processDataDirection( int new_socket ){
	int v;

	v = read( new_socket, buffer, 1); 
	
	if( v == 0 ){
		perror("No bytes received in the handshake");
		return;
	}

	std::cout << "Handshake received: " << int(buffer[0]) << std::endl; 

	if( channel->isValid(buffer[0]) ){
		std::cout << "Correct type" << std::endl;
		buffer[0] = RESP_HELLO;
		send(new_socket, buffer, 1 , 0 ); 
		state = Data_Type;
	}else{
		perror("Unexpected byte mode. E.g. using the ouput channel as an input.");
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
	unsigned int dataSize = 0;
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

	channel->createBuffer( dataSize );

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
	channel->elapsed = 0;
}

void InputStateMachine::processData( int new_socket ){
	std::cout << "Processin data! " << std::endl;
	state = channel->processData( new_socket );
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

// Input channel
InputChannel::InputChannel( OutputChannel *o ):Channel( IN_PORT, "Input channel" ){
	this->output = o;
	this->channelType = AM_SOURCE;
}

InState InputChannel::processData( int new_socket ){
	std::cout << "Processing data" << std::endl;
	int v;

	v = read( new_socket, doubleBuf, sizeof(double)*dataSize );

	if( v == 0 )
		elapsed++; 

	if( elapsed > 10 )
	{
		std::cout << "Connection timed out. Done!" << std::endl;		
		return Done;
	}

	std::cout << "Data received: " << doubleBuf[0] << std::endl;

	mtx.lock();
	output->notify( doubleBuf[0] );
	mtx.unlock();
	std::cout << "Sending response"<< std::endl;
	// Error when connection closed?
	char buffer[1] = {RESP_RECVD};
	int r = send( new_socket, buffer, 1, MSG_NOSIGNAL);
	std::cout << "Data send with code: " << r << std::endl;

	return Read_Data;
}

// Output channel
OutputChannel::OutputChannel():Channel( OUT_PORT, "Output channel" ){
	this->channelType = AM_TARGET;
	ack = true;
	// data = {1, 2, 3};
}

InState OutputChannel::processData( int new_socket ){
	std::cout << "Ready to send data" << std::endl;
	int v;
	char respBuf[16];

	if( !ack )
	{

		v = read( new_socket, respBuf, 1 );

		if( v > 0 ){
			ack = true;
			elapsed = 0;
		}else
			elapsed++;

		if( int(respBuf[0]) < 0 ){
			std::cout << "Simulation done: " << int(respBuf[0]) << std::endl;
				return Done;
		}else
			std::cout << "Ack received: " << int(respBuf[0]) << std::endl;
	}

	double *buffer = new double[1];

	mtx.lock();
	if( this->data.empty() ){
		if( ack )
			buffer[0] = 0; // Pad with zers
		else if( elapsed > 10 )
			return Done;
		else
			return Read_Data;
	}else{
		buffer[0] = data.front();	
		data.pop_front();
	}	
	mtx.unlock();

	std::cout << "Sending data back to the other model: " << buffer[0] << std::endl;

	int r = send( new_socket, buffer, sizeof(double), MSG_NOSIGNAL);

	ack = false;

	if( r < 0 )
		return Done;
	else	
		return Read_Data;
}

void OutputChannel::notify( double d ){
	std::cout << "Output notified with the datum: " << d << std::endl;

	data.push_back( d );
}



// Middleware
MammalbotMiddleware::MammalbotMiddleware(){

}

void MammalbotMiddleware::init(){
	std::cout << "Starting output channel" << std::endl;
	output = new OutputChannel();
	std::thread outThread( &OutputChannel::start, output );

	std::cout << "Starting input channel" << std::endl;
	input = new InputChannel( output );
	std::thread inThread( &InputChannel::start, input  );

	inThread.join();
	outThread.join();

}


MammalbotMiddleware::~MammalbotMiddleware(){
	delete input;
	delete output;
}