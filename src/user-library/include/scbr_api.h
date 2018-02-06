
/*

Publication pub;
pub.attribute( "name", "SecureCloud Inc." );
pub.attribute( "stock_price", 180.00 );
pub.attribute( "date", "31.01.2018" );
pub.payload( "Report about the stock" );
scbr.send( pub );

Subscription sub( SCBR_REGISTER );
sub.predicate("name", SCBR_EQ, "SecureCloud Inc.");
sub.predicate("stock_price", SCBR_LT, 100.00 );
sub.callback( callbak_function );
scbr.send( sub );

*/

enum ComparisonOperator {
    SCBR_EQ,
    SCBR_LT,
    SCBR_GT,
    SCBR_LE,
    SCBR_GE
};

class Publication {
public:
    void attribute( const std::string &key, const std::string &value );
    void payload( const std::string &payload );
};

class Subscription {
public:
    void predicate( const std::string &key, ComparisonOperator c,
                    const std::string &value );
    void callback();
};

class Matcher {
public:
    void send( const Subscription &sub );
};

