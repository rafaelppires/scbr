
#include <iostream>
#include <sstream>
#include <cbr.hh>
#include <pubsubco.h>
#include <utils.h>
#include <zhelpers.h>

#ifdef NONENCLAVE_MATCHING
#undef ENABLE_SGX
#else
#define ENAGLE_SGX
#endif
#include <sgx_cryptoall.h>

//------------------------------------------------------------------------------
unsigned fwdcount = 0;
void PubSubCo::open_matching( const Message &m ) {
    std::vector<std::string> v;
    split(v,m.header());

    viper::Subscription *sub = parse_sub(v);
    viper::Event *pub = parse_pub(v);

    if( sub ) {
        mgraph_.add( *sub );
    } else if( pub ) {
        vector<const viper::Subscription*> *matches =
                                       new vector<const viper::Subscription*>();
        mgraph_.match( *pub, matches );
        if( matches->size() ) {
            vector<const viper::Subscription*>::const_iterator it = matches->begin(),
                                                        end = matches->end();
            for(; it != end; ++it) {
				std::stringstream ss;
				ss << m;

				s_sendmore( comm_, std::string( (*it)->name().c_str() ) );
				s_sendmore( comm_, "" );
				s_send( comm_, ss.str() );

            ++fwdcount;
            if( fwdcount % 1000 == 0 ) std::cerr << blu(fwdcount/1000) << " ";
            }
        }
        delete matches;
        delete pub;
    }
}

#ifndef NONENCLAVE_MATCHING

//------------------------------------------------------------------------------
#include "CBR_Enclave_Filter_u.h"
#else
void ecall_add_evt( const char * buff, size_t len );
#endif
//------------------------------------------------------------------------------
void PubSubCo::enclave_matching( const Message &m ) {
    data_table_[ m.header() ] = m.payload();
    
    if( m.is_pub() ) {
        ++pcount_;
        if( pcount_ % 1000 == 0 ) std::cerr << cyn(pcount_/1000) << " ";
        ecall_pubstats.start();
    } else if( m.is_sub() ) { 
        ++scount_;
        if( scount_ % 1000 == 0 ) std::cerr << ylw(scount_/1000) << " ";
        ecall_substats.start();
    }
#ifdef NONENCLAVE_MATCHING
    int ret = 0;
    ecall_add_evt( m.header().c_str(), m.header().size() );
#else
    int ret =
        ecall_add_evt( global_eid, m.header().c_str(), m.header().size() );
#endif
    if( m.is_pub() ) {
        ecall_pubstats.finish();
    } else if( m.is_sub() ) {
        ecall_substats.finish();
    }

    if( ret ) std::cerr << "Erro " << ret << std::endl;
}

//------------------------------------------------------------------------------
void PubSubCo::forward( const std::string &t, const std::string &header,
                                              const std::string &hash ) {
    if( verbosity_ > 1 ) std::cout << "Fwd msg to: '" << t << "' pload: '" << Crypto::printable(data_table_[header]) << "' which matches '" << Crypto::printable(hash) << "'\n";
    char *buf = strdup( t.c_str() );
    char *p = strtok(buf, " ");
    size_t i = 0;
    while (p) {
        //if( verbosity_ > 0 ) printf ("Sending to: '%s'\n", p);

        s_sendmore( comm_, p );
        s_sendmore( comm_, "" );
        s_send( comm_, hash.substr(i++*32,32)+data_table_[header] );

        p = strtok(NULL, " ");
    }
    free(buf);
}

//------------------------------------------------------------------------------
void PubSubCo::pubsub_message( const std::string &pub ) {
    Message m(pub);
    if( m.bad() ) {
        std::cerr << red(m.error_string()) << std::endl;
        return;
    }

    if( m.is_enc() )
        enclave_matching( m );
    else { //printf("*** Should not happen in MR\n");
        open_matching( m );
    }
}

//------------------------------------------------------------------------------
void PubSubCo::print_stats() {
    if( verbosity_ ){
        ecall_pubstats.close();
        ecall_substats.close();
        printf("\n\n==> %u PUBS\n", pcount_);
        printf("\n==> %u SUBS\n", scount_);
    }
}
//------------------------------------------------------------------------------

