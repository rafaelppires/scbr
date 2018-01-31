#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <event.hh>
#include <pubsubco.h>
#include <communication.h>
#include <crypto.h>
using viper::Event;

class SourceBase {
public:
    virtual void receive_pub( const Message &m ) = 0;
};

class Source : public SourceBase {
public:
    Source( unsigned long id, CommunicationBase &comm, const std::string & );

    void init();
    void publish( const std::string &h );
    virtual void receive_pub( const Message &m );
    std::string encrypt( const std::string &plain );

    //PublicKey publickey;
    std::string keypath_;
    CommunicationBase &comm_;
    unsigned long id_;  
    Crypto::PrvKey prvk_;
    Stats encprofile_; 
};

#include <source.hpp>

#endif
