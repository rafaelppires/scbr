#include <zmq.hpp>
#include <iostream>
#include <unistd.h> // for usleep
#include <zhelpers.hpp>
#include <communication_zmq.h>
#include <source.h>
#include <signal.h>
#include <argp.h>
//------------------------------------------------------------------------------
void process_income( Source &src, const std::string &s ) {
    std::stringstream ss;
    ss.str( s );

}

Source *src = 0;
//------------------------------------------------------------------------------
void ctrlc_handler( int s ) {
    printf("\n");
    if(src) src->encprofile_.print_summary();
    printf("(%d) Goodbye!\n", s);
    exit(1);
}


//------------------------------------------------------------------------------
struct Arguments {
    Arguments() : address("tcp://*:5557") {}
    std::string address, input, key;
};
//------------------------------------------------------------------------------
const char *argp_program_version = "SCBR Publisher 0.9";
const char *argp_program_bug_address = "<rafael.pires@unine.ch>";

/* Program documentation. */
static char doc[] =
  "SCBR Demo: Source / Producer / Publisher";
static char args_doc[] = "";
static struct argp_option options[] = {
    { "address",  'a',  "host:port", 0, "Where to listen. Default: *:5557" },
    { "input",  'i',  "filename", 0, "Publications file" },
    { "key",  'k',  "filename", 0, "File containing RSA Private key" },
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
unsigned subcount = 0, pubcount = 0;
//------------------------------------------------------------------------------
int main( int argc, char **argv ) {
    Arguments args;
    argp_parse(&argp, argc, argv, 0, 0, &args);

#ifdef PLAINTEXT_MATCHING
    printf("Encryption: \033[31mn\033[0m\n");
#else
    printf("Encryption: \033[32my\033[0m\n");

    if( args.key.empty() ) {
        std::cout << "Missing RSA private key (-? or --help for info)\n";
        exit(1);
    }
#endif

    zmq::context_t context(1);

    if( args.input.empty() ) {
        std::cout << "Missing input <Publications file> (-? or --help for info)\n";
        exit(0);
    }

    std::ifstream pubsfile( args.input.c_str() );
    if( !pubsfile.good() ) {
        std::cerr << "Unable to succesfully open input file \""
                  << args.input << "\"\n";
        exit(2);
    }

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrlc_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT,&sigIntHandler,NULL);

    Communication<zmq::socket_t> c( context, args.address, "12345" );
    src = new Source( 12345, c, args.key ); // id = 12345
    Source &source = *src;
    source.init();

    bool recvd_sth = false;
    unsigned cnt = 0;
    while(1) {
        bool rc;
        do {
            // Receives Consumer's encrypted publications that contains 
            // subscriptions. Source's role is to decrypt it and
            // encrypt again with secret shared only with Filter
            zmq::message_t zmsg;
            if( (rc = c.socket().recv( &zmsg, ZMQ_DONTWAIT)) ) {
               // ignore 0 msg
                source.receive_pub( Message(false, "", s_recv(c.socket())) );
                recvd_sth = true;
                cnt = 0;
            }
        } while( rc );
        usleep(1000);

        // After receiving all subscriptions, waits a while and start
        // doing its job. 
        if( recvd_sth && ++cnt == 1000 ) {
            recvd_sth = false;
            cnt = 0;

            //char payload[] = "1234567890abcdefghijklmnopqrstuvwxyz";
            //unsigned psz = sizeof(payload);
            srand(time(0));
            std::string line;
            while( std::getline(pubsfile, line) ) {
                source.publish(line);
            }
            std::cout << "Subs: " << subcount << " Pubs: " << pubcount << "\n";
        }
    }

    return 0;
}
