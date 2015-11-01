#ifndef __VALUEITERATION_H__
#define __VALUEITERATION_H__
// Copyright Jennifer Buehler

#include <rl/MaxUtility.h>
#include <rl/State.h>
#include <rl/Action.h>
#include <rl/StateAlgorithms.h>
#include <rl/Utility.h>
#include <rl/Reward.h>
#include <rl/Transition.h>
#include <rl/Policy.h>
#include <rl/LogBinding.h>
#include <rl/Controller.h>

#include <math/FloatComparison.h>

#include <assert.h>
#include <math.h>

#define ZERO_EPSILON 1e-07

namespace rl
{

/**
 * \brief Generates a policy out of a utility and transition function
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action, typename UtilityDatatype>
class PolicyGenerationAlgorithm: public StateAlgorithm<State>
{
public:
    typedef State StateT;
    typedef Action ActionT;
    typedef UtilityDatatype UtilityDatatypeT;
    typedef PolicyGenerationAlgorithm<StateT, ActionT, UtilityDatatypeT> PolicyGenerationAlgorithmT;
    typedef std::shared_ptr<PolicyGenerationAlgorithmT> PolicyGenerationAlgorithmPtrT
    ;
    typedef Utility<StateT, UtilityDatatypeT> UtilityT;
    typedef ActionGenerator<ActionT> ActionGeneratorT;
    typedef Transition<StateT, ActionT> TransitionT;
    typedef Policy<StateT, ActionT> PolicyT;
    typedef LookupPolicy<StateT, ActionT> LookupPolicyT;
    typedef MaxUtilityActionAlgorithm<StateT, ActionT> MaxUtilityActionAlgorithmT;

    typedef typename TransitionT::TransitionConstPtrT TransitionConstPtrT;
    typedef typename UtilityT::UtilityConstPtrT UtilityConstPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;
    typedef typename PolicyT::PolicyPtrT PolicyPtrT;
    typedef typename PolicyT::PolicyConstPtrT PolicyConstPtrT;

    PolicyGenerationAlgorithm(const TransitionConstPtrT& t, const UtilityConstPtrT& u, const ActionGeneratorConstPtrT& a):
        resultPolicy(new LookupPolicyT()), trans(t), utility(u), aGen(a) {}
    virtual ~PolicyGenerationAlgorithm() {}

    virtual bool apply(const StateT& s)
    {
        //choose the action which leads to be state with the best utility:
        MaxUtilityActionAlgorithmT maxUt(*utility, *trans, s);
        if (!aGen->foreachAction(maxUt))
        {
            PRINTERROR("Could not apply all actions");
            return false;
        }
        else
        {
            resultPolicy->bestAction(s, maxUt.getBestAction());
        }
        return true;
    }

    PolicyPtrT getPolicy()
    {
        return resultPolicy;
    }
private:
    PolicyPtrT resultPolicy;
    TransitionConstPtrT trans;
    UtilityConstPtrT utility;
    ActionGeneratorConstPtrT aGen;
};





/**
 * Performs the update of one iteration within the loop of the value iteration
 * algorithm, by applying each state to this update.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
class ValueIterationUpdate: public StateAlgorithm<State>
{
public:
    typedef float FloatT;
    typedef State StateT;
    typedef Action ActionT;

    typedef ValueIterationUpdate<StateT, ActionT> ValueIterationUpdateT;
    typedef std::shared_ptr<ValueIterationUpdateT> ValueIterationUpdatePtrT;

    typedef Utility<StateT, FloatT> UtilityT;
    typedef Reward<StateT, FloatT> RewardT;
    typedef Transition<StateT, ActionT> TransitionT;
    typedef typename TransitionT::StateTransitionListT StateTransitionListT;
    typedef ActionGenerator<ActionT> ActionGeneratorT;
    typedef MaxUtilityActionAlgorithm<StateT, ActionT> MaxUtilityActionAlgorithmT;
    typedef Policy<StateT, ActionT> PolicyT;

    typedef typename UtilityT::UtilityPtrT UtilityPtrT;
    typedef typename RewardT::RewardPtrT RewardPtrT;
    typedef typename RewardT::RewardConstPtrT RewardConstPtrT;
    typedef typename TransitionT::TransitionConstPtrT TransitionConstPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;
    typedef typename PolicyT::PolicyPtrT PolicyPtrT;
    typedef typename PolicyT::PolicyConstPtrT PolicyConstPtrT;


    /**
     * \param u correctly initialised (but still empty/unlearned) model of utility.
     * \param r correctly initialised model of reward.
     * \param t correctly initialised transition model.
     * \param ag action generator (generates all possible actions) for domain. If this object is NULL, the policy is
     * fixed, and parameter policy MUST be non-NULL! Responsibility of ojbect is passed.
     * \param p policy for domain. If this object is NULL, the policy is NOT
     * fixed, and parameter ag (ActionGenerator) MUST be non-NULL!
     * \param _discount discount factor
     * \param _delta starting value for maximum change in the utility of any state. If utility change for a state exceeds
     * this value, it is updated to the new utility change. The changed delta value after applying all states can
     * be retrieved with getNewDelta().
     */
    ValueIterationUpdate(UtilityPtrT& u, const RewardConstPtrT& r, const TransitionConstPtrT& t,
                         const ActionGeneratorConstPtrT& ag,
                         const PolicyConstPtrT& p, float _discount, float _delta):
        utility(u), tempUtility(u->clone()), reward(r), transition(t), actionGenerator(ag), policy(p), discount(_discount), delta(_delta)
    {

        assert(utility.get());
        assert(tempUtility.get());
        assert(reward.get());
        assert(transition.get());
        assert(actionGenerator.get() || policy.get());
    }
    virtual ~ValueIterationUpdate()
    {
    }
    /**
     * Has to be called before applying all states (see method apply(StateT&) )
     */
    void preApplication()
    {
        delta = 0;
    }

    /**
     * Has to be called after applying all states (see method apply(StateT&) )
     */
    void postApplication()
    {
        //replace utility by copy of newer tempUtility
        utility = UtilityPtrT(tempUtility->clone());
        //and update tempUtility to the newest utility function
        tempUtility = UtilityPtrT(utility->clone());

        /*std::stringstream strng;
        utility->print(strng);
        PRINTMSG(strng.str());*/
    }
    virtual bool apply(const StateT& s)
    {
        //PRINTMSG("Process state "<<s);
        MaxUtilityActionAlgorithmT maxUt(*utility, *transition, s);
        if (actionGenerator.get())
        {
            if (!actionGenerator->foreachAction(maxUt))
            {
                PRINTERROR("Could not apply summation on all actions");
                return false;
            }
        }
        else
        {
            ActionT a;
            if (!policy->getAction(s, a))
            {
                PRINTERROR("No policy assigned for a state. Make sure the policy spits out at least a random action!");
                return false;
            }
            maxUt.apply(a);
        }
        float mean, variance; //to be ignored here
        FloatT oldUt = utility->getUtility(s, mean, variance);
        FloatT ut = reward->getReward(s) + discount * maxUt.getValue(); //instantaneous reward plus discounted utility over following states

        //PRINTMSG("State "<<s<<": Found utility "<<ut);
        tempUtility->experienceUtility(s, ut); //update utility

        if (policy.get()) return true; //the rest of the operations are not needed for a fixed policy

        FloatT newUt = tempUtility->getUtility(s, mean, variance); //a new lookup has to be done, as we don't know how utility values are updated.

        FloatT utChange = fabs(newUt - oldUt);
        if ((utChange > delta) && !equalFloats(utChange, delta, static_cast<float>(ZERO_EPSILON)))
            delta = utChange;
        return true;
    }
    float getDelta()
    {
        return delta;
    }

    UtilityPtrT getUtility()
    {
        if (!utility.get()) throw Exception("Utility assigned was NULL", __FILE__, __LINE__);
        return utility->clone();
    }
protected:


private:
    ValueIterationUpdate() {}
    UtilityPtrT utility;
    UtilityPtrT tempUtility; //temporary copy of utility to apply all states to
    RewardConstPtrT reward;
    TransitionConstPtrT transition;
    ActionGeneratorConstPtrT actionGenerator;
    PolicyConstPtrT policy;
    float discount;
    float delta;
};



/**
 * LearningController implementation for the value iteration algorithm.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class Domain>
class ValueIterationController: public LearningController<Domain, float>
{
public:
    typedef Domain DomainT;
    typedef typename DomainT::DomainPtrT DomainPtrT;
    typedef typename DomainT::DomainConstPtrT DomainConstPtrT;

    typedef float UtilityDataTypeT;
    typedef typename DomainT::StateT StateT;
    typedef typename DomainT::ActionT ActionT;

    typedef LearningController<DomainT, UtilityDataTypeT> LearningControllerT;

    typedef typename LearningControllerT::UtilityT UtilityT;
    typedef typename LearningControllerT::PolicyT PolicyT;

    typedef StateGenerator<StateT> StateGeneratorT;
    typedef ActionGenerator<ActionT> ActionGeneratorT;
    typedef SelectedReward<StateT>  SelectedRewardT;
    typedef Transition<StateT, ActionT> TransitionT;


    typedef typename UtilityT::UtilityPtrT UtilityPtrT;
    typedef typename UtilityT::UtilityConstPtrT UtilityConstPtrT;
    typedef typename TransitionT::TransitionConstPtrT TransitionConstPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;
    typedef typename PolicyT::PolicyPtrT PolicyPtrT;
    typedef typename PolicyT::PolicyConstPtrT PolicyConstPtrT;

    typedef typename StateGeneratorT::StateGeneratorConstPtrT StateGeneratorConstPtrT;


    typedef MaxUtilityActionAlgorithm<StateT, ActionT> MaxUtilityActionAlgorithmT;

    typedef class PolicyGenerationAlgorithm<StateT, ActionT, UtilityDataTypeT> PolicyGenerationAlgorithmT;
    typedef typename PolicyGenerationAlgorithmT::PolicyGenerationAlgorithmPtrT PolicyGenerationAlgorithmPtrT;

    explicit ValueIterationController(DomainConstPtrT _domain, float _defaultUtility,
                                      float _discount, float _maxErr, bool _train = true):
        LearningControllerT(_domain, _train),
        utility(UtilityT::makePtr(new MappedUtilityT(_defaultUtility))),
        discount(_discount), maxErr(_maxErr), initialised(false)
    {
    }
    virtual ~ValueIterationController() {}

    virtual bool isOnlineLearner()
    {
        return false;
    }
    virtual PolicyConstPtrT getPolicy()const
    {
        if (!initialised)
        {
            PRINTERROR("Can't get policy, because learning has not been successful.");
            return NULL;
        }
        TransitionConstPtrT trans = this->domain->getTransition(); //XXX this can be optimised to keep globally
        if (!trans.get())
        {
            PRINTERROR("No transition function available");
            return NULL;
        }
        StateGeneratorConstPtrT stateGenerator = this->domain->getStateGenerator();
        if (!stateGenerator.get())
        {
            PRINTERROR("No state generator available");
            return NULL;
        }
        ActionGeneratorConstPtrT actionGenerator = this->domain->getActionGenerator();
        if (!actionGenerator.get())
        {
            PRINTERROR("No action generator available");
            return NULL;
        }
        PolicyGenerationAlgorithmPtrT pg(new PolicyGenerationAlgorithmT(trans, utility, actionGenerator));
        stateGenerator->foreachState(*pg);

        return pg->getPolicy();
    }
    virtual UtilityConstPtrT getUtility()const
    {
        return utility;
    }

    virtual void resetStartState(const StateT& startState)
    {
    }

    virtual int finishedLearning()const
    {
        return initialised ? 2 : -2;
    }

    virtual void printValues(std::ostream& o) const
    {
        std::stringstream strng;
        //strng<<"Utility function: "<<std::endl;
        //utility->print(strng);
        PolicyConstPtrT policy = getPolicy();
        if (policy.get())
        {
            strng << "Policy: " << std::endl;
            policy->print(strng);
        }
        o << strng.str();
    }

protected:
    typedef MappedUtility<StateT> MappedUtilityT;
    /**
     */
    virtual bool learnOffline(const StateT& currState)
    {
        PRINTMSG("Starting offline learning of value iteration");
        if (!this->domain.get() ||
                !this->domain->getReward().get() ||
                !this->domain->getStateGenerator().get() ||
                !this->domain->getActionGenerator().get() ||
                !this->domain->getTransition().get())
        {
            PRINTERROR("Can't perform value iteration because one of the required objects is NULL");
            return false;
        }

        UtilityPtrT newUt = valueIteration(utility, this->domain->getReward(), this->domain->getTransition(),
                                           this->domain->getActionGenerator(), this->domain->getStateGenerator(), discount, maxErr);

        if (!newUt.get())
        {
            PRINTERROR("Error in value iteration");
            return 1;
        }
        utility = newUt;
        return true;
    }

    virtual ActionT getBestAction(const StateT& currentState)const
    {
        if (!initialised)
        {
            PRINTERROR("Can't get best action, because learning has not been successful.");
            return ActionT(); //return default action
        }
        TransitionConstPtrT trans = this->domain->getTransition(); //XXX this can be optimised to keep globally

        //choose the action which leads to be state with the best utility:
        MaxUtilityActionAlgorithmT maxUt(*utility, *trans, currentState);
        ActionGeneratorConstPtrT actionGenerator = this->domain->getActionGenerator();
        if (!actionGenerator->foreachAction(maxUt))
        {
            PRINTERROR("Could not apply summation on all actions");
            return ActionT(); //return default action
        }
        return maxUt.getBestAction();
    }

    virtual bool initializeImpl(const StateT& startState)
    {
        initialised = true;
        PRINTMSG("Value iteration initialized");
        return true;
    }

    ValueIterationController() {}
private:
    UtilityPtrT utility;
    float discount;
    float maxErr;
    bool initialised;
};







/**
 * This function applies the value iteration algorithm, given a utility function, a reward function, a
 * transition function and implementations of ActionGenerator and StateGenerator.
 * For description of all parameters except sg and maxErr see ValueIterationUpdate constructor parameter.
 * \param u correctly initialised (but still empty/unlearned) model of utility.
 * \param r correctly initialised model of reward.
 * \param t correctly initialised transition model.
 * \param ag action generator (generates all possible actions) for domain. If this object is NULL, the policy is
 * fixed, and parameter policy MUST be non-NULL!
 * \param sg state generator to use.
 * \param discount discount factor
 * \param maxErr maximum error allowed in the utility of any state (determines termination criterion).
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, class Action>
std::shared_ptr<Utility<State> > valueIteration(
    std::shared_ptr<Utility<State> > u,
    const std::shared_ptr<const Reward<State> > r,
    const std::shared_ptr<const Transition<State, Action> > t,
    const std::shared_ptr<const ActionGenerator<Action> > ag,
    const std::shared_ptr<const StateGenerator<State> > sg,
    float discount, float maxErr)
{
    /*template<class State, class Action>
    std::shared_ptr<Utility<State> > valueIteration(
        Utility<State> * u,
        const Reward<State> * r,
        const Transition<State,Action> * t,
        const ActionGenerator<Action> * ag,
        const StateGenerator<State> * sg,
        float discount, float maxErr){
    */

    typedef ValueIterationUpdate<State, Action> ValueIterationUpdateT;
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
    typedef typename RewardT::RewardConstPtrT RewardConstPtrT;
    typedef typename TransitionT::TransitionPtrT TransitionPtrT;
    typedef typename TransitionT::TransitionConstPtrT TransitionConstPtrT;
    typedef typename StateGeneratorT::StateGeneratorPtrT StateGeneratorPtrT;
    typedef typename StateGeneratorT::StateGeneratorConstPtrT StateGeneratorConstPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorPtrT ActionGeneratorPtrT;
    typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;

    UtilityPtrT utility(u);
    const TransitionConstPtrT transition(t);
    const RewardConstPtrT reward(r);
    const StateGeneratorConstPtrT stateGen(sg);
    const ActionGeneratorConstPtrT actionGen(ag);
    const PolicyConstPtrT nullPolicy(NULL); //policy not used, but need to pass NULL value to ValueIterationUpdate



    float delta = 0;
    float discountRatio = static_cast<float>(1.0 - discount) / static_cast<float>(discount);
    float minDelta = maxErr * discountRatio;
    PRINTMSG("Starting value iteration with discount=" << discount << ", discountRatio=" 
        << discountRatio << ", maxErr=" << maxErr << ", minDelta=" << minDelta);
    unsigned int cnt = 0;
    ValueIterationUpdate<State, Action> valueIterationUpdate(utility, reward, transition,
                                                             actionGen, nullPolicy, discount, delta);
    do
    {
        valueIterationUpdate.preApplication();
        if (!sg->foreachState(valueIterationUpdate))
        {
            PRINTERROR("Could not apply value iteration to all states");
            return NULL;
        }
        valueIterationUpdate.postApplication();
        delta = valueIterationUpdate.getDelta();
        PRINTMSG("Finished iteration, delta=" << delta << ", iteration number=" << cnt);
        ++cnt;
        //if (cnt==19) {PRINTMSG("WARN: Break here"); break;}
    }
    while ((delta > minDelta) || ((delta > minDelta) && (!equalFloats(delta, minDelta, static_cast<float>(ZERO_EPSILON)))));

    PRINTMSG("Number of iterations: " << cnt);
    return valueIterationUpdate.getUtility();
}

}
#endif
