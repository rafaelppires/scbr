#ifndef EVENT_HH
#define EVENT_HH

#include "common.hh"

#ifdef ENCLAVESGX
#define ostream int
#endif
namespace viper {

class Event {
public:
	Event(string name = "<anonymous>") : name_(name) { }

	void add(key k, value v) {
		event_[k] = v;
	}

	bool contains(key k) {
		return event_.find(k) != event_.end();
	}

	const event& data() const {
		return event_;
	}

	unsigned int size() const {
		return event_.size();
	}

	const string& name() const {
		return name_;
	}

	friend ostream& operator<<(ostream& stream, const Event& e);

private:
	string name_;
	event event_;
};

} // namespace viper

#endif // EVENT_HH
