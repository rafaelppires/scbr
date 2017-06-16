#ifndef GRAPH_HH
#define GRAPH_HH

#include "common.hh"
#include "util.hh"
#include "subscription.hh"

namespace viper {

class Node;

class Graph {
public:
	struct Stats {
		Stats() : nodes(0), roots(0), leaves(0) { }

		int nodes;
		int roots;
		int leaves;
		int singletons;
		SmartCounter depth_min;
		SmartCounter depth_max;
		SmartCounter containees;
		SmartCounter containers;
		SmartCounter count;
		SmartCounter matches;
		SmartCounter tests;
	};

	Graph() : next_id_(0), size_(0), pubs_(0) { }

	~Graph();

	int size() const {
		return size_;
	}

	void add(Subscription& s);

	bool remove(Subscription& s);

	const Subscription *search(const Subscription& s);

	void containees(const Subscription& s, vector<const Subscription*>& c);

	void containers(const Subscription& s, vector<const Subscription*>& c);

	const Subscription *primary(const Subscription& s);

	int duplicates(const Subscription& s);

	void match(const Event& e, vector<const Subscription*> *matches);

	void stats(Graph::Stats& stats);

	const SmartCounter &test_stats() const {
		return test_stats_;
	}

	const SmartCounter &match_stats() const {
		return match_stats_;
	}

	void dump_stats(ostream& stream);

	friend ostream& operator<<(ostream& stream, const Graph& g);

private:
	static Node *search(const vector<Node*>& v, const Subscription& s);

	int next_id_;
	int size_;
	int pubs_;
	vector<Node*> nodes_;
	vector<Node*> roots_;
	vector<Node*> leaves_;
	SmartCounter test_stats_;
	SmartCounter match_stats_;
};

} // namespace viper

#endif // GRAPH_HH
