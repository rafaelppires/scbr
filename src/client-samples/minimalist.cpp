#include <iostream>
#include <zmq.hpp>
#include <zhelpers.h>

zmq::context_t c;

int main() {
    zmq::socket_t socket( c, ZMQ_DEALER );
    socket.setsockopt( ZMQ_IDENTITY, "54321", 5 );
    socket.bind( "tcp://*:6666" );

    s_sendmore( socket, "" );
    s_send( socket, "5 ADq/Q7Baw0ul4D0XLcKTGk4RkbCGPM+FgpuSg/bMDj7tza9mZfsTmIDpm3x1+/Y1P8GBDVc+|Mi9G/A3wz7IN43//5lQ= " );
    s_sendmore( socket, "" );
    s_send( socket, "3 1gvSGAWUjTOECsRxgTsFUh6wdUWwFqdq4ZVNsWGVwmGwnbYR7Ms91XplC3YJ5ru4yOVfapXV|phqwv52fmSp9ySHoPAI4v8zcrtyDUf4DgaU+INVzfg== UmVwb3J0IGFib3V0IHRoZSBzdG9jaw==" );
    s_sendmore( socket, "" );
    s_send( socket, "3 IAs1W9YZjExaUIN2Aq5bSGuIcchdFJKPkwV6Xq9Drx0EvXtDdRoD4aB/GYh4Hvud0+aGXjuL|vRpmmEEj4PeFVa6hVcZenJ3RkEjv/VapmWoABhWC QW5vdGhlciByZXBvcnQgYWJvdXQgdGhlIHN0b2Nr" );

    while( true ) {
        zmq::message_t zmsg;
        socket.recv( &zmsg );
        if( zmsg.size() == 0 ) continue;
        std::cout << std::string(zmsg.data<char>(),zmsg.size()) << "\n";
        break;
    }
}

