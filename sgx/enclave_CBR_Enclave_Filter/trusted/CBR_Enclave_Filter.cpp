#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <sgx_utiles.h>

#ifndef NONENCLAVE_MATCHING
#include "CBR_Enclave_Filter.h"
#include "CBR_Enclave_Filter_t.h"  /* print_string */

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
//------------------------------------------------------------------------------
void printf(const char *fmt, ...) {
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print(buf);
}
#endif
//------------------------------------------------------------------------------
#include "cbr.hh"
#include "event.hh"
#include "subscription.hh"
#include "prefilter.hh"

viper::Graph *g = NULL;
viper::PreFilter *p = NULL;

//#include <utils.h>
//Stats beforestats, afterstats;
unsigned match_count = 0;
//------------------------------------------------------------------------------
int ecall_CBR_Enclave_Filter_sample() {
return 0; /* pasin 
    viper::Graph::Stats s;
    if(g) g->stats( s );
#ifdef PLAINTEXT_MATCHING
#ifdef NONENCLAVE_MATCHING
    printf("\033[0;31mSCBR Non enclave plaintext matching\033[0m\n"
#else
    printf("\033[0;32mSCBR Enclave plaintext matching\033[0m\n"
#endif
#else
#ifdef NONENCLAVE_MATCHING
    printf("\033[0;31mSCBR Non enclave encrypted matching\033[0m\n"
#else
    printf("\033[0;32mSCBR Enclave encrypted matching\033[0m\n"
#endif
#endif
           "Matches: %u\n"
           "Nodes: %d\n"
           "Roots: %d\n"
           "Leaves: %d\n"
           "Singletons: %d\n"
           "            #\tMean\t\tVariance\tMin\t\tMax\n"
           "Depth min:  %lu\t%f\t%f\t%f\t%f\n"
           "Depth max:  %lu\t%f\t%f\t%f\t%f\n"
           "Containees: %lu\t%f\t%f\t%f\t%f\n"
           "Containers: %lu\t%f\t%f\t%f\t%f\n"
           "Count:      %lu\t%f\t%f\t%f\t%f\n"
           "Matches:    %lu\t%f\t%f\t%f\t%f\n"
           "Tests:      %lu\t%f\t%f\t%f\t%f\n",
           match_count,
           s.nodes, s.roots, s.leaves, s.singletons,
           s.depth_min.samples(), s.depth_min.mean(), s.depth_min.variance(), s.depth_min.min(), s.depth_min.max(),
           s.depth_max.samples(), s.depth_max.mean(), s.depth_max.variance(), s.depth_max.min(), s.depth_max.max(),
           s.containees.samples(), s.containees.mean(), s.containees.variance(), s.containees.min(), s.containees.max(),
           s.containers.samples(), s.containers.mean(), s.containers.variance(), s.containers.min(), s.containers.max(),
           s.count.samples(), s.count.mean(), s.count.variance(), s.count.min(), s.count.max(),
           s.matches.samples(), s.matches.mean(), s.matches.variance(), s.matches.min(), s.matches.max(),
           s.tests.samples(), s.tests.mean(), s.tests.variance(), s.tests.min(), s.tests.max()
    );
    //beforestats.print_summary();
    //afterstats.print_summary();
    return 0; */
}

//------------------------------------------------------------------------------
int verbosity;
void ecall_init( int verb ) {
    verbosity = verb;
}
//------------------------------------------------------------------------------
typedef vector<const viper::Subscription*> MatchList;
void ecall_add_evt( const char * buff, size_t len ) {
    if( len == 0 ) return;
    std::string rawhdr(buff,len); 
#ifndef PLAINTEXT_MATCHING
    char recovered[512];
    size_t sz = std::min(len,sizeof(recovered));
    decrypt( buff, recovered, sz );
    if( is_cipher(recovered, sz) ) {
        recovered[ std::min(sz,sizeof(recovered)-1) ] = 0;
        printf("\033[91mSCBR Error: decryption did not give plaintext sz(%d) len(%d) '%s' -> '%s'\033[0m\n", sz, len,rawhdr.c_str(),recovered);
        return;
    }
    std::string s( (const char*)recovered, sz );
#else
    std::string s=rawhdr; 
#endif
//beforestats.begin();
    if( !g ) g = new viper::Graph();
    vector<string> v;
    split(v, s);

    viper::Subscription *sub = parse_sub(v);
    viper::Event *pub = parse_pub(v);
//beforestats.end();
    if( sub ) {
//afterstats.begin();
        if(verbosity > 1) printf("[SUB] %s\n", s.c_str());
        g->add( *sub );
//afterstats.end();
    } else if( pub ) {
        if(verbosity > 1) printf("(PUB) %s\n", s.c_str());
        MatchList *matches = new MatchList();
        g->match( *pub, matches );
        if( matches->size() ) ++match_count;
        if( matches->size() ) {
            std::string matchlist;
            MatchList::iterator it = matches->begin(), end = matches->end();
            for(; it!=end; ++it)
                matchlist += (*it)->name() + " ";
            ocall_added_notify( matchlist.c_str(), rawhdr.c_str(), rawhdr.size() );
            if(verbosity > 0) printf("Matched %lu: %s\n", matches->size(), matchlist.c_str());
        }
        delete matches;
        delete pub;
    }
}


