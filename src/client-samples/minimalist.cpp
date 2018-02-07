#include <iostream>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>
using namespace std::chrono_literals;
 
std::condition_variable cv;
std::mutex cv_m;
std::atomic<int> done{0};

#include <scbr_api.h>

const std::string key = "very secret key";
void callback_function() {
    done = 1;
    cv.notify_all();
}

int main() {
    Matcher scbr;

    Subscription sub( SCBR_REGISTER );
    sub.predicate("name", SCBR_EQ, "SecureCloud Inc.");
    sub.predicate("stock_price", SCBR_LT, 100.00 );
    sub.callback( callback_function );
    scbr.send( sub );

    Publication pub;
    pub.attribute( "name", "SecureCloud Inc." );
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

    {
        std::unique_lock<std::mutex> lk(cv_m);
        auto now = std::chrono::system_clock::now();
        if( cv.wait_until(lk, now + 500ms, [](){return done == 1;}) ) {
            std::cerr << "All good.\n";
            return 0;
        } else {
            std::cerr << "Timed out. No publication received\n";
            return -1;
        }
    }
}

