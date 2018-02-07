#include <scbr_api.h>
#include <sgx_cryptoall.h>
#include <sstream>
#include <iostream>

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
    std::string header = "0 0 P";
    for( auto const &kv : data_ ) {
        header += " i " + kv.first + " " + kv.second;
    }
std::cout << header << std::endl;
#ifndef PLAINTEXT_MATCHING
    header = Crypto::encrypt_aes( header );
#endif
    Message m(true, header, payload_);
    m.setpub(); // statistics only, not for release
    std::stringstream ss; ss << m;
std::cout << ss.str() << std::endl;
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
    data_[key] = std::make_pair( c, value );
}

//------------------------------------------------------------------------------
void Subscription::callback( void (&)() ) {

}

//------------------------------------------------------------------------------
std::string Subscription::serialize() const {
    return "";
}

//--------------------- ROUTER INTERFACE ---------------------------------------
Matcher::Matcher() : ssubcount_(0), spubcount_(0), rpubcount_(0), context_(1), 
                     comm_(context_, "tcp://*:6666", "54321") {
    
}

//------------------------------------------------------------------------------
void Matcher::send( const Subscription &sub ) {
    comm_.send( sub.serialize() );
    ++ssubcount_;
}

//------------------------------------------------------------------------------
void Matcher::send( const Publication &pub ) {
    comm_.send( pub.serialize() );
    ++spubcount_;
}

//------------------------------------------------------------------------------

