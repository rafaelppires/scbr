#include <communication.h>
#include <zmq.hpp>
#include <message.h>

//------------------------------------------------------------------------------
template<>
class Communication<zmq::socket_t> : public CommunicationBase {
public:
    Communication( zmq::context_t &c, const std::string &addr,
                   const std::string &id = "", bool bind = true );
    void send( const std::string &s );
    void send( const Message &m );
    zmq::socket_t& socket() { return sender_; }
private:
    zmq::socket_t sender_;
    std::string id_;
};

//------------------------------------------------------------------------------

