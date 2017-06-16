#include "event.hh"

namespace viper {

ostream& operator<<(ostream& stream, const Event& e) {
	int nb = 0;
	stream << "{";
	for (event::const_iterator i = e.event_.begin(); i != e.event_.end(); ++i) {
		if (nb++ != 0)
			stream << ",";
		stream << i->first << "=" << i->second;
	}
	stream << "}";
	return stream;
}

}
