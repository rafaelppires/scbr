#include <algorithm>
#include <stack>

#include "graph.hh"

#if 0
#ifdef ENCLAVESGX
extern "C" {
extern void printf(const char *fmt, ... );
}
#endif
#endif

namespace viper {

struct Node {
	Node(const Subscription& s) : sub_(s), count_(1), last_(0), depth_min_(0), depth_max_(0), matches_(0), tests_(0) { }

	bool ancestor(const Node& n) const;

	bool descendant(const Node& n) const;

	void find_containers(const Subscription& s, vector<Node*>& c, int id);

	void find_containees(const Subscription& s, vector<Node*>& c, int id);

	bool matched(int id) {
		return last_ >= id ? true : (last_ = id, false);
	}

	void clear(Node *parent);

	void reset_stats(int id);

	void prepare_stats(const Node *parent);

	void compute_stats(Graph::Stats& stats, int pubs, int id);

	bool operator<(Node &n) { return sub_.name() < n.sub_.name(); }

	const Subscription& sub_;
    vector<const Subscription*> overriden_; // <- Added by Pires
                                            // TODO: Check consequences on 
                                            //       unsubscribe

	vector<Node*> containers_;
	vector<Node*> containees_;
	int count_;
	int last_;
	int depth_min_;
	int depth_max_;
	int matches_;
	int tests_;
};

struct CompareNodes {
	bool operator()(Node *n1, Node *n2) {
		return *n1 < *n2;
	}
};

void Node::clear(Node *parent) {
	// Delete our children
	while (!containees_.empty()) {
		Node *n = *(containees_.begin());
		n->clear(this);
		delete n;
	}
	// Remove self from parents
	for (vector<Node*>::const_iterator i = containers_.begin(); i != containers_.end(); ++i)
		(*i)->containees_.erase(std::remove((*i)->containees_.begin(), (*i)->containees_.end(), this), (*i)->containees_.end());
}

bool Node::ancestor(const Node& n) const {
	for (vector<Node*>::const_iterator i = containees_.begin(); i != containees_.end(); ++i) {
		if (*i == &n)
			return true;
		if ((*i)->ancestor(n))
			return true;
	}
	return false;
}

bool Node::descendant(const Node& n) const {
	for (vector<Node*>::const_iterator i = containers_.begin(); i != containers_.end(); ++i) {
		if (*i == &n)
			return true;
		if ((*i)->descendant(n))
			return true;
	}
	return false;
}

void Node::find_containers(const Subscription& s, vector<Node*>& c, int id) {
	if (last_ == id)
		return;
	last_ = id;
	int found = 0;
	for (vector<Node*>::iterator i = containees_.begin(); i != containees_.end(); ++i) {
		if ((*i)->sub_ >= s) {
			(*i)->find_containers(s, c, id);
			found++;
		}
	}
	if (found == 0)
		c.push_back(this);
}

void Node::find_containees(const Subscription& s, vector<Node*>& c, int id) {
	if (last_ == id)
		return;
	last_ = id;
	int found = 0;
	for (vector<Node*>::iterator i = containers_.begin(); i != containers_.end(); ++i) {
		if (s >= (*i)->sub_) {
			(*i)->find_containees(s, c, id);
			found++;
		}
	}
	if (found == 0)
		c.push_back(this);
}

void Node::reset_stats(int id) {
	if (last_ == id)
		return;
	last_ = id;
	depth_min_ = depth_max_ = 0;
	for (vector<Node*>::const_iterator i = containees_.begin(); i != containees_.end(); ++i)
		(*i)->reset_stats(id);
}

void Node::prepare_stats(const Node *parent) {
	bool updated = false;
	if (parent == NULL) {
		depth_min_ = depth_max_ = 1;
		updated = true;
	} else {
		if (depth_max_ == 0 || depth_max_ < parent->depth_max_ + 1) {
			depth_max_ = parent->depth_max_ + 1;
			updated = true;
		}
		if (depth_min_ == 0 || depth_min_ > parent->depth_min_ + 1) {
			depth_min_ = parent->depth_min_ + 1;
			updated = true;
		}
	}
	if (updated) {
		for (vector<Node*>::const_iterator i = containees_.begin(); i != containees_.end(); ++i)
			(*i)->prepare_stats(this);
	}
}

void Node::compute_stats(Graph::Stats& stats, int pubs, int id) {
	if (last_ == id)
		return;
	last_ = id;
	stats.depth_min.add(depth_min_);
	stats.depth_max.add(depth_max_);
	stats.containees.add(containees_.size());
	stats.containers.add(containers_.size());
	stats.count.add(count_);
	for (int i = 0; i < count_; i++)
		stats.matches.add(pubs == 0 ? 0.0 : (double) matches_ / (double) pubs);
	stats.tests.add(pubs == 0 ? 0.0 : (double) tests_ / (double) pubs);
	for (int i = 1; i < count_; i++)
		stats.tests.add(0);
	for (vector<Node*>::const_iterator i = containees_.begin(); i != containees_.end(); ++i)
		(*i)->compute_stats(stats, pubs, id);
}

Node *Graph::search(const vector<Node*>& v, const Subscription& s) {
	for (vector<Node*>::const_iterator i = v.begin(); i != v.end(); ++i) {
		if ((*i)->sub_ >= s) {
			if (s >= (*i)->sub_)
				return *i;
			return search((*i)->containees_, s);
		}
	}
	return NULL;
}

Graph::~Graph() {
	for (vector<Node*>::iterator i = roots_.begin(); i != roots_.end(); ++i) {
		(*i)->clear(NULL);
		delete *i;
	}
}

void Graph::add(Subscription& s) {
	size_++;
	// Already an equivalent subscription?
	Node *n = search(roots_, s);
	if (n != NULL) {
		s.attributes(GRAPH_ATTR, n);
        n->overriden_.push_back(&s);
		n->count_++;
		return;
	}
	// Add subscription
	n = new Node(s);
	s.attributes(GRAPH_ATTR, n);
	nodes_.push_back(n);
	// Start from roots
	++next_id_;
	for (vector<Node*>::iterator i = roots_.begin(); i != roots_.end(); ++i) {
		if ((*i)->sub_ >= s) {
			// Create containers list
			(*i)->find_containers(s, n->containers_, next_id_);
		}
	}
	// Start from leaves
	++next_id_;
	for (vector<Node*>::iterator i = leaves_.begin(); i != leaves_.end(); ++i) {
		if (s >= (*i)->sub_) {
			// Create containees list
			(*i)->find_containees(s, n->containees_, next_id_);
		}
	}
	// Unlink parents and children, if necessary
	for (vector<Node*>::iterator i = n->containers_.begin(); i != n->containers_.end(); ++i) {
		if ((*i)->containees_.empty()) {
			leaves_.erase(std::remove(leaves_.begin(), leaves_.end(), *i), leaves_.end());
		} else {
			for (vector<Node*>::iterator j = n->containees_.begin(); j != n->containees_.end(); ++j)
				(*i)->containees_.erase(std::remove((*i)->containees_.begin(), (*i)->containees_.end(), *j), (*i)->containees_.end());
		}
		(*i)->containees_.push_back(n);
	}
	for (vector<Node*>::iterator i = n->containees_.begin(); i != n->containees_.end(); ++i) {
		if ((*i)->containers_.empty()) {
			roots_.erase(std::remove(roots_.begin(), roots_.end(), *i), roots_.end());
		} else {
			for (vector<Node*>::iterator j = n->containers_.begin(); j != n->containers_.end(); ++j)
				(*i)->containers_.erase(std::remove((*i)->containers_.begin(), (*i)->containers_.end(), *j), (*i)->containers_.end());
		}
		(*i)->containers_.push_back(n);
	}
	// Add to roots or leaves if necessary
	if (n->containers_.empty())
		roots_.push_back(n);
	if (n->containees_.empty())
		leaves_.push_back(n);
}

bool Graph::remove(Subscription& s) {
	// Multiple equivalent subscriptions?
	Node *n = search(roots_, s);
	if (n == NULL)
		return false;
	s.attributes(GRAPH_ATTR, NULL);
	size_--;
	if (n->count_ > 1) {
		n->count_--;
		return true;
	}
	// Remove subscription
	nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), n), nodes_.end());
	// Remove from roots or leaves if necessary
	if (n->containers_.empty())
		roots_.erase(std::remove(roots_.begin(), roots_.end(), n), roots_.end());
	if (n->containees_.empty())
		leaves_.erase(std::remove(leaves_.begin(), leaves_.end(), n), leaves_.end());
	// Remove node from graph
	vector<Node*> parents(n->containers_);
	vector<Node*> children(n->containees_);
	for (vector<Node*>::iterator i = n->containers_.begin(); i != n->containers_.end(); ++i)
		(*i)->containees_.erase(std::remove((*i)->containees_.begin(), (*i)->containees_.end(), n), (*i)->containees_.end());
	for (vector<Node*>::iterator i = n->containees_.begin(); i != n->containees_.end(); ++i)
		(*i)->containers_.erase(std::remove((*i)->containers_.begin(), (*i)->containers_.end(), n), (*i)->containers_.end());
	// Link parents and children, if necessary
	for (vector<Node*>::iterator i = parents.begin(); i != parents.end(); ++i) {
		for (vector<Node*>::iterator j = children.begin(); j != children.end(); ++j) {
			if (!(*j)->descendant(**i))
				(*j)->containers_.push_back(*i);
			if (!(*i)->ancestor(**j))
				(*i)->containees_.push_back(*j);
		}
	}
	// Update roots and leaves if necessary
	for (vector<Node*>::iterator i = parents.begin(); i != parents.end(); ++i) {
		if ((*i)->containees_.empty())
			leaves_.push_back(*i);
	}
	for (vector<Node*>::iterator i = children.begin(); i != children.end(); ++i) {
		if ((*i)->containers_.empty())
			roots_.push_back(*i);
	}
	delete n;
	return true;
}

const Subscription *Graph::search(const Subscription& s) {
	Node *n = search(roots_, s);
	return n == NULL ? NULL : &n->sub_;
}


void Graph::containees(const Subscription& s, vector<const Subscription*>& c)
{
	Node *n = (Node *) s.attributes(GRAPH_ATTR);
	for (vector<Node*>::iterator i = n->containees_.begin(); i != n->containees_.end(); ++i)
		c.push_back(&(*i)->sub_);
}

void Graph::containers(const Subscription& s, vector<const Subscription*>& c)
{
	Node *n = (Node *) s.attributes(GRAPH_ATTR);
	for (vector<Node*>::iterator i = n->containers_.begin(); i != n->containers_.end(); ++i)
		c.push_back(&(*i)->sub_);
}

const Subscription *Graph::primary(const Subscription& s)
{
		Node *n = (Node *) s.attributes(GRAPH_ATTR);
		return &n->sub_;
}

int Graph::duplicates(const Subscription& s)
{
		Node *n = (Node *) s.attributes(GRAPH_ATTR);
		return n->count_;
}

void Graph::match(const Event& e, vector<const Subscription*> *matches) {
	pubs_++;
	stack<Node*> cands;
	int id = ++next_id_;
	int match = 0;
	int test = 0;
	// Start with roots
	for (vector<Node*>::iterator i = roots_.begin(); i != roots_.end(); ++i)
		cands.push(*i);
	// Traverse the containment graph
	while (!cands.empty()) {
		Node *c = cands.top();
		cands.pop();
		if (!c->matched(id)) {
			test++;
			c->tests_++;
			if (c->sub_.matches(e)) {
				if (matches != NULL) {
                    // This was wrong: returned copies of same sub
                    // TEMPORARY solution: vector of subs inside Node type
					for (int i = 0; i < c->count_; i++) {
                        const Subscription *cur = &c->sub_;
                        if( i > 0 ) cur = c->overriden_[i-1];
						matches->push_back( cur );
                    }
				}
				for (vector<Node*>::iterator i = c->containees_.begin(); i != c->containees_.end(); ++i)
					cands.push(*i);
				match += c->count_;
				c->matches_++;
			}
		}
	}
	test_stats_.add((double) test / (double) size_);
	match_stats_.add((double) match / (double) size_);
}

void Graph::stats(Graph::Stats& stats) {
	stats.nodes = nodes_.size();
	stats.roots = roots_.size();
	stats.leaves = leaves_.size();
	stats.singletons = 0;
	for (vector<Node*>::iterator i = roots_.begin(); i != roots_.end(); ++i)
		if ((*i)->containees_.empty())
			stats.singletons++;
	int id = ++next_id_;
	for (vector<Node*>::iterator i = roots_.begin(); i != roots_.end(); ++i)
		(*i)->reset_stats(id);
	for (vector<Node*>::iterator i = roots_.begin(); i != roots_.end(); ++i)
		(*i)->prepare_stats(NULL);
	id = ++next_id_;
	for (vector<Node*>::iterator i = roots_.begin(); i != roots_.end(); ++i)
		(*i)->compute_stats(stats, pubs_, id);
}

void Graph::dump_stats(ostream& stream) {
#ifndef ENCLAVESGX
	int n = 0;
	for (vector<Node*>::const_iterator i = nodes_.begin(); i != nodes_.end(); ++i) {
		for (int j = 0; j < (*i)->count_; j++) {
			stream << "  [" << (n++) << "] depth=[" << (*i)->depth_min_ << "," << (*i)->depth_max_ << "]";
			stream << " matches=" << (*i)->matches_ << " tests=" << (*i)->tests_;
			stream << " containers=" << (*i)->containers_.size() << " containees=" << (*i)->containees_.size() << endl;
		}
	}
#endif
}

ostream& operator<<(ostream& stream, const Node& n) {
#ifndef ENCLAVESGX
	stream << "[" << n.sub_.name() << "#" << n.count_ << ":" << n.sub_ << "] P={";
	vector<Node*> c;
	c = n.containers_;
	sort(c.begin(), c.end(), CompareNodes());
	for (vector<Node*>::const_iterator i = c.begin(); i != c.end(); ++i)
		stream << (i == c.begin() ? "" : ",") << (*i)->sub_.name();
	stream << "} C={";
	c = n.containees_;
	sort(c.begin(), c.end(), CompareNodes());
	for (vector<Node*>::const_iterator i = c.begin(); i != c.end(); ++i)
		stream << (i == c.begin() ? "" : ",") << (*i)->sub_.name();
	stream << "}";
#endif
	return stream;
}

ostream& operator<<(ostream& stream, const Graph& g) {
#ifndef ENCLAVESGX
	stream << "Graph: #" << g.size_ << " (" << g.nodes_.size() << ") ";
	stream << "R={";
	vector<Node*> c;
	c = g.roots_;
	sort(c.begin(), c.end(), CompareNodes());
	for (vector<Node*>::const_iterator i = c.begin(); i != c.end(); ++i)
		stream << (i == c.begin() ? "" : ",") << (*i)->sub_.name();
	stream << "} L={";
	c = g.leaves_;
	sort(c.begin(), c.end(), CompareNodes());
	for (vector<Node*>::const_iterator i = c.begin(); i != c.end(); ++i)
		stream << (i == c.begin() ? "" : ",") << (*i)->sub_.name();
	stream << "}" << endl;
	c = g.nodes_;
	sort(c.begin(), c.end(), CompareNodes());
	for (vector<Node*>::const_iterator i = c.begin(); i != c.end(); ++i)
		stream << "  " << **i << endl;
#endif
	return stream;
}

}
