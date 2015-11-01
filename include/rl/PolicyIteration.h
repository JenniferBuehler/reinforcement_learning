#ifndef __POLICY_ITERATION_H__
#define __POLICY_ITERATION_H__
//  Copyright Jennifer Buehler

#include <rl/MaxUtility.h>
#include <rl/State.h>
#include <rl/Action.h>
#include <rl/StateAlgorithms.h>
#include <rl/Utility.h>
#include <rl/Reward.h>
#include <rl/Policy.h>
#include <rl/Transition.h>
#include <rl/LogBinding.h>

#include <math/FloatComparison.h>

#include <assert.h>
#include <math.h>

namespace rl
{

/**
 * \brief Implements one iteration update for policy iteration algorithm.
 * method preApplication() has to be called, before this update
 * is applied to all states of the domain.
 * method isChanged() can be used to see whether no changes have been
 * made on the policy during an update.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class PolicyIterationUpdate: public StateAlgorithm<State>
{
public:
    typedef State StateT;
    typedef Action ActionT;
    typedef PolicyIterationUpdate<StateT, ActionT> PolicyIterationUpdateT;
    typedef std::shared_ptr<PolicyIterationUpdateT> PolicyIterationUpdatePtrT;
    typedef std::shared_ptr<const PolicyIterationUpdateT> PolicyIterationUpdateConstPtrT;

    typedef float FloatT;

    typedef Utility<StateT, FloatT> UtilityT;
    typedef Reward<StateT, FloatT> RewardT;
    typedef Transition<StateT, ActionT> TransitionT;
    typedef typename TransitionT::StateTransitionListT StateTransitionListT;
    typedef ActionGenerator<ActionT> ActionGeneratorT;
    typedef MaxUtilityActionAlgorithm<StateT, ActionT> MaxUtilityActionAlgorithmT;
    typedef Policy<StateT, ActionT> PolicyT;

    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;
    typedef typename TransitionT::TransitionConstPtrT TransitionConstPtrT;
    typedef typename PolicyT::PolicyPtrT PolicyPtrT;
    typedef typename UtilityT::UtilityPtrT UtilityPtrT;

    /**
     * \param u correctly initialised (but still empty/unlearned) model of utility.
     * This object will be changed in the course of applying this StateAlgorithm.
     * \param p correctly initialised (but still empty/unlearned) policy.
     * This object will be changed in the course of applying this StateAlgorithm.
     * \param t correctly initialised transition model.
     * \param ag action generator (generates all possible actions) for domaon.
     */
    PolicyIterationUpdate(UtilityPtrT& u,
                          const TransitionConstPtrT& t,
                          const PolicyPtrT& p,
                          const ActionGeneratorConstPtrT& ag):
        utility(u), transition(t), policy(p), actionGenerator(ag), unchanged(true)
    {

        assert(utility.get());
        assert(policy.get());
        assert(transition.get());
        assert(actionGenerator.get());
    }
    virtual ~PolicyIterationUpdate()
    {
    }
    /**
     * Has to be called before applying all states (see method apply(StateT&) ).
     * Evaluates the policy.
     */
    void preApplication()
    {
        unchanged = true;
    }


    virtual bool apply(const StateT& s)
    {
        // PRINTMSG("Process state "<<s);

        /*std::stringstream strng;
        utility->print(strng);
        PRINTMSG("-----Utility = ");
        PRINTMSG(strng.str());*/


        MaxUtilityActionAlgorithmT maxActionUt(*utility, *transition, s);
        if (!actionGenerator->foreachAction(maxActionUt))
        {
            PRINTERROR("Could not apply summation on all actions");
            return false;
        }
        FloatT maxActionUtVal = maxActionUt.getValue();

        MaxUtilityActionAlgorithmT maxPolicyUt(*utility, *transition, s);
        Action targetAction;
        if (!policy->getAction(s, targetAction))
        {
            PRINTERROR("No action returned for the state " << s << ", make sure policy returns at least random value!");
            return false;
        }

        maxPolicyUt.apply(targetAction);
        FloatT maxPolicyUtVal = maxPolicyUt.getValue();

        if (maxActionUtVal > maxPolicyUtVal)
        {
            float util = 0, confidence = 0; // values not used but need to be declared
            policy->bestAction(s, maxActionUt.getBestAction(), util, confidence);
            unchanged = false;
        }
        return true;
    }
    bool isUnchanged()
    {
        return unchanged;
    }
    void setUtility(UtilityPtrT ut)
    {
        utility = ut;
    }
    PolicyPtrT getPolicy()
    {
        return policy;
    }
protected:


private:
    PolicyIterationUpdate() {}
    UtilityPtrT utility;
    const TransitionConstPtrT transition;
    PolicyPtrT policy;
    const ActionGeneratorConstPtrT actionGenerator;
    bool unchanged;
};



/**
 * Initialises a policy for each state with a random action
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class PolicyInitialisation: public StateAlgorithm<State>
{
public:
    typedef State StateT;
    typedef Action ActionT;
    typedef Policy<StateT, ActionT> PolicyT;
    typedef ActionGenerator<ActionT> ActionGeneratorT;
    typedef PolicyInitialisation<StateT, ActionT> PolicyInitialisationT;
    typedef std::shared_ptr<PolicyInitialisationT> PolicyInitialisationPtrT;

    typedef typename PolicyT::PolicyPtrT PolicyPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;

    /**
     * \param p correctly initialised (but still empty/unlearned) policy.
     * \param _actionGenerator the action generator to use
     * This object will be changed in the course of applying this StateAlgorithm.
     */
    PolicyInitialisation(const PolicyPtrT& p, const ActionGeneratorConstPtrT& _actionGenerator):
        policy(p), actionGenerator(_actionGenerator)
    {
        assert(actionGenerator.get());
        assert(policy.get());
    }
    virtual ~PolicyInitialisation()
    {
    }

    virtual bool apply(const StateT& s)
    {
        // PRINTMSG("Process state "<<s);
        policy->bestAction(s, actionGenerator->randomAction(), 0, 0);
        return true;
    }

    PolicyPtrT getPolicy()
    {
        return policy;
    }

private:
    PolicyInitialisation() {}
    PolicyPtrT policy;
    const ActionGeneratorConstPtrT actionGenerator;
};


/**
 * Implementation of a LearningController for the policy iteration algorithm.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class Domain>
class PolicyIterationController: public LearningController<Domain, float>
{
public:
    typedef Domain DomainT;
    typedef float UtilityDataTypeT;
    typedef LearningController<DomainT, UtilityDataTypeT> LearningControllerT;
    typedef typename DomainT::StateT StateT;
    typedef typename DomainT::ActionT ActionT;

    typedef StateGenerator<StateT> StateGeneratorT;
    typedef ActionGenerator<ActionT> ActionGeneratorT;
    typedef SelectedReward<StateT>  SelectedRewardT;
    typedef Utility<StateT, UtilityDataTypeT> UtilityT;
    typedef Policy<StateT, ActionT> PolicyT;
    typedef LookupPolicy<StateT, ActionT> LookupPolicyT;


    typedef typename UtilityT::UtilityPtrT UtilityPtrT;
    typedef typename UtilityT::UtilityConstPtrT UtilityConstPtrT;
    typedef typename PolicyT::PolicyPtrT PolicyPtrT;
    typedef typename PolicyT::PolicyConstPtrT PolicyConstPtrT;
    typedef typename StateGeneratorT::StateGeneratorPtrT StateGeneratorPtrT;
    typedef typename StateGeneratorT::StateGeneratorConstPtrT StateGeneratorConstPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorPtrT ActionGeneratorPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;
    typedef typename DomainT::DomainConstPtrT DomainConstPtrT;



    explicit PolicyIterationController(DomainConstPtrT _domain, float _defaultUtility,
                                       float _discount, bool _train = true):
        LearningControllerT(_domain, _train),
        policy(new LookupPolicyT()),
        defaultUtility(_defaultUtility),
        discount(_discount), initialised(false) {}
    virtual ~PolicyIterationController() {}

    virtual bool isOnlineLearner()
    {
        return false;
    }

    /**
     */
    virtual PolicyConstPtrT getPolicy()const
    {
        return policy;
    }
    virtual UtilityConstPtrT getUtility()const
    {
        return NULL;
    }

    virtual void resetStartState(const StateT& startState)
    {
    }

    virtual int finishedLearning() const
    {
        return initialised ? 2 : -2;
    }

    virtual void printValues(std::ostream& o) const
    {
        std::stringstream strng;
        strng << "Learned policy: " << std::endl;
        policy->print(strng);
        o << strng.str();
    }

protected:
    typedef MappedUtility<StateT> MappedUtilityT;
    /**
     */
    virtual bool learnOffline(const StateT& currState)
    {
        if (!this->domain.get() ||
                !this->domain->getReward().get() ||
                !this->domain->getStateGenerator().get() ||
                !this->domain->getActionGenerator().get() ||
                !this->domain->getTransition().get())
        {
            PRINTERROR("Can't perform value iteration because one of the required objects is NULL");
            return false;
        }
        UtilityPtrT utility(new MappedUtilityT(defaultUtility));

        PRINTMSG("Start policy iteration..");
        PolicyPtrT resultPolicy = policyIteration(utility, policy,
                                  this->domain->getReward(), this->domain->getTransition(),
                                  this->domain->getStateGenerator(), this->domain->getActionGenerator(), discount);
        if (!resultPolicy.get())
        {
            PRINTERROR("Error in value iteration");
            return 1;
        }
        policy = resultPolicy;
        return true;
    }
    virtual ActionT getBestAction(const StateT& currentState)const
    {
        if (!initialised)
        {
            PRINTERROR("Can't get best action, because learning has not been successful.");
            return ActionT(); // return default action
        }
        ActionT a;
        if (!policy->getAction(currentState, a))
        {
            PRINTERROR("Could not get the best action for the state " << currentState);
            return ActionT(); // return default action
        }
        return a;
    }

    virtual bool initializeImpl(const StateT& startState)
    {
        initialised = true;
        return true;
    }

    PolicyIterationController() {}
private:
    PolicyPtrT policy;
    float defaultUtility;
    float discount;
    bool initialised;
};






/**
  * For description of all parameters except sg see PolicyIterationUpdate constructor parameter
  * description.
  * \author Jennifer Buehler
  * \date May 2011

  * \param u correctly initialised (but still empty/unlearned) model of utility.
  * This object will be changed in the course of applying this StateAlgorithm.
  * \param p correctly initialised (but still empty/unlearned) policy.
  * This object will be changed in the course of applying this StateAlgorithm.
  * \param r reward function to use
  * \param t correctly initialised transition model.
  * \param ag action generator (generates all possible actions) for domaon.
  * \param sg state generator to use
  * \param discount this is used for the policy evaluation
  * \param modPolicyIter for policy evaluation (modified policy iteration). Indicates how many value
  * iteration steps are performed per iteration of the policy iteration algorithm to update the utility.
  */
template<class State, class Action>
std::shared_ptr<Policy<State, Action> > policyIteration(
    std::shared_ptr<Utility<State> > u,
    std::shared_ptr<Policy<State, Action> > p,
    std::shared_ptr<const Reward<State> > r,
    std::shared_ptr<const Transition<State, Action> > t,
    std::shared_ptr<const StateGenerator<State> > sg,
    std::shared_ptr<const ActionGenerator<Action> > ag,
    float discount, unsigned int modPolicyIter = 5)
{

    typedef PolicyIterationUpdate<State, Action> PolicyIterationUpdateT;
    typedef ValueIterationUpdate<State, Action> ValueIterationUpdateT;
    typedef PolicyInitialisation<State, Action> PolicyInitialisationT;
    typedef Utility<State> UtilityT;
    typedef Policy<State, Action> PolicyT;
    typedef Transition<State, Action> TransitionT;
    typedef Reward<State> RewardT;
    typedef StateGenerator<State> StateGeneratorT;
    typedef ActionGenerator<Action> ActionGeneratorT;


    typedef typename UtilityT::UtilityPtrT UtilityPtrT;
    typedef typename UtilityT::UtilityConstPtrT UtilityConstPtrT;
    typedef typename PolicyT::PolicyPtrT PolicyPtrT;
    typedef typename PolicyT::PolicyConstPtrT PolicyConstPtrT;
    typedef typename TransitionT::TransitionPtrT TransitionPtrT;
    typedef typename TransitionT::TransitionConstPtrT TransitionConstPtrT;
    typedef typename RewardT::RewardConstPtrT RewardConstPtrT;
    typedef typename StateGeneratorT::StateGeneratorPtrT StateGeneratorPtrT;
    typedef typename StateGeneratorT::StateGeneratorConstPtrT StateGeneratorConstPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorPtrT ActionGeneratorPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;
    typedef typename PolicyIterationUpdateT::PolicyIterationUpdatePtrT PolicyIterationUpdatePtrT;
    typedef typename ValueIterationUpdateT::ValueIterationUpdatePtrT ValueIterationUpdatePtrT;
    typedef typename PolicyInitialisationT::PolicyInitialisationPtrT PolicyInitialisationPtrT;

    unsigned int cnt = 0;

    UtilityPtrT utility(u);
    PolicyPtrT policy(p);
    const TransitionConstPtrT transition(t);
    const RewardConstPtrT reward(r);
    const StateGeneratorConstPtrT stateGen(sg);
    const ActionGeneratorConstPtrT actionGen(ag);


    PolicyIterationUpdatePtrT policyIterationUpdate(
        new PolicyIterationUpdateT(utility, transition, policy, actionGen));
    bool unchanged = true;

    ValueIterationUpdatePtrT valueIterationUpdate(
        new ValueIterationUpdateT(utility, reward, transition, NULL, policy, discount, 0));

    PolicyInitialisationPtrT policyInit(
        new PolicyInitialisationT(policy, actionGen));

    if (!stateGen->foreachState(*(policyInit.get())))
    {
        PRINTERROR("Could not initialise policy");
        return NULL;
    }

    do
    {
        // PRINTMSG("iteration "<<cnt);
        // policy evaluation:
        for (unsigned int k = 0; k < modPolicyIter; ++k)
        {
            valueIterationUpdate->preApplication();
            if (!stateGen->foreachState(*(valueIterationUpdate.get())))
            {
                PRINTERROR("Could not apply value iteration to all states");
                return NULL;
            }
            valueIterationUpdate->postApplication();
        }

        utility = valueIterationUpdate->getUtility();

        /*std::stringstream strng;
        utility->print(strng);
        PRINTMSG("Iteration  "<<cnt<<": utility = ");
        PRINTMSG(strng.str());*/

        /*std::stringstream strng;
        policy->print(strng);
        PRINTMSG("Iteration  "<<cnt<<": policy = ");
        PRINTMSG(strng.str());*/

        policyIterationUpdate->setUtility(utility);

        // policy iteration:
        policyIterationUpdate->preApplication();
        if (!stateGen->foreachState(*(policyIterationUpdate.get())))
        {
            PRINTERROR("Could not apply value iteration to all states");
            return NULL;
        }
        unchanged = policyIterationUpdate->isUnchanged();
        ++cnt;
        // if (cnt==19) {PRINTMSG("WARN: Break here"); break;}
    }
    while (!unchanged);

    PRINTMSG("Number of iterations: " << cnt);
    return policyIterationUpdate->getPolicy();
}

}

#endif
