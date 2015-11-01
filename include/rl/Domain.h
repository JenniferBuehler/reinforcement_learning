#ifndef RL_DOMAIN_H
#define RL_DOMAIN_H
// Copyright Jennifer Buehler

#include <rl/Transition.h>
#include <rl/Reward.h>
#include <rl/StateAlgorithms.h>

namespace rl
{

/**
 * \brief Base class for all domains, implementing certain aspects which need
 * domain knowledge.
 *
 * The following typedefs must be provided by all implementing subclasses:
 * - StateT a typedef over the State template parameter
 * - ActionT a typedef over the Action template parameter
 * - RewardValueTypeT the reward value type which is returned by the reward function
 * - TransitionT correctly parameterized Transition (base class!)
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class Domain
{
public:
    typedef State StateT; //state of the world, subclass of StateBase
    typedef Action ActionT; //actions, subclass of ActionBase
    typedef float RewardValueTypeT; //the value type of a reward received

    typedef Domain<StateT, ActionT> DomainT;
    typedef std::shared_ptr<DomainT> DomainPtrT;
    typedef std::shared_ptr<const DomainT> DomainConstPtrT;

    typedef Transition<StateT, ActionT, float> TransitionT; //transition function
    typedef Reward<StateT, RewardValueTypeT> RewardT;
    typedef StateGenerator<StateT> StateGeneratorT;
    typedef ActionGenerator<ActionT> ActionGeneratorT;

    typedef typename TransitionT::TransitionConstPtrT TransitionConstPtrT;
    typedef typename RewardT::RewardConstPtrT RewardConstPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;
    typedef typename StateGeneratorT::StateGeneratorConstPtrT StateGeneratorConstPtrT;

    Domain() {}
    virtual ~Domain() {}
    /**
     * NULL if no default transition function is provided
     * by domain. This means only learning algorithms can
     * be used which learn the transition function.
     */
    virtual TransitionConstPtrT getTransition()const = 0;
    /**
     * NULL if no default reward function is provided
     * by domain. This means only learning algorithms can
     * be used which learn the reward function.
     */
    virtual RewardConstPtrT getReward()const = 0;

    virtual StateGeneratorConstPtrT getStateGenerator()const = 0;
    virtual ActionGeneratorConstPtrT getActionGenerator()const = 0;

    /**
     * returns a default start state for the world, or the
     * start state which was explicitly set in the domain
     */
    virtual StateT getStartState()const = 0;

    /**
     * Based on the domain knowledge, an action is to be performed from
     * the current state (parameter currState) to lead to a new state.
     * The new state is returned. In terminal states, the same state should
     * be returned, because we can't transfer out of it.
     */
    virtual StateT transferState(const StateT& currState, const ActionT& action) = 0;

    /**
     * returns true if this state is terminal
     */
    virtual bool isTerminalState(const StateT& s)const = 0;
};


}  // namespace rl
#endif  // RL_DOMAIN_H
