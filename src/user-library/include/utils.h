#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdlib.h>
#include <sstream>
#include <cstring>

template< typename T >
std::string colorize( T x, int c ) {
    std::stringstream ss;
    ss << "\033[0;" << c << "m" << x << "\033[0m";
    return ss.str();
}

template< typename T > std::string blk( T x ) { return colorize(x,30); }
template< typename T > std::string red( T x ) { return colorize(x,31); }
template< typename T > std::string grn( T x ) { return colorize(x,32); }
template< typename T > std::string ylw( T x ) { return colorize(x,33); }
template< typename T > std::string blu( T x ) { return colorize(x,34); }
template< typename T > std::string mgt( T x ) { return colorize(x,35); }
template< typename T > std::string cyn( T x ) { return colorize(x,36); }
template< typename T > std::string wht( T x ) { return colorize(x,37); }

//------------------------------------------------------------------------------
#include <boost/chrono.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
using boost::chrono::high_resolution_clock;
using boost::chrono::nanoseconds;
using namespace boost::accumulators;
class Stats {
public:
    enum Resolution {
        SECONDS,
        MICRO,
        MILI,
        NANO
    };

    Stats( Resolution r = MILI ) {
        switch( r ) {
        case SECONDS:   divisor_ = 1e+9; break;
        case MILI:      divisor_ = 1e+6; break;
        case MICRO:     divisor_ = 1e+3; break;
        case NANO:      divisor_ = 1;    break;
        };
    }
    void begin();
    double end();
    void print_summary(const std::string &title = "");
    unsigned count();

private:
    high_resolution_clock::time_point start_;
    accumulator_set<double,stats<tag::variance,tag::sum,tag::count,tag::mean> > accum_;
    double divisor_;
};

//------------------------------------------------------------------------------
#include <fstream>
class Logger {
public:
    void init( std::string outfname );
    void close();
    void start();
    void finish();
private:
    Stats timing_;
    std::ofstream log_;
    int cachemiss_fd, cacheref_fd;
};

//------------------------------------------------------------------------------
#endif

