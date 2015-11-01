#ifndef __TRANSITION__H__
#define __TRANSITION__H__
// Copyright Jennifer Buehler

#include <map>
#include <deque>
#include <memory>
#include <utility>

#include <rl/StateActionPair.h>
#include <rl/LogBinding.h>
#include <general/Exception.h>

namespace rl
{


/**
 * Interface for a transition from a state, performing an action,
 * leading to a target state.
 * This must use knowledge of the underlying domain to determine
 * to which destination statees an action will lead with which probability.
 *
 * It is left to the implementing subclass how  to manage and store
 * the transition states. To access a list of states s', which
 * can be reched from state-action pair (s,a) with probability p,
 * a queue of such states is returned (StateTransitionListT). See method
 * getTransitionStates().
 * This class can also be used as an interface to learn the transition
 * function, by using setTransitionState() (after an experienced state transition).
 *
 * It is not specified whether the list StateTransitionListT will be ordered
 * in any way, because it is merely intended to iterate through possible target states.
 * The use of smart pointers ensures that the implementing subclass can either
 * return a new structure of StateTransitionListT (a COPY of internally used structure)
 * or, alternatively, a pointer to an internally stored StateTransitionListT.
 *
 * \param State template class for State description. Prerequisite: Must support < operator,
 * an be uniquely identifiable, i.e. suitable to use in a std::map as key
 * \param Action template class for Action description. Prerequisite: Must support < operator,
 * an be uniquely identifiable, i.e. suitable to use in a std::map as key
 * \param StateActionStateValue the value assigned to a state-action-state pair. This can
 * be either a probability (float) or a number (counting of state assignments)
 *
 * \author Jennifer Buehler
 * \date May 2011
 *
 */
template<class State, class Action, typename StateActionStateValue = float>
class Transition
{
public:
    Transition() {}
    Transition(const Transition& o) {}
    virtual ~Transition() {}
    typedef State StateT;
    typedef Action ActionT;
    typedef StateActionStateValue StateActionStateValueT;
    typedef Transition<StateT, ActionT, StateActionStateValue> TransitionT;
    typedef std::shared_ptr<TransitionT> TransitionPtrT;
    typedef std::shared_ptr<const TransitionT> TransitionConstPtrT;

    class StateTransition
    {
    public:
        explicit StateTransition(const State& _s, const StateActionStateValueT& _p): s(_s), p(_p) {}
        StateTransition(const  StateTransition& o): s(o.s), p(o.p) {}

        bool operator < (const StateTransition& t) const
        {
            return s < t.s;
        }
        friend std::ostream& operator<<(std::ostream& o, const StateTransition& t)
        {
            o << t.s << " with p=" << t.p;
            return o;
        }
        State s; //State
        StateActionStateValueT p; //Probability or frequency
    private:
    };

    typedef StateTransition StateTransitionT;
    typedef std::deque<StateTransitionT> StateTransitionListT;
    typedef std::shared_ptr<StateTransitionListT> StateTransitionListPtrT;

    /**
     * \return false if no such transition states exist (remain in same state with this action),
     * and true if parameter ret is initialised with the result set.
     */
    virtual bool getTransitionStates(const State& s, const Action& a, StateTransitionListPtrT& ret) const = 0;

    /**
     * Adds a transition state, or if this transition (s1,a,s2) does exist, the assigned value p is updated.
     */
    virtual void setTransitionState(const State& s1, const Action& a, const State& s2, StateActionStateValueT p = 1) = 0;

    /**
     * prints the transition map
     */
    virtual void print(std::ostream& o)const = 0;

protected:

};



/**
 * This implementation can be used to store a learned transition map (by experienced state transitions)
 * by simply storing the observed frequencies/probabilities in a map.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action, typename StateActionStateValue = float>
class TransitionStlMap: public Transition<State, Action, StateActionStateValue>
{
public:

    typedef Transition<State, Action, StateActionStateValue> ParentT;
    typedef typename ParentT::StateTransitionListT StateTransitionListT;
    typedef typename ParentT::StateTransitionT StateTransitionT;
    typedef typename ParentT::StateActionStateValueT StateActionStateValueT;
    typedef typename ParentT::StateTransitionListPtrT StateTransitionListPtrT;

    TransitionStlMap() {}
    TransitionStlMap(const TransitionStlMap& o): t(o.t), ParentT(o) {}
    virtual ~TransitionStlMap() {}

    virtual bool getTransitionStates(const State& s, const Action& a, StateTransitionListPtrT& ret) const
    {
        typename TransitionMapT::const_iterator it = t.find(StateActionPairT(s, a));

        if (it->second->size() > 1)
        {
            PRINTMSG("WE HAVE THIS CASE!!!");
        }
        if (it == t.end()) return false;
        ret = it->second;
        return true;
    }

    virtual void setTransitionState(const State& s1, const Action& a, const State& s2, StateActionStateValueT p = 1)
    {
        std::pair<typename TransitionMapT::iterator, bool> mit = t.insert(std::make_pair(StateActionPairT(s1, a), new StateTransitionListT()));
        StateTransitionListPtrT tList = mit.first->second;
        //PRINTMSG("Create "<<s1<<", "<<a);
        if (mit.second)  //no such entry (s1,a) existed yet, so we can simply push it back.
        {
            //XXX inefficient: try insert first, and only if it does not fail, then we can update the
            //entry which is returned by std::map::insert().
            tList->push_back(StateTransitionT(s2, p));
            return;
        }
        //entry exists: See whether the same state s2 exists in the queue.
        //If so, only probability has to be updated.
        typename StateTransitionListT::iterator lit;
        for (lit = tList->begin(); lit != tList->end(); ++lit)
        {
            //PRINTMSG("Transfer "<<*lit);
            if (equal(lit->s, s2))
            {
                //PRINTMSG("Equal "<<lit->s<<", "<<s2);
                lit->p = p;
                return;
            }
        }
        //this transition state does not exist. Add it to the list.
        tList->push_back(StateTransitionT(s2, p));
    }

    virtual void print(std::ostream& o)const
    {
        typename TransitionMapT::const_iterator it;
        for (it = t.begin(); it != t.end(); ++it)
        {
            typename StateTransitionListT::iterator lit;
            for (lit = it->second->begin(); lit != it->second->end(); ++lit)
            {
                o << it->first << ":  " << lit->s << " / " << lit->p << std::endl;
            }
        }
    }
    virtual bool getTransitionStatesNonConst(const State& s, const Action& a, StateTransitionListPtrT& ret)
    {
        typename TransitionMapT::iterator it = t.find(StateActionPairT(s, a));
        if (it == t.end()) return false;
        ret = it->second;
        return true;
    }
protected:


    typedef StateActionPair<State, Action>  StateActionPairT;
    //helper: == operator just using the < operator
    static bool equal(const State& s1, const State& s2)
    {
        return !(s1 < s2) && !(s2 < s1);
    }

    typedef std::map<StateActionPairT, StateTransitionListPtrT> TransitionMapT;
    TransitionMapT t;
};




/**
 * \brief A transition map which can be learned. Implements TransitionStlMap for
 * holding probabilities, and uses another TransitionStlMap (with StateActionStateValue = integer)
 * to count the total number experienceTransition() was called on a transition.
 *
 * Based on the counting, the probability map is kept up to date.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class LearnableTransitionMap: public TransitionStlMap<State, Action, float>
{
public:

    typedef float ProbabilityT;
    typedef TransitionStlMap<State, Action, ProbabilityT> ParentT;
    typedef unsigned int CounterT;
    typedef TransitionStlMap<State, Action, CounterT> CountingTransitionMapT;

    typedef typename ParentT::StateTransitionT StateTransitionT;
    typedef typename ParentT::StateTransitionListT StateTransitionListT;
    typedef typename ParentT::StateTransitionListPtrT StateTransitionListPtrT;
    typedef typename ParentT::StateActionStateValueT StateActionStateValueT;


    LearnableTransitionMap() {}
    LearnableTransitionMap(const LearnableTransitionMap& o): countingMap(o.countingMap), ParentT(o) {}
    virtual ~LearnableTransitionMap() {}

    void experienceTransition(const State& s1, const Action& a, const State& s2)
    {
        //PRINTMSG("Experience "<<s2<<", "<<a<<" -> "<<s2);
        typedef typename CountingTransitionMapT::StateTransitionListT CountingStateTransitionListT;
        typedef typename CountingTransitionMapT::StateTransitionListPtrT CountingStateTransitionListPtrT;
        CountingStateTransitionListPtrT cs = NULL;
        countingMap.getTransitionStatesNonConst(s1, a, cs);

        CounterT cntTotal = 0; //counter for total amount of trials s1/a
        if (!cs.get())  //no such transition was counted yet
        {
            countingMap.setTransitionState(s1, a, s2, 0); //set to 0, because it is going to be counted up below
            cntTotal = 1;
            cs = NULL;
            countingMap.getTransitionStatesNonConst(s1, a, cs);
            if (!cs.get() || (cs->size() != 1))
            {
                throw Exception("This time, the list should not be empty!", __FILE__, __LINE__);
            }
            setTransitionState(s1, a, s2, 0.0); //initialise new entry for probabilities table, initially 0
        }

        /*if (cs->size()>4) {
            throw Exception("Too many target states! ",__FILE__,__LINE__);
        }*/

        //now, update probability map.

        bool targetExists = false;
        //first, count all occurrences of s1,a so far, and increase occurrence of s1,a,s2 by 1
        typename CountingStateTransitionListT::iterator csit = cs->begin();
        //PRINTMSG("Test for "<<s1<<" / "<<a<<" -> "<<s2);
        for (; csit != cs->end(); ++csit)
        {
            if (this->equal(csit->s, s2))
            {
                csit->p = csit->p + 1; //transition experienced once more, right now
                targetExists = true;
            }
            //PRINTMSG(*csit);
            cntTotal += csit->p; //increase total count of state-action trials
        }

        if ((cs->size() > 1) && (cntTotal <= 1))
        {
            PRINTERROR("total cnt=" << cntTotal << ", size=" << cs->size());
            throw Exception("Total CNT should be higher! ", __FILE__, __LINE__);
        }


        //now, update all probabilities
        StateTransitionListPtrT ps;
        getTransitionStatesNonConst(s1, a, ps);
        if (ps->size() != cs->size())
        {
            throw Exception("Inconsistency in maps! ", __FILE__, __LINE__);
        }

        if (!targetExists)  //we don't have target state s2 yet, though s1/a has been tried before. Insert it in both maps.
        {
            cs->push_back(typename CountingTransitionMapT::StateTransitionT(s2, 1));
            ps->push_back(StateTransitionT(s2, 0.0));
        }

        typename StateTransitionListT::iterator psit = ps->begin();
        csit = cs->begin();
        for (; (csit != cs->end()) && (psit != ps->end()); ++csit, ++psit)
        {
            if (!this->equal(csit->s, psit->s))
            {
                throw Exception("Inconsistency in maps! ", __FILE__, __LINE__);
            }
            //PRINTMSG("Update: "<<*psit<<" / "<<*csit<<", total cnt = "<<cntTotal);
            psit->p = (ProbabilityT)csit->p / (ProbabilityT)cntTotal;
        }

        if ((psit != ps->end()) || (csit != cs->end()))
        {
            throw Exception("Inconsistency in maps! ", __FILE__, __LINE__);
        }
    }

    virtual void print(std::ostream& o)const
    {
        //countingMap.print(o);
        //o<<std::endl<<"PROBABILITIES: "<<std::endl;
        ParentT::print(o);
    }


protected:
    CountingTransitionMapT countingMap;
};









}
#endif
