#ifndef _SCBR_API_H_
#define _SCBR_API_H_

#include <zmq.hpp>
#include <communication_zmq.h>
#include <string>
#include <map>

enum Action {
    SCBR_REGISTER,
    //SCBR_UNREGISTER
};

enum ComparisonOperator {
    SCBR_EQ,
    SCBR_LT,
    SCBR_GT,
    SCBR_LE,
    SCBR_GE
};

typedef std::map< std::string, std::string > PubMap;
typedef std::map< std::string, std::pair< ComparisonOperator, std::string > >
                                             SubMap;

//------------------------------------------------------------------------------
class Publication {
public:
    void attribute( const std::string &key, const std::string &value );
    void attribute( const std::string &key, double value );
    void payload( const std::string &payload );
    void encrypt_payload( const std::string &key );
    std::string serialize() const;
private:
    PubMap data_;
    std::string payload_;
};

//------------------------------------------------------------------------------
class Subscription {
public:
    Subscription( Action );
    void predicate( const std::string &key, ComparisonOperator c,
                    double );
    void predicate( const std::string &key, ComparisonOperator c,
                    const std::string &value );
    void callback(void (&)());
    std::string serialize() const;
private:
    SubMap data_;
};

//------------------------------------------------------------------------------
class Matcher {
public:
    Matcher();

    void send( const Subscription &sub );
    void send( const Publication &pub );

private:
    zmq::context_t context_;
    Communication<zmq::socket_t> comm_;
    size_t ssubcount_, spubcount_, rpubcount_;
};

#endif

