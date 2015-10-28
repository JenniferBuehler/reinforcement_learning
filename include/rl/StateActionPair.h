#ifndef __STATE_ACTION_PAIR_H__
#define __STATE_ACTION_PAIR_H__
#include <iostream>

namespace rl {

/**
 * \brief A pair of state and action which can be used as a key for stl maps.
 * Prerequisite is that both State and Action support the < operator.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class StateActionPair{
public:
	explicit StateActionPair(const State& _s, const Action& _a): s(_s),a(_a){}
	StateActionPair(const StateActionPair& o): s(o.s),a(o.a){}

	StateActionPair& operator=(const StateActionPair& o){
		if (this==&o) return *this;
		s=o.s;
		a=o.a;
		return *this;
	}

	bool operator < (const StateActionPair& p) const{
		return (s<p.s) || 
			(!(p.s<s) && (a<p.a)); //!(p.s<s) assesses s==p.s at this point in expression, because (s<p.s) above
	}
	friend std::ostream& operator<<(std::ostream& o, const StateActionPair& p){
		o<<p.s<<" / "<<p.a;
		return o;
	}
	State s;
	Action a;
private:
	StateActionPair(){}
};

}

#endif
