#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pwd.h>
#include <libgen.h>
#include <stdlib.h>
#include <argp.h>

#define MAX_PATH FILENAME_MAX


#include <sgx_urts.h>
#include <sgx_error.h>
#include "scbr.h"

#ifndef NONENCLAVE_MATCHING
#include "CBR_Enclave_Filter_u.h"
#include <sgx_initenclave.h>
#endif


/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* OCall functions */
void ocall_print(const char *str) {
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("\033[96m%s\033[0m", str);
}
//------------------------------------------------------------------------------
#undef ENABLE_SGX
#include <sgx_cryptoall.h>
#include <pubsubco.h>
PubSubCo *pubsub = 0;
void ocall_added_notify( const char *b, const char *hdr, size_t len ) {
    //std::cout << "OCALL: [" << hdr << "] " << Crypto::printable(b) << std::endl;
    if( pubsub ) pubsub->forward( b, std::string(hdr,len) );
}

//------------------------------------------------------------------------------
#if NONENCLAVE_MATCHING
#include <CBR_Enclave_Filter.cpp>
#endif
//------------------------------------------------------------------------------
struct Arguments {
    Arguments() : verbose(0), address("tcp://*:5555") {}
    std::string address;
    std::vector<std::string> multconnect;
    int verbose;
};
//------------------------------------------------------------------------------
#include <zhelpers.hpp>
#include <utils.h>
void ctrlc_handler(int);
void legacymain( const Arguments &args ) {

    zmq::context_t context(1);
    zmq::socket_t broker( context, ZMQ_ROUTER );

    if( args.multconnect.empty() ) {
        std::cout << "SCBR is listening...\n";
        broker.bind( args.address.c_str() );
    } else {
        for( auto const &addr : args.multconnect ) {
            std::cout << "Connecting to " << addr << std::endl;
            broker.connect( addr.c_str() );
        }
    }

    pubsub = new PubSubCo( broker, args.verbose );

    FILE* f = fopen("/tmp/scbr-run", "w");
    if (f) fclose(f);

    while (1) {
        std::string s1 = s_recv( broker );
        std::string s2 = s_recv( broker );
        std::string s3 = s_recv( broker );

        pubsub->pubsub_message( s3 );
    }

}

//------------------------------------------------------------------------------
void ctrlc_handler( int s ) {
    if( pubsub ) pubsub->print_stats();

    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int ecall_return = 0;

#ifdef NONENCLAVE_MATCHING
    ret = SGX_SUCCESS;
    ecall_CBR_Enclave_Filter_sample();
#else
    ret = ecall_CBR_Enclave_Filter_sample(global_eid, &ecall_return);
#endif
    if (ret != SGX_SUCCESS)
        abort();

    if (ecall_return == 0) {
      printf("SCBR application ran with success\n");
    } else {
        printf("SCBR application failed %d \n", ecall_return);
    }
    
    printf("SCBR (%d) Goodbye!\n", s);
    if( s ) exit(1);
}

//------------------------------------------------------------------------------
const char *argp_program_version = "SCBR 0.9";
const char *argp_program_bug_address = "<rafael.pires@unine.ch>";

/* Program documentation. */
static char doc[] =
  "SCBR - Secure Content Based Routing";
static char args_doc[] = "";
static struct argp_option options[] = {
    { "address",  'a',  "host:port", 0, "Where to listen. Default: *:5555" },
    { "connect", 'c',  "host:port",  0, "Where to connect. Multiple possible." },
    { "verbose",  'v', "v", OPTION_ARG_OPTIONAL, "Displays more information"
                         "on screen. Verbosity increseases if arg is passed" },
    { "silent",   's',  0, 0, "No output" },
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
    case 'c':
        args->multconnect.push_back(std::string("tcp://") + arg);
        break;
    case 'v':
        args->verbose = arg ? strlen(arg) : 1;
        break;
    case 's':
        args->verbose = 0;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    };
    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };
//------------------------------------------------------------------------------
/* Application entry */
int SGX_CDECL main(int argc, char *argv[]) {
    Arguments args;
    argp_parse(&argp, argc, argv, 0, 0, &args);

    if( args.multconnect.empty() ) printf("SCBR zeromq address: %s\n", args.address.c_str());
#ifndef NONENCLAVE_MATCHING
    /* Changing dir to where the executable is.*/
    char absolutePath [MAX_PATH];
    char *ptr = NULL;

    ptr = realpath(dirname(argv[0]),absolutePath);

    if( chdir(absolutePath) != 0)
    		abort();

    /* Initialize the enclave */
    if(initialize_enclave( global_eid, "CBR_Enclave_Filter.signed.so", 
                           "enclave.scbr.token" ) < 0) {
        return -1; 
    }

    ecall_init( global_eid, args.verbose );
    printf("SCBR SGX: \033[32my\033[0m\n");
#else
    ecall_init( args.verbose );
    printf("SCBR SGX: \033[31mn\033[0m\n");
#endif

#ifdef PLAINTEXT_MATCHING
    printf("SCBR encryption: \033[31mn\033[0m\n");
#else
    printf("SCBR encryption: \033[32my\033[0m\n");
#endif

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrlc_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT,&sigIntHandler,NULL);

    legacymain( args );

#ifndef NONENCLAVE_MATCHING
    sgx_destroy_enclave(global_eid);
#endif
    return 0;
}

