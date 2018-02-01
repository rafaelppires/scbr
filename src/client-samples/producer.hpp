#include <fstream>
#include <sstream>
#include <utils.h>
#include <crypto.h>

//------------------------------------------------------------------------------
//template <typename T>
Source::Source( unsigned long id, CommunicationBase &comm,
                const std::string &key ) :
                                      id_(id), comm_(comm), keypath_(key) {
/*
    std::ifstream file( pk.c_str() );
    std::string str, filecontent;
    if( file.is_open() )
    while( getline(file,str) ) {
        filecontent += str; filecontent += "\n";
    }

    std::cout << filecontent.size() << std::endl;
    word32 idx = 0;
    wc_InitRsaKey(&publickey,NULL);
    std::cout << wc_RsaPublicKeyDecode( (const byte*)filecontent.c_str(), &idx, &publickey, filecontent.size() ) << std::endl;
*/
}

//------------------------------------------------------------------------------
//template <typename T>
void Source::init() {
    std::cout << "source::init\n";
    std::stringstream ss;
    ss << "0 " << id_ << " S i dest = " << id_;
    Message m( false, ss.str(), "" );
    ss.str(""); ss.clear();
    ss << m;

    comm_.send( ss.str() );
    std::cout << "msg sent: " << ss.str() << std::endl;

    Crypto::decodeBase64PrivateKey(keypath_, prvk_);
}
//------------------------------------------------------------------------------
//template <typename T>
extern unsigned pubcount;
void Source::publish( const std::string &h ) {
#ifndef PLAINTEXT_MATCHING
    Message m(true, encrypt(h), h);
#else
    Message m(true, h, h);
#endif
    m.setpub(); // for statitical reasons - not intended for release
    stringstream ss; ss << m;

    comm_.send( ss.str() );

    ++pubcount;
    if( pubcount % 1000 == 0 ) std::cerr << cyn(pubcount/1000) << " ";
}

//------------------------------------------------------------------------------
//template<typename T>
std::string Source::encrypt( const std::string &plain ) {
    return Crypto::encrypt_aes( plain );
}

//------------------------------------------------------------------------------
//template <typename T>
extern unsigned subcount;
// Receive a pub that actually is a sub. Decrypts it and forward to filter with
// a diferent secret
void Source::receive_pub( const Message &m ) {
    encprofile_.begin();
    Message n( m.payload() );
#ifndef PLAINTEXT_MATCHING
    std::string subscription = 
                           encrypt( Crypto::decrypt_rsa( prvk_, n.payload() ) );
#else
    std::string subscription = n.payload();
#endif
    encprofile_.end();

    Message subconsumer(true, subscription, "");
    subconsumer.setsub();
    std::stringstream ss; ss << subconsumer;
    comm_.send( ss.str() );

    ++subcount;
    if( subcount % 1000 == 0 ) std::cerr << ylw(subcount/1000) << " ";
}

//------------------------------------------------------------------------------
