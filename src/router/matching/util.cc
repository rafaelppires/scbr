#include "util.hh"

namespace viper {

void SmartCounter::add(double sample) {
	double prev = mean_;
	if (samples_ == 0)
		min_ = max_ = sample;
	else if (sample < min_)
		min_ = sample;
	else if (sample > max_)
		max_ = sample;
	mean_ = mean_ + (sample - mean_) / (samples_ + 1);
	variance_ = variance_ + (sample - prev) * (sample - mean_);
	samples_++;
}

ostream& operator<<(ostream& stream, const SmartCounter& sc) {
#ifndef ENCLAVESGX
	stream << sc.mean() << " (#" << sc.samples() << ",~" << sc.variance() << ",<" << sc.min() << ",>" << sc.max() << ")";
#endif
	return stream;
}

}