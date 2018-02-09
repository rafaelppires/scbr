#ifndef _PUBSUBCO_H_
#define _PUBSUBCO_H_

#include <string>
#include <message.h>
#include <prefilter.hh>
#include <zmq.hpp>
#include <utils.h>

extern uint64_t global_eid;

class PubSubCo {
public:
	PubSubCo( zmq::socket_t &com, int v = 0 ) : 
                             comm_(com), pcount_(0), scount_(0), verbosity_(v) {
        ecall_pubstats.init("pubs.log");
        ecall_substats.init("subs.log");
        printf("SCBR logs opened\n");
    }

    void pubsub_message( const std::string &pub );
	void forward( const std::string &to, const std::string &header,
                                         const std::string &hash );
    void print_stats();

private:
    void open_matching( const Message& );
    void enclave_matching( const Message& );
	
    int verbosity_;
    viper::Graph mgraph_;
	zmq::socket_t &comm_;
	std::string last_payload_;
    Logger ecall_pubstats, ecall_substats;
    unsigned pcount_, scount_;
    std::map<std::string,std::string> data_table_;
};

#endif

