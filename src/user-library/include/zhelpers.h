#ifndef _ZHELPERS_H_
#define _ZHELPERS_H_

#include <zmq.hpp>

std::string s_recv (zmq::socket_t & socket);
bool s_send (zmq::socket_t & socket, const std::string & string);
bool s_sendmore (zmq::socket_t & socket, const std::string & string);

#endif

