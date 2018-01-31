#ifndef SUBSCRIPTION_HH
#define SUBSCRIPTION_HH

#include "common.hh"
#include "event.hh"

namespace viper {

#define GRAPH_ATTR     0
#define PREFILTER_ATTR 1

class Subscription {
public:
	Subscription(string name = "<anonymous>") : name_(name) {
		attributes_[0] = attributes_[1] = NULL;
	}

	void add(key k, op o, value v) {
		sub_.insert(pair<key, predicate>(k, predicate(o, v)));
	}

	bool contains(key k) const {
		return sub_.find(k) != sub_.end();
	}

	unsigned int size() const {
		return sub_.size();
	}

	const string& name() const {
		return name_;
	}

	const subscription& data() const {
		return sub_;
	}

	bool matches(const Event& e) const;

	bool operator>=(const Subscription& s) const;

	bool operator==(const Subscription& s) const {
		return *this >= s && s >= *this;
	}

	bool operator>(const Subscription& s) const {
		return *this >= s && !(s >= *this);
	}

	void attributes(int type, void *attributes) {
		attributes_[type] = attributes;
	}

	void *attributes(int type) const {
		return attributes_[type];
	}

	friend ostream& operator<<(ostream& stream, const Subscription& s);

private:
	string name_;
	subscription sub_;
	void *attributes_[2];
};

} // namespace viper

#endif // SUBSCRIPTION_HH
