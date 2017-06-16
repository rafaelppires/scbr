#include "prefilter.hh"
#include "event.hh"
#include "subscription.hh"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <boost/dynamic_bitset.hpp>

namespace viper {

static unsigned int MurmurHash2(const void *key, int len, unsigned int seed)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	unsigned int h = seed ^ len;
	const unsigned char *data = (const unsigned char *)key;

	while(len >= 4) {
		unsigned int k = *(unsigned int *)data;

		k *= m; 
		k ^= k >> r; 
		k *= m; 

		h *= m; 
		h ^= k;

		data += 4;
		len -= 4;
	}

	switch(len)	{
		case 3: h ^= data[2] << 16;
		case 2: h ^= data[1] << 8;
		case 1: h ^= data[0];
	        	h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 

static void hash_n(value val, int max, vector<int>& hashes, int n) {
	unsigned int h = 0;
	while (n-- > 0) {
		h = MurmurHash2(&val, sizeof(value), h);
		hashes.push_back(h % max);
	}
}

class BloomFilter {
public:
	BloomFilter(int size, int nb_hashes) : nb_hashes_(nb_hashes), bits_(size) {	}

	void insert(value val) {
		vector<int> hashes;
		hash_n(val, bits_.size(), hashes, nb_hashes_);
		for (vector<int>::const_iterator i = hashes.begin(); i != hashes.end(); ++i)
			bits_.set(*i);
	}

	void insert_1(value val) {
		vector<int> hashes;
		hash_n(val, bits_.size(), hashes, nb_hashes_);
		random_shuffle(hashes.begin(), hashes.end());
		bits_.set(*hashes.begin());
	}

	size_t count() const {
		return bits_.count();
	}

	bool test(int i) const {
		return bits_.test(i);
	}

	bool superset(const BloomFilter& bf) {
		return ((bits_ & bf.bits_) == bf.bits_);
	}

	friend ostream& operator<<(ostream& stream, const BloomFilter& bf);

private:
	boost::dynamic_bitset<> bits_;
	int nb_hashes_;
};

ostream& operator<<(ostream& stream, const BloomFilter& bf) {
	stream << bf.bits_ << " #" << bf.bits_.count();
	return stream;
}

struct BloomSubscription {
	BloomSubscription(const Subscription& s, int size, int nb_hashes) : sub_(s), bf_(size, nb_hashes), matches_(0), tests_(0), duplicates_(NULL) { }
	~BloomSubscription() {
		delete duplicates_;
	}

	const Subscription& sub_;
	unsigned int id_;
	BloomFilter bf_;
	int matches_;
	int tests_;
	vector<BloomSubscription*> *duplicates_;
};

PreFilter::PreFilter(int nb_bits, int nb_hashes, int variant, Graph *graph) : nb_bits_(nb_bits), nb_hashes_(nb_hashes), variant_(variant), graph_(graph), pubs_(0) {
	subs_bf_ = new vector<BloomSubscription*>[nb_bits + 1];
	if (nb_hashes < 1 || nb_hashes > nb_bits) {
		cerr << "Unvalid number of hash functions: " << nb_hashes << endl;
		exit(1);
	}
	if ((variant == ALL_CONTAINMENT_N || variant == ALL_CONTAINMENT_NP) && graph == NULL) {
		cerr << "Containment graph required with variant: " << variant << endl;
		exit(1);
	}
}

PreFilter::~PreFilter() {
	for (vector<BloomSubscription*>::iterator i = subs_.begin(); i != subs_.end(); ++i)
		delete *i;
	delete[] subs_bf_;
}

void PreFilter::add(Subscription& s) {
	BloomSubscription *bs = new BloomSubscription(s, nb_bits_, nb_hashes_);
	s.attributes(PREFILTER_ATTR, bs);
	bs->id_ = subs_.size();
	subs_.push_back(bs);
	// Check for duplicate subscription
	if (graph_ != NULL) {
		const Subscription *ps = graph_->primary(s);
		if (ps != &s) {
			BloomSubscription *pbs = (BloomSubscription *) ps->attributes(PREFILTER_ATTR);
			if (pbs->duplicates_ == NULL)
				pbs->duplicates_ = new vector<BloomSubscription*>;
			pbs->duplicates_->push_back(bs);
		}
	}
	// Create Bloom filter for subscription
	for (subscription::const_iterator i = s.data().begin(); i != s.data().end(); ++i) {
		const predicate& p = i->second;
		if (p.first == EQ) {
			if (variant_ == ONE_NO_CONTAINMENT)
				bs->bf_.insert_1(p.second);
			else
				bs->bf_.insert(p.second);
		}
	}
	if (bs->bf_.count() != 0) {
		for (int i = 0; i < nb_bits_; ++i) {
			if (bs->bf_.test(i)) {
				if (variant_ == ALL_CONTAINMENT_N || variant_ == ALL_CONTAINMENT_NP) {
					// Insert containers first
					vector<BloomSubscription*>::iterator j = subs_bf_[i].begin();
					while (j != subs_bf_[i].end() && !(s >= (*j)->sub_))
						++j;
					subs_bf_[i].insert(j, bs);
				} else {
					// Insert randomly
					subs_bf_[i].push_back(bs);
				}
			}
		}
	} else {
		if (variant_ == ALL_CONTAINMENT_N || variant_ == ALL_CONTAINMENT_NP) {
			// Insert containers first
			vector<BloomSubscription*>::iterator j = subs_bf_[nb_bits_].begin();
			while (j != subs_bf_[nb_bits_].end() && !(s >= (*j)->sub_))
				++j;
			subs_bf_[nb_bits_].insert(j, bs);
		} else {
			// Insert randomly
			subs_bf_[nb_bits_].push_back(bs);
		}
	}
}

bool PreFilter::remove(Subscription& s) {
	// Add only nodes that are not earlier in list
	BloomSubscription *bs = (BloomSubscription *) s.attributes(PREFILTER_ATTR);
	if (bs == NULL)
		return false;
	int count = bs->bf_.count();
	if (count == 0) {
		subs_bf_[nb_bits_].erase(std::remove(subs_bf_[nb_bits_].begin(), subs_bf_[nb_bits_].end(), bs), subs_bf_[nb_bits_].end());
	} else {
		for (int bit = 0; bit < nb_bits_ && count > 0; ++bit) {
			if (bs->bf_.test(bit)) {
				subs_bf_[bit].erase(std::remove(subs_bf_[bit].begin(), subs_bf_[bit].end(), bs), subs_bf_[bit].end());
				count--;
			}
		}
	}
	subs_.erase(std::remove(subs_.begin(), subs_.end(), bs), subs_.end());
	delete bs;
	return true;
}

void PreFilter::match(const Event& e, vector<const Subscription*> *matches) {
	pubs_++;
	// Create Bloom filter for event
	BloomFilter bf(nb_bits_, nb_hashes_);
	for (event::const_iterator i = e.data().begin(); i != e.data().end(); ++i)
		bf.insert(i->second);
	size_t c = bf.count();
	if (VERBOSE >= 3)
		cout << "  " << bf << endl;
	// Only keep subscriptions that have the same bits set
	bool *matched = new bool[subs_.size()];
	memset(matched, 0, subs_.size() * sizeof(bool));
	int match = 0;
	int test = 0;
	vector<const Subscription*> cands;
	// Traverse all lists (except default)
	for (int bit = 0; bit < nb_bits_ && c > 0; ++bit) {
		if (bf.test(bit)) {
			for (vector<BloomSubscription*>::const_iterator i = subs_bf_[bit].begin(); i != subs_bf_[bit].end(); ++i) {
				if (graph_ != NULL) {
					// Do not test duplicate subscriptions
					if (graph_->primary((*i)->sub_) != &(*i)->sub_)
						matched[(*i)->id_] = true;
				}
				if (!matched[(*i)->id_]) {
					matched[(*i)->id_] = true;
					bool m = false;
					if (bf.superset((*i)->bf_)) {
						test++;
						(*i)->tests_++;
						if ((*i)->sub_.matches(e)) {
							int count = (graph_ == NULL ? 1 : graph_->duplicates((*i)->sub_));
							if (matches != NULL) {
								for (int j = 0; j < count; j++)
									matches->push_back(&(*i)->sub_);
							}
							match += count;
							(*i)->matches_++;
							if ((*i)->duplicates_ != NULL) {
								for (vector<BloomSubscription*>::const_iterator j = (*i)->duplicates_->begin(); j != (*i)->duplicates_->end(); ++j) {
									(*j)->matches_++;
									matched[(*j)->id_] = true;
								}
							}
							m = true;
						}
					}
					if (m && variant_ == ALL_CONTAINMENT_NP) {
						// Propagate positive match to containers
						vector<const Subscription*> c;
						graph_->containers((*i)->sub_, c);
						while (!c.empty()) {
							const Subscription *s = c.back();
							c.pop_back();
							BloomSubscription *bs = (BloomSubscription *) s->attributes(PREFILTER_ATTR);
							if (!matched[bs->id_]) {
								int count = graph_->duplicates(bs->sub_);
								if (matches != NULL) {
									for (int j = 0; j < count; j++)
										matches->push_back(s);
								}
								match += count;
								bs->matches_++;
								if (bs->duplicates_ != NULL) {
									for (vector<BloomSubscription*>::const_iterator j = bs->duplicates_->begin(); j != bs->duplicates_->end(); ++j) {
										(*j)->matches_++;
										matched[(*j)->id_] = true;
									}
								}
								matched[bs->id_] = true;
								graph_->containers(*s, c);
							}
						}
					}
					if (!m && (variant_ == ALL_CONTAINMENT_N || variant_ == ALL_CONTAINMENT_NP)) {
						// Propagate negative match to containees
						vector<const Subscription*> c;
						graph_->containees((*i)->sub_, c);
						while (!c.empty()) {
							const Subscription *s = c.back();
							c.pop_back();
							BloomSubscription *bs = (BloomSubscription *) s->attributes(PREFILTER_ATTR);
							if (!matched[bs->id_]) {
								matched[bs->id_] = true;
								graph_->containees(*s, c);
							}
						}
					}
				}
			}
		}
	}
	// Traverse default list
	for (vector<BloomSubscription*>::const_iterator i = subs_bf_[nb_bits_].begin(); i != subs_bf_[nb_bits_].end(); ++i) {
		if (!matched[(*i)->id_]) {
			matched[(*i)->id_] = true;
			test++;
			(*i)->tests_++;
			if ((*i)->sub_.matches(e)) {
				if (matches != NULL)
					matches->push_back(&(*i)->sub_);
				match++;
				(*i)->matches_++;
				if (variant_ == ALL_CONTAINMENT_NP) {
					// Propagate positive match to containers
					vector<const Subscription*> c;
					graph_->containers((*i)->sub_, c);
					while (!c.empty()) {
						const Subscription *s = c.back();
						c.pop_back();
						BloomSubscription *bs = (BloomSubscription *) s->attributes(PREFILTER_ATTR);
						if (!matched[bs->id_]) {
							int count = graph_->duplicates(bs->sub_);
							if (matches != NULL) {
								for (int j = 0; j < count; j++)
									matches->push_back(s);
							}
							match += count;
							bs->matches_++;
							if (bs->duplicates_ != NULL) {
								for (vector<BloomSubscription*>::const_iterator j = bs->duplicates_->begin(); j != bs->duplicates_->end(); ++j) {
									(*j)->matches_++;
									matched[(*j)->id_] = true;
								}
							}
							matched[bs->id_] = true;
							graph_->containers(*s, c);
						}
					}
				}
			} else if (variant_ == ALL_CONTAINMENT_N || variant_ == ALL_CONTAINMENT_NP) {
				// Propagate negative match to containees
				vector<const Subscription*> c;
				graph_->containees((*i)->sub_, c);
				while (!c.empty()) {
					const Subscription *s = c.back();
					c.pop_back();
					BloomSubscription *bs = (BloomSubscription *) s->attributes(PREFILTER_ATTR);
					if (!matched[bs->id_]) {
						matched[bs->id_] = true;
						graph_->containees(*s, c);
					}
				}
			}
		}
	}
	delete[] matched;
	test_stats_.add((double) test / (double) subs_.size());
	match_stats_.add((double) match / (double) subs_.size());
}

void PreFilter::stats(PreFilter::Stats& stats) {
	stats.empty_lists = 0;
	for (int i = 0; i < nb_bits_; i++) {
		if (subs_bf_[i].empty())
			stats.empty_lists++;
		stats.list_size.add(subs_bf_[i].size());
	}
	stats.nobf_size = subs_bf_[nb_bits_].size();
	for (vector<BloomSubscription*>::const_iterator i = subs_.begin(); i != subs_.end(); ++i) {
		stats.matches.add(pubs_ == 0 ? 0.0 : (double) (*i)->matches_ / (double) pubs_);
		stats.tests.add(pubs_ == 0 ? 0.0 : (double) (*i)->tests_ / (double) pubs_);
	}
}

void PreFilter::dump_stats(ostream& stream) {
	int n = 0;
	for (vector<BloomSubscription*>::const_iterator i = subs_.begin(); i != subs_.end(); ++i)
		stream << "  [" << (n++) << "] matches=" << (*i)->matches_ << " tests=" << (*i)->tests_ << endl;
}

ostream& operator<<(ostream& stream, const PreFilter& p) {
	stream << "Pre-filter: #" << p.subs_.size() << " (" << p.nb_bits_ << "," << p.nb_hashes_ << ")" << endl;
	stream << "[";
	for (int i = 0; i < p.nb_bits_; i++)
		stream << p.subs_bf_[i].size() << (i == p.nb_bits_ - 1 ? "" : ",");
	stream << "]," << p.subs_bf_[p.nb_bits_].size() << endl;
	for (vector<BloomSubscription*>::const_iterator i = p.subs_.begin(); i != p.subs_.end(); ++i)
		stream << "  " << (*i)->sub_.name() << " (" << (*i)->sub_ << ")" << endl << "  " << (*i)->bf_ << endl;

	return stream;
}

}
