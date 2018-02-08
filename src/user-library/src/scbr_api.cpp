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

}

//------------------------------------------------------------------------------
std::string Publication::serialize() const {
//return "cachaça";
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
std::string Subscription::serialize() const {
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
    std::stringstream ss; ss << m;

    return ss.str();
}

//--------------------- ROUTER INTERFACE ---------------------------------------
Matcher::Matcher() : ssubcount_(0), spubcount_(0), rpubcount_(0), 
                     terminated_(false), context_(1), 
                     comm_(context_, "tcp://*:6666", "54321") {
    set_logmask( ~0 );
    set_rand_seed();    
    std::thread t( &Matcher::receive_polling, this );
    t.detach();
}

//------------------------------------------------------------------------------
#include <unistd.h>
void Matcher::receive_polling() {
    bool finished;
    struct zmq_pollitem_t pi[] = { {(void*)comm_.socket(), 0, ZMQ_POLLIN, 0} };
    while( true ) {
        {
            std::unique_lock<std::mutex> lock(term_mtx_);
            finished = terminated_;
        }
        if( finished ) break;
        int rc = zmq_poll( pi, 1, 1000 ); // each 1ms
        if( rc == 0 ) continue;

        zmq::message_t zmsg;
        comm_.socket().recv( &zmsg );
        if( zmsg.size() == 0 ) continue;
        if( callbacks_.size() ) 
            callbacks_.begin()->second
                          ( std::string((const char*)zmsg.data(),zmsg.size()) );
    }
}

//------------------------------------------------------------------------------
void Matcher::send( const Subscription &sub ) {
    std::string subscription = sub.serialize(),
                sha = Crypto::sha256( subscription );
    comm_.send( subscription );
    callbacks_[sha] = sub.callback();
    ++ssubcount_;
}

//------------------------------------------------------------------------------
void Matcher::send( const Publication &pub ) {
    comm_.send( pub.serialize() );
    ++spubcount_;
}

//------------------------------------------------------------------------------
void Matcher::terminate() {
    using namespace std::chrono_literals;
    {
        std::unique_lock<std::mutex> lock(term_mtx_);
        terminated_ = true;
    }
    std::this_thread::sleep_for(2ms);
}

//------------------------------------------------------------------------------

