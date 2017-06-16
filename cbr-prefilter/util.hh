#ifndef UTIL_HH
#define UTIL_HH

#include "common.hh"

#ifdef ENCLAVESGX
#define ostream int
#endif
namespace viper {

class SmartCounter {
public:
	SmartCounter() : samples_(0), mean_(0.0), variance_(0.0), min_(0.0), max_(0.0) { }

	void add(double sample);

	unsigned long samples() const {
		return samples_;
	}

	double mean() const {
		return mean_;
	}

	double variance() const {
		if(samples_ <= 1)
			return 0.0;
		return variance_ / (samples_ - 1);
	}

	double min() const {
		return min_;
	}

	double max() const {
		return max_;
	}

	friend ostream& operator<<(ostream& stream, const SmartCounter& sc);

private:
	unsigned long samples_;
	double mean_;
	double variance_;
	double min_;
	double max_;
};

} // namespace viper

#endif // UTIL_HH
