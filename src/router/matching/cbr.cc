#ifndef ENCLAVESGX
#include <iostream>
#include <fstream>
#endif
#include <string>
#include <vector>
#include <stdlib.h>
#include "cbr.hh"
#include "graph.hh"
#include "prefilter.hh"

int VERBOSE;

void split(std::vector<std::string> &v, const std::string &s ) {
	std::string::const_iterator it = s.begin(), end = s.end();
	std::string t = "";
	for(;it!=end;++it) {
		if( *it == '\t' || *it == ' '  ) {
			if( t.size() ) {
				v.push_back( t );
				t = "";
			}
		} else t += *it;
	}

	if( t.size() ) {
		v.push_back( t );
		t = "";
	}
}

void test() {
#ifndef ENCLAVESGX
	// Create subscriptions
	viper::Subscription s1("s1"), s2("s2"), s3("s3"), s4("s4"), s5("s5"), s6("s6"), s7("s7"), s8("s8"), s9("s9");
	s1.add("a", viper::EQ, 1);
	s2.add("b", viper::NE, 5);
	s3.add("d", viper::EQ, 0);
	s4.add("a", viper::EQ, 1);
	s4.add("b", viper::EQ, 2);
	s4.add("c", viper::EQ, 4);
	s5.add("c", viper::EQ, 3);
	s6.add("a", viper::LE, 5);
	s7.add("b", viper::LE, 5);
	s7.add("c", viper::GT, 2);
	s8.add("c", viper::EQ, 3);
	s9.add("d", viper::LE, 0);
	s9.add("d", viper::GE, 0);

	vector<viper::Subscription*> subs;
	subs.push_back(&s1);
	subs.push_back(&s2);
	subs.push_back(&s3);
	subs.push_back(&s4);
	subs.push_back(&s5);
	subs.push_back(&s6);
	subs.push_back(&s7);
	subs.push_back(&s8);
	subs.push_back(&s9);

	// Create events
	viper::Event e1("e1"), e2("e2"), e3("e3");
	e1.add("a", 1);
	e1.add("b", 2);
	e1.add("c", 3);
	e2.add("a", 1);
	e2.add("b", 2);
	e2.add("c", 4);
	e3.add("a", 0);
	e3.add("b", 0);
	e3.add("c", 0);

	// Test containment
	if (1) {
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			cout << (*i)->name() << " (" << **i << ")" << ":" << endl;
			for (vector<viper::Subscription*>::const_iterator j = subs.begin(); j != subs.end(); ++j) {
				if (**i >= **j)
					cout << "  >= " << (*j)->name() << " (" << **j << ")" << endl;
				if (**i == **j)
					cout << "  == " << (*j)->name() << " (" << **j << ")" << endl;
				if (**i > **j)
					cout << "  >  " << (*j)->name() << " (" << **j << ")" << endl;
			}
		}
	}

	// Test graph
	if (1) {
		// Test graph insertion
		viper::Graph g;
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			g.add(**i);
			cout << g;
		}
		// Test graph search
		viper::Subscription s3b("s3b"), s4b("s4b");
		s3b.add("d", viper::EQ, 0);
		s4b.add("a", viper::EQ, 1);
		s4b.add("b", viper::EQ, 2);
		s4b.add("c", viper::EQ, 4);
		const viper::Subscription *s;
		s = g.search(s3b);
		if (s != NULL)
			cout << s3b.name() << " (" << s3b << ") found: " << s->name()  << " (" << *s << ")" << endl;
		s = g.search(s4b);
		if (s != NULL)
			cout << s4b.name() << " (" << s4b << ") found: " << s->name()  << " (" << *s << ")" << endl;
		// Match events with containment
		g.match(e1, NULL);
		g.match(e2, NULL);
		g.match(e3, NULL);
		// Print stats
		cout << "Matches: " << g.match_stats() << endl;
		cout << "Tests: " << g.test_stats() << endl;
		// Test graph removal
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			g.remove(**i);
			cout << g;
		}
	}

	// Test pre-filter
	if (1) {
		// Test pre-filter insertion
		viper::PreFilter p(128, 3, ALL_NO_CONTAINMENT, NULL);
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			p.add(**i);
			cout << p;
		}
		// Test pre-filter search
		viper::Subscription s3b("s3b"), s4b("s4b");
		s3b.add("d", viper::EQ, 0);
		s4b.add("a", viper::EQ, 1);
		s4b.add("b", viper::EQ, 2);
		s4b.add("c", viper::EQ, 4);
		// Match events with containment
		p.match(e1, NULL);
		p.match(e2, NULL);
		p.match(e3, NULL);
		// Print stats
		cout << "Matches: " << p.match_stats() << endl;
		cout << "Tests: " << p.test_stats() << endl;
		// Test pre-filter removal
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			p.remove(**i);
			cout << p;
		}
	}

	// Test bug fix (Jan 10, 2012)
	if (0) {
		vector<viper::Subscription*> subs;
		viper::Subscription s1("s1"), s2("s2"), s3("s3"), s4("s4"), s5("s5"), s6("s6"), s7("s7"), s8("s8");
		s1.add("symbol", viper::EQ, 160);
		s2.add("symbol", viper::EQ, 160);
		s3.add("symbol", viper::EQ, 160);
		s4.add("symbol", viper::EQ, 160);
		s5.add("symbol", viper::EQ, 160);
		s6.add("symbol", viper::EQ, 160);
		s7.add("symbol", viper::EQ, 160);
		s8.add("symbol", viper::EQ, 160);
		s1.add("high", viper::GE, 440);
		s2.add("high", viper::GE, 344);
		s3.add("high", viper::GE, 430);
		s4.add("high", viper::GE, 336);
		s5.add("high", viper::GE, 197);
		s6.add("high", viper::GE, 39);
		s7.add("high", viper::GE, 364);
		s8.add("high", viper::GE, 365);
		subs.push_back(&s1);
		subs.push_back(&s2);
		subs.push_back(&s3);
		subs.push_back(&s4);
		subs.push_back(&s5);
		subs.push_back(&s6);
		subs.push_back(&s7);
		subs.push_back(&s8);
		viper::Graph g;
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			g.add(**i);
			cout << g;
		}
	}

	// Test bug fix (Jan 11, 2012)
	if (0) {
		vector<viper::Subscription*> subs;
		viper::Subscription s1("s1"), s2("s2"), s3("s3"), s4("s4"), s5("s5");
		s1.add("a", viper::EQ, 1);
		s2.add("b", viper::EQ, 1);
		s3.add("a", viper::EQ, 1);
		s3.add("b", viper::EQ, 1);
		s3.add("c", viper::EQ, 1);
		s4.add("a", viper::EQ, 1);
		s4.add("b", viper::EQ, 1);
		s4.add("c", viper::EQ, 1);
		s4.add("d", viper::EQ, 1);
		s5.add("a", viper::EQ, 1);
		s5.add("b", viper::EQ, 1);
		subs.push_back(&s1);
		subs.push_back(&s2);
		subs.push_back(&s3);
		subs.push_back(&s4);
		subs.push_back(&s5);
		viper::Graph g;
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			g.add(**i);
			cout << g;
		}
	}
#endif
}

viper::Subscription *parse_sub( const vector<string> &v ) {
	if (v.size() >= 7 && v[2] == "S") {
		viper::Subscription *sub = new viper::Subscription(v[1]);
		for (int i = 3; i < v.size(); i += 4) {
			if (v[i] != "i") {
#ifndef ENCLAVESGX
				cerr << "Unsupported operation: " << v[i] << endl;
#endif
				continue;
			}
			int op = -1;
			if (v[i+2] == "=")
				op = viper::EQ;
			else if (v[i+2] == "!=")
				op = viper::NE;
			else if (v[i+2] == ">")
				op = viper::GT;
			else if (v[i+2] == ">=")
				op = viper::GE;
			else if (v[i+2] == "<")
				op = viper::LT;
			else if (v[i+2] == "<=")
				op = viper::LE;
			if (op < 0) {
#ifndef ENCLAVESGX
				cerr << "Unsupported subscription: " << v[i+2] << endl;
#endif
				continue;
			}
			sub->add(v[i+1], (viper::op) op, atoi(v[i+3].c_str()));
		}
		return sub;
	}
	return NULL;
}

bool parse_subs(const char *file, vector<viper::Subscription*> &subs) {
#ifndef ENCLAVESGX
	if (VERBOSE >= 1)
		cout << "Parsing subscription file " << file << " [" << flush;
	string s;
	ifstream f(file);
	if (f.is_open()) {
		while (f.good()) {
			getline(f, s);
			vector<string> v;
			split( v, s );
			viper::Subscription *sub = parse_sub( v );
			if( sub ) {
				subs.push_back(sub);
				if (VERBOSE >= 1 && (subs.size() % 1000) == 0)
					cout << '#' << flush;
			}
		}
		f.close();
	} else {
		cerr << "Cannot open file" << endl;
		return false;
	}
	if (VERBOSE >= 1)
		cout << "]" << endl;
#endif
	return true;
}

viper::Event *parse_pub( const vector<string> &v ) {
	if (v.size() >= 6 && v[2] == "P") {
		viper::Event *pub = new viper::Event(v[1]);
		for (int i = 3; i < v.size(); i += 3) {
			if (v[i] != "i") {
#ifndef ENCLAVESGX
				cerr << "Unsupported operation: " << v[i] << endl;
#endif
				continue;
			}
			pub->add(v[i+1], atoi(v[i+2].c_str()));
		}
		return pub;
	}
	return NULL;
}

bool parse_pubs(const char *file, vector<viper::Event*> &pubs) {
#ifndef ENCLAVESGX
	if (VERBOSE >= 1)
		cout << "Parsing publication file " << file << " [" << flush;
	string s;
	ifstream f(file);
	if (f.is_open()) {
		while (f.good()) {
			getline(f, s);
			vector<string> v;
			split( v, s );
			viper::Event *pub = parse_pub( v );
			if( pub ) {
				pubs.push_back(pub);
				if (VERBOSE >= 1 && (pubs.size() % 1000) == 0)
					cout << '#' << flush;
			}
		}
		f.close();
	} else {
		cerr << "Cannot open file" << endl;
		return false;
	}
	if (VERBOSE >= 1)
		cout << "]" << endl;
#endif
	return true;
}
