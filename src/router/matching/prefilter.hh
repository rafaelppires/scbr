#ifndef PREFILTER_HH
#define PREFILTER_HH

#include "common.hh"
#include "util.hh"
#include "subscription.hh"
#include "graph.hh"

#include <set>
#include <vector>

#define ALL_NO_CONTAINMENT  0
#define ONE_NO_CONTAINMENT  1
#define ALL_CONTAINMENT_N   2
#define ALL_CONTAINMENT_NP  3

namespace viper {

class BloomSubscription;

class PreFilter {
public:
	struct Stats {
		Stats() : empty_lists(0), nobf_size(0) { }

		int empty_lists;
		int nobf_size;
		SmartCounter list_size;
		SmartCounter matches;
		SmartCounter tests;
	};

	PreFilter(int nb_bits, int nb_hashes, int variant, Graph *graph);
	~PreFilter();

	int size() const {
		return subs_.size();
	}

	void add(Subscription& s);

	bool remove(Subscription& s);

	void match(const Event& e, vector<const Subscription*> *matches);

	void stats(PreFilter::Stats& stats);

	const SmartCounter &test_stats() const {
		return test_stats_;
	}

	const SmartCounter &match_stats() const {
		return match_stats_;
	}

	void dump_stats(ostream& stream);

	friend ostream& operator<<(ostream& stream, const PreFilter& g);

private:
	int nb_bits_;
	int nb_hashes_;
	int variant_;
	int pubs_;
	bool optimized_;
	Graph *graph_;
	vector<BloomSubscription*> subs_;
	vector<BloomSubscription*> *subs_bf_;
	SmartCounter test_stats_;
	SmartCounter match_stats_;
};

} // namespace viper

#endif // PREFILTER_HH
