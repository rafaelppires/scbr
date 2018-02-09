#ifndef _SCBR_API_H_
#define _SCBR_API_H_

#include <zmq.hpp>
#include <communication_zmq.h>
#include <string>
#include <map>
#include <thread>

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

typedef std::function<void(std::string)> PubCallback;
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
    void set_callback( PubCallback );
    PubCallback callback() const;
    std::string serialize( std::string &hash ) const;
private:
    SubMap data_;
    PubCallback callback_;
};

//------------------------------------------------------------------------------
class Matcher {
public:
    Matcher( std::string addr, std::string id );
    void terminate();

    void send( const Subscription &sub );
    void send( const Publication &pub );
    void send( const std::string &str );

private:
    void establish_communication();
    void receive_polling();

    std::string id_, addr_;
    std::thread recv_polling_;
    zmq::context_t context_;
    zmq::socket_t outsock_;
    size_t ssubcount_, spubcount_, rpubcount_;
    std::map< std::string, PubCallback > callbacks_;
};

#endif

