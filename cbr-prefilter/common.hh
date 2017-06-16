#ifndef COMMON_HH
#define COMMON_HH

#include <string>
#include <utility>
#include <map>
#include <vector>

#ifndef ENCLAVESGX
#include <iostream>
#endif

using namespace std;

extern int VERBOSE;

namespace viper {

typedef string key;
typedef int value;
typedef enum { EXISTS, EQ, NE, LT, GT, LE, GE, CONTAINS, STARTS_WITH } op;

typedef pair<op, value> predicate;

typedef map<key, value> event;
typedef multimap<key, predicate> subscription;

} // namespace viper

#endif // COMMON_HH
