#ifndef CBR_HH
#define CBR_HH

#include "common.hh"

namespace viper {

} // namespace viper


#include "event.hh"
#include "subscription.hh"
void split( std::vector<std::string> &v, const std::string &s );
viper::Event *parse_pub( const vector<string> &v );
viper::Subscription *parse_sub( const vector<string> &v );
bool parse_pubs(const char *file, vector<viper::Event*> &pubs);
bool parse_subs(const char *file, vector<viper::Subscription*> &subs);

#endif // CBR_HH
