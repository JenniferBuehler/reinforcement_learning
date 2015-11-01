#ifndef __MAX_UTILITY_H__
#define __MAX_UTILITY_H__
//  Copyright Jennifer Buehler

#include <rl/State.h>
#include <rl/Action.h>
#include <rl/StateAlgorithms.h>
#include <rl/Utility.h>
#include <rl/Transition.h>
#include <rl/LogBinding.h>

#include <math/FloatComparison.h>
#include <general/Exception.h>

#define ZERO_EPSILON 1e-07

namespace rl
{

/**
 * For all actions which can be performed, finds the action with maximum utility,
 * considering the probability for the action. Formally, it calculates:
 * max_over_a{sum_over_all_s'[T(s,a,s')*U(s')]}
 * where T is the transition function, and U the utility function
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class MaxUtilityActionAlgorithm: public ActionAlgorithm<Action>
{
public:
    typedef Action ActionT;
    typedef State StateT;
    typedef float FloatT;
    typedef Utility<StateT, FloatT> UtilityT;
    typedef Transition<StateT, ActionT> TransitionT;
    typedef typename TransitionT::StateTransitionListT StateTransitionListT;
    typedef typename TransitionT::StateTransitionListPtrT StateTransitionListPtrT;


    MaxUtilityActionAlgorithm(const UtilityT& _u, const TransitionT& _t, const StateT& _s):
        u(_u), t(_t), s(_s), maxVal(0), maxAction(ActionT()) {}

    virtual bool apply(const ActionT& a)
    {
        StateTransitionListPtrT transitionList;
        if (!t.getTransitionStates(s, a, transitionList)) // No transition states available
        {
            return true;
        }
        if (transitionList->empty())  // no transition available for this state/action
        {
            PRINTERROR("Consistency: No transition states available. Function should have returned false.");
            return false;
        }
        typename StateTransitionListT::iterator it;
        FloatT tmpUt = 0;
        float mean, variance; // mean and variance will be ignored here, but variables are needed
        // PRINTMSG("FROM state "<<s<<", Action "<<a);
        float probCnt = 0.0;
        for (it = transitionList->begin(); it != transitionList->end(); ++it)
        {
            // PRINTMSG("State: "<<it->s<<" with probability "<<it->p);
            tmpUt += it->p * u.getUtility(it->s, mean, variance);
            probCnt += it->p;
        }
        if (!equalFloats(probCnt, 1.0f, static_cast<float>(ZERO_EPSILON)))
        {
            PRINTERROR("Probabilities doo not add up to 1! " << probCnt);
            throw Exception("Abort due to above print error", __FILE__, __LINE__);
        }
        if (tmpUt > maxVal)
        {
            maxVal = tmpUt;
            maxAction = a;
        }
        // PRINTMSG("Applying action "<<a<<" lead to value "<<maxVal);
        return true;
    }
    FloatT getValue()
    {
        return maxVal;
    }
    ActionT getBestAction()
    {
        return maxAction;
    }
private:
    const UtilityT& u;
    const TransitionT& t;
    const StateT& s;
    FloatT maxVal;  // the maximum found value of T(s,a,s')*U(s') for all actions on which this algorithm was applied
    ActionT maxAction;  // the action belonging to the best utility found in apply(Action&)
};
}
#endif
