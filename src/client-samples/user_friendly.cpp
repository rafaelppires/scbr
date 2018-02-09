#include <iostream>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <scbr_api.h>
#include <zhelpers.h>
#include <sgx_cryptoall.h>
using namespace std::chrono_literals;
 
std::condition_variable cv;
std::mutex cv_m;
std::atomic<int> done{0};
const std::string key = "very secret key";
const char *date_str  = "date",
           *stock_str = "stock_price";

void good_callback_function( const std::string &data ) {
    Publication pub;
    pub.deserialize( key, data );
    PubMap attrib = pub.attributes();
    std::cout << "\\o/ On " << attrib[date_str] << " the stock price was " 
              << attrib[stock_str] << ". Publisher says: " 
              << pub.payload() << std::endl;
    ++done;
    cv.notify_one();
}

void bad_callback_function( const std::string &data ) {
    Publication pub;
    pub.deserialize( key, data );
    PubMap attrib = pub.attributes();
    std::cout << ":( On " << attrib[date_str] << " the stock price was " 
              << attrib[stock_str] << ". Publisher says: " 
              << pub.payload() << std::endl;
    ++done;
    cv.notify_one();
}

int main() {
    Matcher scbr( "*:6666", "54322" ); // Local addr, id
    
    Subscription sub( SCBR_REGISTER );
    sub.predicate( "id", SCBR_EQ, 1 );
    sub.predicate( stock_str, SCBR_LT, 100.00 );
    sub.set_callback( bad_callback_function );
    scbr.send( sub );

    sub.predicate( stock_str, SCBR_GT, 200.00 );
    sub.set_callback( good_callback_function );
    scbr.send( sub );

    Publication pub;
    pub.attribute( "id", 1 );
    pub.attribute( stock_str, 180.00 );
    pub.attribute( date_str, "31.01.2018" );
    pub.payload( "Yeah... not bad" );
    pub.encrypt_payload( key );
    scbr.send( pub );

    pub.attribute( stock_str, 90.00 );
    pub.attribute( date_str, "01.02.2018" );
    pub.payload( "We're doomed" );
    pub.encrypt_payload( key );
    scbr.send( pub );

    pub.attribute( stock_str, 250.00 );
    pub.attribute( date_str, "02.02.2018" );
    pub.payload( "Let's buy a brand new yacht" );
    pub.encrypt_payload( key );
    scbr.send( pub );

    int ret;
    {
        std::unique_lock<std::mutex> lk(cv_m);
        auto now = std::chrono::system_clock::now();
        if( cv.wait_until(lk, now + 500ms, [](){return done == 2;}) ) {
            std::cout << "All good.\n";
            ret = 0;
        } else {
            std::cerr << "Timed out. No publication received\n";
            ret = -1;
        }
    }

    scbr.terminate();
    return ret;
}

