#include <scbr_api.h>
#include <sgx_cryptoall.h>
#include <sgx_utils_rp.h>
#include <sstream>
#include <iostream>
#include <thread>
#include <zhelpers.h>

//--------------------- PUBLICATION --------------------------------------------
void Publication::attribute( const std::string &key, 
                             const std::string &value ) {
    data_[key] = Crypto::base64( value );
    std::replace( data_[key].begin(), data_[key].end(), '\n', '|');
}

//------------------------------------------------------------------------------
void Publication::attribute( const std::string &key, double value ) {
    data_[key] = std::to_string( value );
}

//------------------------------------------------------------------------------
void Publication::payload( const std::string &payload ) {
    payload_ = payload;
}

//------------------------------------------------------------------------------
void Publication::encrypt_payload( const std::string &key ) {
    Crypto::encrypt_aes_inline( key, payload_ );
}

//------------------------------------------------------------------------------
std::string Publication::serialize() const {
    std::string header = "0 0 P";
    for( auto const &kv : data_ ) {
        header += " i " + kv.first + " " + kv.second;
    }

    printinfo( LLLOG, "-> %s\n", header.c_str() );
#ifndef PLAINTEXT_MATCHING
    Crypto::encrypt_aes_inline( "_header_key_", header );
#endif
    Message m(true, header, payload_);
    m.setpub(); // statistics only, not for release
    std::stringstream ss; ss << m;

    return ss.str();
}

//--------------------- SUBSCRIPTION -------------------------------------------
Subscription::Subscription( Action ) {

}

//------------------------------------------------------------------------------
void Subscription::predicate( const std::string &key, ComparisonOperator c,
                              double value ) {
    data_[key] = std::make_pair( c, std::to_string(value) );
}

//------------------------------------------------------------------------------
void Subscription::predicate( const std::string &key, ComparisonOperator c,
                              const std::string &value ) {
    data_[key] = std::make_pair( c, Crypto::base64(value) );
    std::replace( data_[key].second.begin(),
                  data_[key].second.end(), '\n', '|');
}

//------------------------------------------------------------------------------
void Subscription::set_callback( PubCallback cb ) {
    callback_ = cb;
}

//------------------------------------------------------------------------------
PubCallback Subscription::callback() const {
    return callback_;
}

//------------------------------------------------------------------------------
std::string Subscription::serialize( std::string &headerhash ) const {
    std::string header = "0 54321 S";
    for( auto const &kv : data_ ) {
        header += " i " + kv.first + " ";     // key
        switch( kv.second.first ) {           // comparison operator
        case SCBR_EQ: header += "= ";  break;
        case SCBR_LT: header += "< ";  break;
        case SCBR_GT: header += "> ";  break;
        case SCBR_LE: header += "<= "; break;
        case SCBR_GE: header += ">= "; break;
        } 
        header += kv.second.second;           // value
    }

    printinfo( LLLOG, "-> %s\n", header.c_str() );
#ifndef PLAINTEXT_MATCHING
    Crypto::encrypt_aes_inline( "_header_key_", header );
#endif
    Message m(true, header, "");
    m.setsub();
    headerhash = Crypto::sha256(header);

    std::stringstream ss; ss << m;
    return ss.str();
}

//--------------------- ROUTER INTERFACE ---------------------------------------
Matcher::Matcher( std::string addr, std::string id ) : id_(id),
                            addr_( "tcp://" + addr ),
                            ssubcount_(0), spubcount_(0), rpubcount_(0), 
                            context_(1), outsock_(context_,ZMQ_PUSH),
                            recv_polling_(&Matcher::receive_polling, this) {
    set_logmask( ~0 );
    set_rand_seed();   
    outsock_.bind( "inproc://remote" );
}

//------------------------------------------------------------------------------
// Receives publications that matched previous subscriptions
//------------------------------------------------------------------------------
void Matcher::receive_polling() {
    zmq::socket_t outside( context_, ZMQ_DEALER ), 
                  inside( context_, ZMQ_PULL );

    // set socket options
    int linger = 1;
    outside.setsockopt( ZMQ_LINGER, &linger, sizeof(linger) );
    outside.setsockopt( ZMQ_IDENTITY, id_.c_str(), id_.size() );
    inside.setsockopt( ZMQ_LINGER, &linger, sizeof(linger) );

    // bind or connect
    outside.bind( addr_ );
    inside.connect( "inproc://remote" );

    // start polling
    zmq_pollitem_t items[] = { { (void*)outside, 0, ZMQ_POLLIN, 0 },
                               { (void*)inside,  0, ZMQ_POLLIN, 0 } };
    try{
        while (1) {
            zmq::message_t zmsg;
            zmq::poll( items, sizeof(items)/sizeof(zmq_pollitem_t), -1 );

            if( items[0].revents & ZMQ_POLLIN ) {
                outside.recv( &zmsg );
                if( zmsg.size() == 0 ) continue;
                ++rpubcount_;
                std::string content(zmsg.data<char>(),zmsg.size());
                std::string hash( content.substr(0,32) );
                content = content.substr(32);

                // Call the right callback based on SUB hash
                auto it = callbacks_.find( hash );
                if( it != callbacks_.end() ) 
                    it->second( content );

            } else if( items[1].revents & ZMQ_POLLIN ) {
                inside.recv( &zmsg );
                std::string content( zmsg.data<char>(), zmsg.size() );
                if( content == "*die*" ) break;
                if( zmsg.size() == 0 )
                    s_sendmore( outside, content );
                else
                    s_send( outside, content );
            }
        }
    } catch( zmq::error_t e ) {
        printf("error_t\n");
    }
}

//------------------------------------------------------------------------------
void Matcher::send( const Subscription &sub ) {
    std::string sha, subscription = sub.serialize( sha );
    send( subscription );
    callbacks_[sha] = sub.callback();
    ++ssubcount_;
}

//------------------------------------------------------------------------------
void Matcher::send( const Publication &pub ) {
    send( pub.serialize() );
    ++spubcount_;
}

//------------------------------------------------------------------------------
void Matcher::send( const std::string &str ) {
    s_sendmore( outsock_, "" );
    s_send( outsock_, str );
}

//------------------------------------------------------------------------------
void Matcher::terminate() {
    s_send( outsock_, "*die*" );
    recv_polling_.join();
    outsock_.close();
}

//------------------------------------------------------------------------------

