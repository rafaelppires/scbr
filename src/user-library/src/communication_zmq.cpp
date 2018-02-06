#include <communication_zmq.h>
#include "zhelpers.hpp"

//------------------------------------------------------------------------------
Communication<zmq::socket_t>::Communication( zmq::context_t &c,
                   const std::string &addr, const std::string &id, bool bind ) :
                                               id_(id), sender_(c, ZMQ_DEALER) {
    if( id != "" ) sender_.setsockopt( ZMQ_IDENTITY, id.c_str(), id.size() );
    if( bind ) {
        sender_.bind( addr.c_str() );
        std::cout << "zeromq bound source " << addr << std::endl;
    } else {
        sender_.connect( addr.c_str() );
        std::cout << "zeromq connect source " << addr << std::endl;
    }
}

//------------------------------------------------------------------------------
void Communication<zmq::socket_t>::send( const std::string &s ) {
  //s_sendmore( sender_, id_ );
  s_sendmore( sender_, "" );
  s_send( sender_, s );
}

//------------------------------------------------------------------------------
void Communication<zmq::socket_t>::send( const Message &m ) {
    std::stringstream ss;
    ss << m;
    send( ss.str() );
}

//------------------------------------------------------------------------------

