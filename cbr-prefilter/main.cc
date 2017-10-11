#include <stdlib.h>
#include "cbr.hh"
#include "prefilter.hh"
#include <getopt.h>
#include <sys/time.h>

int main(int argc, char* argv[]) {
	struct option long_options[] = {
		// These options set a flag
		{"silent",       no_argument, &VERBOSE, 0},
		{"verbose",      no_argument, &VERBOSE, 1},
		{"debug",        no_argument, &VERBOSE, 2},
		{"DEBUG",        no_argument, &VERBOSE, 3},
		// These options don't set a flag
		{"help",         no_argument, 0, 'h'},
		{"test",         no_argument, 0, 't'},
		{"containment",  no_argument, 0, 'c'},
		{"prefiltering", no_argument, 0, 'p'},
		{"dump-stats",   no_argument, 0, 'd'},
		{"bloom-size",   required_argument, 0, 'b'},
		{"num-hashes",   required_argument, 0, 'n'},
		{"variant",      required_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	bool containment = false;
	bool prefiltering = false;
	bool dump_stats = false;
	int bloom_size = 128;
	int num_hashes = 3;
	int variant = ALL_NO_CONTAINMENT;

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "htcpdb:n:v:", long_options, &option_index);

		if (c == -1)
			break;

		if (c == 0 && long_options[option_index].flag == 0)
			c = long_options[option_index].val;

		switch (c) {
			case 0:
				// Flag is automatically set
				break;
 			case 'h':
 				cout << "Usage: " << argv[0] << " [options...] subscriptions [publications...]" << endl
					<< "  --silent" << endl
					<< "  --verbose" << endl
					<< "  --debug" << endl
					<< "  --DEBUG" << endl
					<< "        Print no, little, or much debug information (default=silent)" << endl
					<< "  -h, --help" << endl
					<< "        Print this help message" << endl
					<< "  -t, --test" << endl
					<< "        Execute some unit tests and exit" << endl
					<< "  -c, --containment" << endl
					<< "        Run with containment" << endl
					<< "  -p, --prefiltering" << endl
					<< "        Run with prefiltering" << endl
					<< "  -d, --dump-stats" << endl
					<< "        Print detailed statistics" << endl
					<< "  -b, --bloom-size <int>" << endl
					<< "        Number of bits in bloom filter (default=128)" << endl
					<< "  -n, --num-hashes <int>" << endl
					<< "        Number of hash functions (default=3)" << endl
					<< "  -v, --variant <int>" << endl
					<< "        Variant of prefiltering algorithm (default=0)" << endl
					<< "          0: all bits and no containment" << endl
					<< "          1: one bit and no containment" << endl
					<< "          2: all bits and containment for negative matching" << endl
					<< "          3: all bits and containment for negative and positive matching" << endl
					;
				exit(0);
			case 't':
				test();
				exit(0);
			case 'c':
				containment = true;
				break;
			case 'p':
				prefiltering = true;
				break;
			case 'd':
				dump_stats = true;
				break;
			case 'b':
				bloom_size = atoi(optarg);
				break;
			case 'n':
				num_hashes = atoi(optarg);
				break;
			case 'v':
				variant = atoi(optarg);
				break;
			case '?':
				cout << "Use -h or --help for help" << endl;
				exit(0);
			default:
				exit(1);
		}
	}
	if (optind >= argc) {
		cerr << "Not enough arguments (use -h for help)" << endl;
		exit(1);
	}
	if (bloom_size < 1 || num_hashes < 1 || num_hashes > bloom_size || variant < 0 || variant > 3) {
		cerr << "Invalid value for arguments (use -h for help)" << endl;
		exit(1);
	}

	vector<viper::Subscription*> subs;
	if (!parse_subs(argv[optind++], subs))
		exit(1);
	if (VERBOSE >= 1)
		cout << "  ...parsed " << subs.size() << " subscriptions" << endl;
	if (VERBOSE >= 3) {
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i)
			cout << "  " << (*i)->name() << " (" << **i << ")" << endl;
	}

	vector<viper::Event*> pubs;
	while (optind < argc) {
		if (!parse_pubs(argv[optind++], pubs))
			exit(1);
		if (VERBOSE >= 1)
			cout << "  ...parsed " << pubs.size() << " publications" << endl;
	}
	if (VERBOSE >= 3) {
		for (vector<viper::Event*>::const_iterator i = pubs.begin(); i != pubs.end(); ++i)
			cout << "  " <<  (*i)->name() << " (" << **i << ")" << endl;
	}

	viper::SmartCounter nb_pred;
	viper::SmartCounter nb_eq;
	for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
		int n = 0;
		for (viper::subscription::const_iterator j = (*i)->data().begin(); j != (*i)->data().end(); ++j) {
			if (j->second.first == viper::EQ)
				n++;
		}
		nb_pred.add((*i)->data().size());
		nb_eq.add(n);
	}
	cout << "Subscription stats" << endl;
	cout << "  #predicates  : " << nb_pred << endl;
	cout << "  #equality    : " << nb_eq << endl;

	viper::Graph *g = NULL;
	viper::PreFilter *p = NULL;

	if (containment || (prefiltering && variant > 1)) {
		if (VERBOSE >= 1)
			cout << "Constructing graph [" << flush;
		g = new viper::Graph();
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			g->add(**i);
			if (VERBOSE >= 1 && (g->size() % 1000) == 0)
				cout << '#' << flush;
		}
		if (VERBOSE >= 1)
			cout << "]" << endl << "  ...graph complete" << endl;
	}

	if (containment) {
		if (!pubs.empty()) {
			vector<const viper::Subscription*> *matches = NULL;
			if (dump_stats || VERBOSE >= 3)
				matches = new vector<const viper::Subscription*>();
			if (VERBOSE >= 1)
				cout << "Matching publications" << endl;
			int n = 0;
			for (vector<viper::Event*>::const_iterator i = pubs.begin(); i != pubs.end(); ++i) {
				if (VERBOSE >= 3)
					cout << "  " << (*i)->name() << " (" << **i << ")" << endl;
				if (matches != NULL)
					matches->clear();
				g->match(**i, matches);
				if (dump_stats)
					cout << "  [" << (n++) << "] matches " << matches->size() << " subscriptions" << endl;
				if (VERBOSE >= 3) {
					for (vector<const viper::Subscription*>::const_iterator j = matches->begin(); j != matches->end(); ++j)
						cout << "    " << (*j)->name() << " (" << **j << ")" << endl;
				}
			}
			delete matches;
		}

		viper::Graph::Stats stats;
		g->stats(stats);
		cout << "Graph stats" << endl;
		cout << "  #subs        : " << g->size() << endl;
		cout << "  #nodes       : " << stats.nodes << endl;
		cout << "  #roots       : " << stats.roots << endl;
		cout << "  #leaves      : " << stats.leaves << endl;
		cout << "  #singletons  : " << stats.singletons << endl;
		cout << "  depth-min    : " << stats.depth_min << endl;
		cout << "  depth-max    : " << stats.depth_max << endl;
		cout << "  #containees  : " << stats.containees << endl;
		cout << "    non-leaf   : " << (stats.leaves == stats.nodes ? 0 : (stats.containees.mean() * stats.containees.samples() / (stats.containees.samples() - stats.leaves))) << endl;
		cout << "  #containers  : " << stats.containers << endl;
		cout << "    non-root   : " << (stats.roots == stats.nodes ? 0 : (stats.containers.mean() * stats.containers.samples() / (stats.containers.samples() - stats.roots))) << endl;
		cout << "  #identical   : " << stats.count << endl;
		if (VERBOSE >= 3)
			cout << *g;

		if (!pubs.empty()) {
			cout << "Matching stats per publication (relative over " << g->size() << " subscriptions)" << endl;
			cout << "  #matches-p   : " << g->match_stats() << endl;
			cout << "  #tests-p     : " << g->test_stats() << endl;
			cout << "Matching stats per subscription (relative over " << pubs.size() << " publications)" << endl;
			cout << "  #matches-s   : " << stats.matches << endl;
			cout << "  #tests-s     : " << stats.tests << endl;
		}

		if (dump_stats)
			g->dump_stats(cout);
	}

	if (prefiltering) {
		if (VERBOSE >= 1)
			cout << "Constructing pre-filter [" << flush;
		p = new viper::PreFilter(bloom_size, num_hashes, variant, g);
		for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
			p->add(**i);
			if (VERBOSE >= 1 && (p->size() % 1000) == 0)
				cout << '#' << flush;
		}
		if (VERBOSE >= 1)
			cout << "]" << endl << "  ...pre-filter complete" << endl;

		if (!pubs.empty()) {
			vector<const viper::Subscription*> *matches = NULL;
			if (dump_stats || VERBOSE >= 3)
				matches = new vector<const viper::Subscription*>();
			if (VERBOSE >= 1)
				cout << "Matching publications" << endl;
			struct timeval starttime;
			struct timeval endtime;
			int n = 0;
			gettimeofday(&starttime, NULL);
			for (vector<viper::Event*>::const_iterator i = pubs.begin(); i != pubs.end(); ++i) {
				if (VERBOSE >= 3)
					cout << "  " << (*i)->name() << " (" << **i << ")" << endl;
				if (matches != NULL)
					matches->clear();
				p->match(**i, matches);
				if (dump_stats)
					cout << "  [" << (n++) << "] matches " << matches->size() << " subscriptions" << endl;
				if (VERBOSE >= 3) {
					for (vector<const viper::Subscription*>::const_iterator j = matches->begin(); j != matches->end(); ++j)
						cout << "    " << (*j)->name() << " (" << **j << ")" << endl;
				}
			}
			gettimeofday(&endtime, NULL);
			double time = ((endtime.tv_sec - starttime.tv_sec) * 1000.0) + ((endtime.tv_usec - starttime.tv_usec) / 1000.0);
			if (VERBOSE >= 1)
				cout << "Matching time: " << time << endl;
			delete matches;
		}

		viper::PreFilter::Stats stats;
		p->stats(stats);
		cout << "Pre-filter stats" << endl;
		cout << "  #subs        : " << p->size() << endl;
		cout << "  #empty-lists : " << stats.empty_lists << endl;
		cout << "  noBF-size    : " << stats.nobf_size << endl;
		cout << "  lists-size   : " << stats.list_size << endl;
		if (VERBOSE >= 3)
			cout << *p;

		if (!pubs.empty()) {
			cout << "Matching stats per publication (relative over " << p->size() << " subscriptions)" << endl;
			cout << "  #matches-p   : " << p->match_stats() << endl;
			cout << "  #tests-p     : " << p->test_stats() << endl;
			cout << "Matching stats per subscription (relative over " << pubs.size() << " publications)" << endl;
			cout << "  #matches-s   : " << stats.matches << endl;
			cout << "  #tests-s     : " << stats.tests << endl;
		}

		if (dump_stats)
			p->dump_stats(cout);
	}

	delete g;
	delete p;

	for (vector<viper::Event*>::const_iterator i = pubs.begin(); i != pubs.end(); ++i)
		delete *i;
	for (vector<viper::Subscription*>::const_iterator i = subs.begin(); i != subs.end(); ++i)
		delete *i;

	return 0;
}
