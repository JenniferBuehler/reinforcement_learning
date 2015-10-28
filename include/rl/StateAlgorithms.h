#ifndef __STATE_ALGORITHMS__H__
#define __STATE_ALGORITHMS__H__

#include <memory>

#include <rl/State.h>
#include <rl/Action.h>

namespace rl{

/**
 * \brief An algorithm to operate on one State object.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State>
class StateAlgorithm {
public:
	typedef State StateT;
	typedef std::shared_ptr<State> StatePtrT;
	typedef std::shared_ptr<const State> StateConstPtrT;

	StateAlgorithm(){}
	virtual ~StateAlgorithm(){}

	virtual bool apply(const StateT& s)=0;
};

/**
 * \brief An algorithm to operate on one Action object.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class Action>
class ActionAlgorithm {
public:
	typedef Action ActionT;
	typedef ActionAlgorithm<ActionT> ActionAlgorithmT;
	typedef std::shared_ptr<ActionAlgorithmT> ActionAlgorithmPtrT;
	typedef std::shared_ptr<const ActionAlgorithmT> ActionAlgorithmConstPtrT;

	ActionAlgorithm(){}
	virtual ~ActionAlgorithm(){}

	virtual bool apply(const ActionT& a)=0;
};



/**
 * \brief Iterates through all possible states by generating each
 * possible state (or a random set of states) and applying
 * a StateAlgorithm to each state. Knowledge of underlying domain
 * (e.g. grid world) is to be used to generate states.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State>
class StateGenerator {
public:
	typedef State StateT;
	typedef StateGenerator<StateT> StateGeneratorT;
	typedef std::shared_ptr<StateGeneratorT> StateGeneratorPtrT;
	typedef std::shared_ptr<const StateGeneratorT> StateGeneratorConstPtrT;

	typedef StateAlgorithm<StateT> StateAlgorithmT;

	StateGenerator(){}
	virtual ~StateGenerator(){}

	/**
	 * \return false if errors occurred and not all states could be iterated through
	 */
	virtual bool foreachState(StateAlgorithmT& s) const=0;
	/**
	 * Generates a random state
	 */
	virtual State randomState()const=0;

};

/**
 * \brief Iterates through all possible action by generating each
 * possible action and applying an ActionAlgorithm to each one. 
 * Knowledge of underlying domain (e.g. grid world actions up,down...) 
 * is to be used to generate actions.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class Action>
class ActionGenerator {
public:
	typedef Action ActionT;
	typedef ActionGenerator<ActionT> ActionGeneratorT;
	typedef std::shared_ptr<ActionGeneratorT> ActionGeneratorPtrT;
	typedef std::shared_ptr<const ActionGeneratorT> ActionGeneratorConstPtrT;

	typedef ActionAlgorithm<ActionT> ActionAlgorithmT;

	ActionGenerator(){}
	virtual ~ActionGenerator(){}

	/**
	 * \return false if errors occurred and not all actions could be iterated through
	 */
	virtual bool foreachAction(ActionAlgorithmT& s)const=0;

	/**
	 * Generates a random action
	 */
	virtual Action randomAction()const=0;
};


}

#endif
