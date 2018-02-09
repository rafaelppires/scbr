//#define NDEBUG

#include <iostream>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <scbr_api.h>
#include <zhelpers.h>
using namespace std::chrono_literals;

 
std::condition_variable cv;
std::mutex cv_m;
std::atomic<int> done{0};
const std::string key = "very secret key";

void callback_function( const std::string &data ) {
    std::cout << data << std::endl;
    done = 1;
    cv.notify_all();
}

int main() {
    Matcher scbr( "*:6666", "54321" ); // Local addr, id
    
    Subscription sub( SCBR_REGISTER );
    sub.predicate("id", SCBR_EQ, 1 );
    sub.predicate("stock_price", SCBR_LT, 100.00 );
    sub.set_callback( callback_function );
    scbr.send( sub );

    Publication pub;
    pub.attribute( "id", 1 );
    pub.attribute( "stock_price", 180.00 );
    pub.attribute( "date", "31.01.2018" );
    pub.payload( "Report about the stock" );
    pub.encrypt_payload( key );
    scbr.send( pub );

    pub.attribute( "stock_price", 90.00 );
    pub.attribute( "date", "01.02.2018" );
    pub.payload( "Another report about the stock" );
    pub.encrypt_payload( key );
    scbr.send( pub );

    int ret;
    {
        std::unique_lock<std::mutex> lk(cv_m);
        auto now = std::chrono::system_clock::now();
        if( cv.wait_until(lk, now + 500ms, [](){return done == 1;}) ) {
            std::cerr << "All good.\n";
            ret = 0;
        } else {
            std::cerr << "Timed out. No publication received\n";
            ret = -1;
        }
    }

    scbr.terminate();
    return ret;
}

