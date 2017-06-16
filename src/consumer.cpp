#include <consumer.h>
#include <message.h>
#include <sstream>
#include <crypto.h>
#include <zmq.hpp>
#include <utils.h>
#include <fstream>
#include <communication_zmq.h>
#include <argp.h>

static unsigned subcount = 0;
//------------------------------------------------------------------------------
void Consumer::subscribe( unsigned long id, const std::string &sub ) {
    std::stringstream ss;
    ss << "0 0 P i dest " << id;
    std::string h( ss.str() );

#ifndef PLAINTEXT_MATCHING
    Crypto::PubKey pubk;
    Crypto::decodeBase64PublicKey( keypath_, pubk );
    std::string p = Crypto::encrypt_rsa( pubk, sub );
#else
    std::string p = sub;
#endif

    Message m( false, h, p );
    ss.str(""); ss.clear();
    ss << m;
    comm_.send( ss.str() ); // Publish announcement of subscription

    ++subcount;
    if( subcount % 1000 == 0 ) std::cerr << ylw(subcount/1000) << " ";
#ifndef PLAINTEXT_MATCHING
    usleep(7000);
#endif
}

//------------------------------------------------------------------------------
void process_income( const std::string &s ) {
    if( !s.empty() )
        std::cout << "--> '" << s << "'\n";
}

//------------------------------------------------------------------------------
struct Arguments {
    Arguments() : address("tcp://*:5558") {}
    std::string address, input, key;
};
//------------------------------------------------------------------------------
const char *argp_program_version = "SCBR Subscriber 0.9";
const char *argp_program_bug_address = "<rafael.pires@unine.ch>";

/* Program documentation. */
static char doc[] =
  "SCBR Demo: Consumer / Client / Subscriber";
static char args_doc[] = "";
static struct argp_option options[] = {
    { "address",  'a',  "host:port", 0, "Where to listen. Default: *:5558" },
    { "input",  'i',  "filename", 0, "Subscriptions file" },
    { "key",  'k',  "filename", 0, "File containing Source (Publisher) RSA Public key" },
    { 0 }
};
//------------------------------------------------------------------------------
/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    Arguments *args = (Arguments*)state->input;
    switch(key) {
    case 'a':
        args->address = "tcp://" + std::string(arg);
        break;
    case 'i':
        args->input = arg;
        break;
    case 'k':
        args->key = arg;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    };
    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };
//------------------------------------------------------------------------------
int main(int argc, char **argv) {
    Arguments args;
    argp_parse(&argp, argc, argv, 0, 0, &args);

#ifdef PLAINTEXT_MATCHING
    printf("Encryption: \033[31mn\033[0m\n");
#else
    printf("Encryption: \033[32my\033[0m\n");

    if( args.key.empty() ) {
        std::cout << "Missing Source/Pub RSA public key (-? or --help for info)\n";
        exit(1);
    }
#endif

    if( args.input.empty() ) {
        std::cerr << "Missing <Subs-filename> (-? or --help for info)\n";
        exit(1);
    }

    std::ifstream subsfile( args.input.c_str() );
    if( !subsfile.good() ) {
        std::cerr << "Cannot open file \"" << args.input << "\"\n";
        exit(2);
    }

    zmq::context_t context(1);

    Communication<zmq::socket_t> com( context, args.address, "ConsumerID1" );
    Consumer cons( com, args.key );

    std::string line;
    while( std::getline( subsfile, line ) ) {
        size_t pos1 = line.find_first_of(' ');
        size_t pos2 = line.find_first_of(' ', pos1+1);
        line.replace( pos1+1, pos2-pos1-1, "ConsumerID1" );
        cons.subscribe( 12345, line );
    }

    std::cout << "Sent: " << subcount << " subs\n";


    while(1) {
        bool rc;
        do {
            zmq::message_t zmsg;
            if( (rc = com.socket().recv( &zmsg, ZMQ_DONTWAIT)) ) {
                process_income(std::string((const char*)zmsg.data(),zmsg.size()));
            }
        } while( rc );
        usleep(1000);
    }

   return 0;
}
