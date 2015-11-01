#ifndef RL_CONTROLLER_H
#define RL_CONTROLLER_H
// Copyright Jennifer Buehler

#include <rl/Policy.h>
#include <rl/Transition.h>
#include <rl/Utility.h>

namespace rl
{

/**
 * \brief Interface to learning algorithms.
 * After setting the initial state of the environment for the learner,
 * it can be used to update the state each time step.
 * Internally, each state will lead to a recommended action by the learner,
 * which in turn can be used to transfer the system into a new state.
 * This will have to be done outside this class, e.g. in a subclass of
 * Domain, which in turn uses the transition function to transfer between states.
 *
 * Basically, there is two general differences, depending on the implementation
 * of the subclass:
 *
 * 1. ONLINE LEARNING
 * Each time the controller updated (by method updateAndGetAction()) a learning
 * step is performed. Based on the information known until this time step,
 * (inferred from the updated learning parameters in the learning step)
 * the best action is chosen to be performed (this will mostly not be the
 * actual best action in the beginning of execution).
 * An example is Q-learning, which will update the q-table each step based on an
 * internally used reward function.
 *
 * 2. OFFLINE LEARNING
 * At initialisation time (call of method initialize()), the learning is performed
 * based on the information available (i.e. internally used model of environment, reward
 * function, utility funcitons, which are all objects which have to be returned by Domain
 * implementation). Then, each time updateAndGetAction() is called,
 * the best action is chosen based on the pre-learned policy (or utility table).
 * Examples are value and policy iteration, which will learn the utility function or
 * policy given a transition table and a reward function.
 *
 * These two cases can be handeled generically for a simulation of the environment
 * by using an algorithm similar as described in the following simulation code.
 *
 * Action a;
 * State s=initial_state;
 * controller.initialize();
 * while (simulator_running){
 *  a=controller.getAction(s);
    s=... transfer in new state by executing action a
 *  .. do stuff with s, e.g. set visualisation parameters ...
 * }
 *
 * The method isOnlineLearner() can be used to determine whether this algorithm
 * does some learning. This can be important for determining whether to do a quick
 * simulation without visualisation, or a to run a learned policy. The method
 *
 * \author Jennifer Buehler
 * \date May 2011
 *
 * \param Domain the domain class (the directly used class, NOT the base class!)
 * \param StateUtilityDatatype the data type used to express the utility of a state. Default: float
 */
template<class Domain, typename StateUtilityDatatype = float>
class LearningController
{
public:
    typedef Domain DomainT;
    typedef typename DomainT::DomainPtrT DomainPtrT;
    typedef typename DomainT::DomainConstPtrT DomainConstPtrT;

    typedef typename DomainT::StateT StateT;
    typedef typename DomainT::ActionT ActionT;

    typedef Policy<StateT, ActionT> PolicyT;
    typedef StateUtilityDatatype StateUtilityT;
    typedef Utility<StateT, StateUtilityT> UtilityT;

    typedef LearningController<DomainT, StateUtilityT> LearningControllerT;
    typedef std::shared_ptr<LearningControllerT> LearningControllerPtrT;

    typedef typename PolicyT::PolicyPtrT PolicyPtrT;
    typedef typename PolicyT::PolicyConstPtrT PolicyConstPtrT;
    typedef typename UtilityT::UtilityPtrT UtilityPtrT;
    typedef typename UtilityT::UtilityConstPtrT UtilityConstPtrT;


    /**
     * \param _domain the domain to be learned
     * \param _train initial value for training (set setTraining())
     */
    explicit LearningController(DomainConstPtrT _domain, bool _train = true): domain(_domain), train(_train) {}
    LearningController(const LearningController& o): domain(o.domain), train(o.train) {}
    virtual ~LearningController() {}

    /**
     * returns the best action using the policy learned so far. For offline learnes,
     * this will be the value from the policy learned at inisialisation state. For
     * online learners, this will be the best action depending on the current learning
     * stage.
     * IMPORTANT: Online learners may have to implement this function, as by default,
     * it will return getBestAction()!
     */
    virtual ActionT getBestLearnedAction(const StateT& currentState) const
    {
        return getBestAction(currentState);
    }

    ActionT updateAndGetAction(const StateT& currState)
    {
        // PRINTMSG("............. state "<<currState);
        if (train)
        {
            if (!learnOnline(currState))
            {
                PRINTERROR("Could not update the learning process!");
            }
            return getBestAction(currState);
        }
        return getBestLearnedAction(currState);
    }

    bool initialize(const StateT& startState)
    {
        // PRINTMSG("Initialisation of learning controller");
        if (!initializeImpl(startState))
        {
            PRINTERROR("Could not successfully initialize learner");
            return false;
        }
        if (!train)
        {
            // PRINTMSG("No offline learning to be performed");
            return true;
        }
        return learnOffline(startState);
    }

    /**
     * This method can be used to set the start state to follow with the optimal
     * policy. This means that no connection with the previous state passed to updateAndGetBestAction()
     * is assumed any more, which may be important for some online learning algorithms.
     * (therefore, this method is pure virtual, to make sure subclasses consider the case that
     * the state may be reset).
     */
    virtual void resetStartState(const StateT& startState) = 0;

    /**
     * If this method returns true, we will only have to call
     * initialize() in order to learn the utility function.
     * After it has been initialised, the function transferState() can be used
     * to transfer the state of the domain and thus use the learned policy
     * to move around in the world.
     */
    virtual bool isOnlineLearner() = 0;


    /**
     * Return the learned policy
     */
    virtual PolicyConstPtrT getPolicy()const = 0;

    /**
     * Return the learned utility function
     */
    virtual UtilityConstPtrT getUtility()const = 0;

    /**
     * \retval -2 the learning can never be finished because the system was not initialized
     * \retval -1 the learning process has not converged yet
     * \retval 0 it is not known whether the learning has converged yet. This can
     * only be determined by checking back with the domain and evaluating there whether
     * the learning has finished (for example, in Q-Learning). This function will therefore
     * always return 0 for this Controller implementation.
     * \retval 1 the learning has converged.
     */
    virtual int finishedLearning() const = 0;

    /**
     * This will print the relevant values for the learning algorithm, e.g. the learned utility,
     * policy, or q-table. This will vary between implementations.
     */
    virtual void printValues(std::ostream& o) const = 0;
    /**
     * Prints some statistics, as learning progress or sizes of tables
     */
    virtual void printStats(std::ostream& o) const
    {
        o << "No stats implementation";
    }

    /**
     * Set training on or off. If set to off (parameter false),
     * the learning stops and the so far explored policy will be used
     * to determine action returned by updateAndGetAction().
     */
    void setTraining(bool on)
    {
        train = on;
    }
protected:
    /**
     * returns the best action to perform given the current state, at the current stage
     * of the learning process. For online learners, this will be the action
     * currently recommended. For offline learners, this will be the optimal action
     * from the policy learned at initialisation (learnOffline()).
     */
    virtual ActionT getBestAction(const StateT& currentState)const = 0;


    /**
     * updates the online learning based on the current state. After performing
     * such an update, the best action recommended at the current stage of learning
     * has to be returned with method getBestAction().
     * If the learner is an offline learner, this method only returns true.
     */
    virtual bool learnOnline(const StateT& currState)
    {
        return true;
    }
    /**
     * If the implementing algorithm is an online method, this function should return true
     * and do nothing.
     */
    virtual bool learnOffline(const StateT& currState)
    {
        // PRINTMSG("No offline learning required");
        return true;
    }

    virtual bool initializeImpl(const StateT& startState) = 0;

protected:
    LearningController() {}
    DomainConstPtrT domain;
    bool train;
};

}  // namespace rl

#endif  // RL_CONTROLLER_H
