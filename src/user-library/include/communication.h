#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include <string>

class CommunicationBase {
public:
    virtual void send( const std::string & ) = 0;
};

template< typename T >
class Communication : public CommunicationBase {
public:
    Communication() {}
    void send( const std::string& );
};

void ffn(void* messagep, void* bufferp);

#endif

