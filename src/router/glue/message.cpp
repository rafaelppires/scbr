#include <message.h>
#include <sstream>
#undef ENABLE_SGX
#include <sgx_cryptoall.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>

//------------------------------------------------------------------------------
std::string& trim_right_inplace( std::string &s,
                               const std::string& delimiters ) {
  return s.erase( s.find_last_not_of( delimiters ) + 1 );
}

//------------------------------------------------------------------------------
std::string& trim_left_inplace( std::string &s,
                               const std::string& delimiters ) {
  return s.erase( 0, s.find_first_not_of( delimiters ) );
}

//------------------------------------------------------------------------------
std::string& trim_inplace( std::string &s,
                           const std::string& delimiters ) {
  return trim_left_inplace( trim_right_inplace( s, delimiters ), delimiters );
}

//------------------------------------------------------------------------------
void Message::init_common() {
    error_ = mNO_ERROR;
}

//------------------------------------------------------------------------------
Message::Message( bool he, const std::string &h, const std::string &p )  {
    init_common();
    henc_ = he ? ENC_HEADER : 0;
    
    CryptoPP::StringSource s1( h, true, new CryptoPP::Base64Encoder(
                               new CryptoPP::StringSink(header_) ));
    trim_inplace( header_ );
    std::replace( header_.begin(), header_.end(), '\n', '|');

    CryptoPP::StringSource s2( p, true, new CryptoPP::Base64Encoder(
                               new CryptoPP::StringSink(payload_) ));
    trim_inplace( payload_ );
    std::replace( payload_.begin(), payload_.end(), '\n', '|');
}

//------------------------------------------------------------------------------
Message::Message( const std::string &s) {
    init_common();
    parse(s);
}

//------------------------------------------------------------------------------
void Message::setpub() {
    henc_ |= PUB_BIT;
}

//------------------------------------------------------------------------------
void Message::setsub() {
    henc_ |= SUB_BIT;
}

//------------------------------------------------------------------------------
bool Message::bad() {
    return error_ != mNO_ERROR;
}

//------------------------------------------------------------------------------
std::string Message::error_string() {
    switch( error_ ) {
    case mEMPTY_MSG: return "Empty message";
    default: return "No error";
    };
}

//------------------------------------------------------------------------------
int Message::parse( const std::string &s ) {
    //std::cout << "Raw msg: " << s << std::endl;

    // sanity check
    if( s.empty() ) return (error_ = mEMPTY_MSG);

    // parsing
    std::stringstream ss;
    ss.str(s);
    ss >> henc_  >> header_ >> payload_;
}

//------------------------------------------------------------------------------
std::string Message::to_string() const {
    std::stringstream ret, s_in, s_out;
    std::string header, payload;

    CryptoPP::StringSource s1( header_, true, new CryptoPP::Base64Decoder(
                                        new CryptoPP::StringSink( header ) ));

    CryptoPP::StringSource s2( payload_, true, new CryptoPP::Base64Decoder(
                                        new CryptoPP::StringSink( payload ) ));

    ret << "Enc: " << henc_ << "\n"
        << "Header: '" << Crypto::printable(header) << "'\n"
        << "Payload: '" << Crypto::printable(payload) << "'\n";
    return ret.str();
}

//------------------------------------------------------------------------------
std::string Message::payload() const {
    std::string ret;
    CryptoPP::StringSource s2( payload_, true, new CryptoPP::Base64Decoder(
                                        new CryptoPP::StringSink( ret ) ));
    return ret;
}

//------------------------------------------------------------------------------
std::string Message::header() const {
    std::string ret;
    CryptoPP::StringSource s2( header_, true, new CryptoPP::Base64Decoder(
                                        new CryptoPP::StringSink( ret ) ));
    return ret;
}

//------------------------------------------------------------------------------
