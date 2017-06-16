#include "subscription.hh"
#include "event.hh"

namespace viper {

bool Subscription::matches(const Event& e) const {
	// Slow match
	const map<key, value>& d = e.data();
	for (subscription::const_iterator i = sub_.begin(); i != sub_.end(); ++i) {
		map<key, value>::const_iterator j = d.find(i->first);
		if (j == d.end())
			return false;
		const predicate& p = i->second;
		switch (p.first) {
			case EXISTS:
				break;
			case EQ:
				if (j->second != p.second)
					return false;
				break;
			case NE:
				if (j->second == p.second)
					return false;
				break;
			case LT:
				if (j->second >= p.second)
					return false;
				break;
			case GT:
				if (j->second <= p.second)
					return false;
				break;
			case LE:
				if (j->second > p.second)
					return false;
				break;
			case GE:
				if (j->second < p.second)
					return false;
				break;
			default:
#ifndef ENCLAVESGX
				cerr << "Unsupported operator: " << p.first << endl;
#endif
				return false;
		}
	}

	return true;
}

bool Subscription::operator>=(const Subscription& s) const {
	for (subscription::const_iterator i = sub_.begin(); i != sub_.end(); ++i) {
		pair<subscription::const_iterator, subscription::const_iterator> j = s.sub_.equal_range(i->first);
		if (j.first == j.second)
			return false;
		const predicate& p1 = i->second;
		bool c;
		for (c = false; j.first != j.second && !c; ++j.first) {
			const predicate& p2 = j.first->second;
			switch (p1.first) {
				case EXISTS:
					c = true;
					break;
				case EQ:
					c = (p2.first == EQ && p2.second == p1.second);
					if ((p2.first == GE || p2.first == LE) && p2.second == p1.second) {
						// "x <= a <= x" is equivalent to "a = x"
						subscription::const_iterator k = j.first;
						for (++k; k != j.second && !c; ++k)
							c = (k->second.first == (p2.first == GE ? LE : GE));
					}
					break;
				case NE:
					c = (p2.first == NE && p2.second == p1.second) ||
						(p2.first == EQ && p2.second != p1.second) ||
						(p2.first == LT && p2.second <= p1.second) ||
						(p2.first == LE && p2.second < p1.second) ||
						(p2.first == GT && p2.second >= p1.second) ||
						(p2.first == GE && p2.second > p1.second);
					break;
				case LT:
					c = (p2.first == LT && p2.second <= p1.second) ||
						((p2.first == LE || p2.first == EQ) && p2.second < p1.second);
					break;
				case LE:
					c = ((p2.first == LT || p2.first == LE || p2.first == EQ) && p2.second <= p1.second);
					break;
				case GT:
					c = (p2.first == GT && p2.second >= p1.second) ||
						((p2.first == GE || p2.first == EQ) && p2.second > p1.second);
					break;
				case GE:
					c = ((p2.first == GT || p2.first == GE || p2.first == EQ) && p2.second >= p1.second);
					break;
				default:
#ifndef ENCLAVESGX
					cerr << "Unsupported operator: " << p1.first << endl;
#endif
					return false;
			}
		}
		if (!c)
			return false;
	}

	return true;
}

ostream& operator<<(ostream& stream, const Subscription& s) {
#ifndef ENCLAVESGX
	int nb = 0;
	stream << "{";
	for (subscription::const_iterator i = s.sub_.begin(); i != s.sub_.end(); ++i) {
		if (nb++ != 0)
			stream << ",";
		stream << i->first;
		const predicate& p = i->second;
		switch (p.first) {
			case EXISTS:
				break;
			case EQ:
				stream << "=" << p.second;
				break;
			case NE:
				stream << "!=" << p.second;
				break;
			case LT:
				stream << "<" << p.second;
				break;
			case GT:
				stream << ">" << p.second;
				break;
			case LE:
				stream << "<=" << p.second;
				break;
			case GE:
				stream << ">=" << p.second;
				break;
			default:
				cerr << "?" << endl;
		}
	}
	stream << "}";
#endif
	return stream;
}

}
