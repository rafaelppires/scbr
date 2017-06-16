#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <string>

//------------------------------------------------------------------------------
std::string& trim_right_inplace( std::string &s,
                               const std::string& delimiters = " \f\n\r\t\v" );
std::string& trim_left_inplace( std::string &s,
                               const std::string& delimiters = " \f\n\r\t\v" );
std::string& trim_inplace( std::string &s,
                           const std::string& delimiters = " \f\n\r\t\v" );
//------------------------------------------------------------------------------
enum ErrorCodes{
    mNO_ERROR = 0,
    mEMPTY_MSG = -1
};

//------------------------------------------------------------------------------
#define ENC_HEADER 1
//------------------------------------------------------------------------------
// Statistical reason only - Not supposed to be used in production
#define PUB_BIT    2
#define SUB_BIT    4
//------------------------------------------------------------------------------

struct Message {
    unsigned short henc_;
    std::string header_;
    std::string payload_;
    ErrorCodes error_;

    Message( const std::string &s);
    Message( bool he, const std::string &h, const std::string &p );
    
    std::string header() const;
    std::string payload() const;
    void setpub();
    void setsub();
    bool is_enc() const { return henc_ & ENC_HEADER; }
    bool is_pub() const { return henc_ & PUB_BIT; } // stats - not for release
    bool is_sub() const { return henc_ & SUB_BIT; } // stats - not for release
    int parse( const std::string &s );
    bool bad();

    std::string error_string();
    std::string to_string() const;

    template < typename Stream >
    friend Stream& operator<< ( Stream &out, const Message &m ) {
        out << m.henc_ << " " << m.header_ << " " << m.payload_;
        return out;
    }
private:
    void init_common();
};


#endif

