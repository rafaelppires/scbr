#ifndef _CONSUMER_H_
#define _CONSUMER_H_

#include <communication.h>
#include <subscription.hh>
using viper::Subscription;

class Consumer {
public:
    Consumer( CommunicationBase &comm,
              const std::string &key ) : comm_(comm), keypath_(key) {}
    void subscribe( unsigned long id, const std::string &sub );

    CommunicationBase &comm_;
    std::string keypath_;
};

#endif
