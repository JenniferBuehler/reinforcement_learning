#ifndef __POLICY_H__
#define __POLICY_H__
// Copyright Jennifer Buehler

#include <map>
#include <memory>

namespace rl
{

/**
 * Implements a policy: Which Action to perform in which State.
 * This can be implemented by looking up a State in a table, or by a learning function
 * which generates Actions out of States.
 *
 * \param State template class for State description. Prerequisite: Must support < operator,
 * an be uniquely identifiable, i.e. suitable to use in a std::map as key. Even if the Reward
 * is being calculated as a function by the underlying implementation, it is to be made sure
 * that a map-lookup can be used as well for each state.
 * \param Action represents an action to be performed in a state. Can be any datatype, but it should
 * provide the assignment operator and a copy constructor (if it is a class).
 * It is not intended to use complex Action datatypes here (if you have complex ones, use an index to the
 * complex action instead), as the datatype is internally copied.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class Policy
{
public:
    typedef State StateT;
    typedef Action ActionT;
    typedef Policy<StateT, ActionT> PolicyT;
    typedef std::shared_ptr<PolicyT> PolicyPtrT;
    typedef std::shared_ptr<const PolicyT> PolicyConstPtrT;

    Policy() {}
    Policy(const Policy& p) {}
    virtual ~Policy() {}
    /**
     * Get the action to perform in a certain state. If non action
     * assigned to this state, returns false. Otherwise, will contain
     * the action to be performed in targetAction.
     */
    virtual bool getAction(const State& s, Action& targetAction) const = 0;

    /**
     * Under certain circumstances, the Action a was determined best for the
     * State s. This should lead to an update of the policy, either by inserting/replacing
     * this action in a table, or by feeding a learning algorithm. If available, a utility
     * estimate for this state-action pair can be passed for use by learning algorithms,
     * along with a confidence value [0..1] that this action will successfully lead to this utility.
     * This could be used to trade off the choice of an action using a risk factor, when
     * returning an action in getAction(State&).
     * In a simple state-action lookup implementation, the parameters confidence and utility
     * won't have any effect.
     */
    virtual void bestAction(const State& s, const Action& a,  float utility = 1.0, float confidence = 1.0) = 0;

    virtual PolicyPtrT clone() const = 0;

    friend std::ostream& operator<<(std::ostream& o, const PolicyT& p)
    {
        p.print(o);
        return o;
    }
    virtual void print(std::ostream& o) const = 0;

protected:


};


/**
 * Simple table lookup policy. bestAction() replaces an action for a state.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class LookupPolicy: public Policy<State, Action>
{
public:
    typedef State StateT;
    typedef Action ActionT;
    typedef Policy<StateT, ActionT> PolicyT;
    typedef typename PolicyT::PolicyPtrT PolicyPtrT;

    typedef LookupPolicy<StateT, ActionT> LookupPolicyT;
    typedef std::shared_ptr<LookupPolicyT> LookupPolicyPtrT;
    typedef std::shared_ptr<const LookupPolicyT> LookupPolicyConstPtrT;

    LookupPolicy(): PolicyT() {}
    LookupPolicy(const LookupPolicy& o): PolicyT(o), p(o.p) {}
    virtual ~LookupPolicy() {}

    virtual bool getAction(const State& s, Action& targetAction) const
    {
        typename PolicyMapT::const_iterator it = p.find(s);
        if (it == p.end()) return false;
        targetAction = it->second;
        return true;
    }
    virtual void bestAction(const State& s, const Action& a,  float utility = 1.0, float confidence = 1.0)
    {
        typename PolicyMapT::iterator it = p.find(s);
        if (it == p.end()) //no such action yet: insert it in the table
        {
            p.insert(std::make_pair(s, a));
            return;
        }
        //replace action in table
        it->second = a;
    }
    virtual PolicyPtrT clone() const
    {
        return PolicyPtrT(new LookupPolicyT(*this));
    }

    virtual void print(std::ostream& o) const
    {
        typename PolicyMapT::const_iterator it;
        for (it = p.begin(); it != p.end(); ++it)
        {
            o << it->first << " -> " << it->second << std::endl;
        }
    }
protected:
    typedef std::map<State, Action> PolicyMapT;
    PolicyMapT p;
};

}
#endif
