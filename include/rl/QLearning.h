#ifndef __QLEARNING_H__
#define __QLEARNING_H__

#include <rl/Transition.h>
#include <rl/StateActionPair.h>
#include <rl/StateAlgorithms.h>
#include <rl/Exploration.h>
#include <rl/Policy.h>

#include <math/RandomNumber.h>
#include <general/Exception.h>

#include <iostream>
#include <limits>
#include <deque>
#include <set>

//if defined, during q-learning the transition
//function is learned
//#define LEARN_TRANSITION

//#define UPDATE_WITH_OLD_REWARD

//keep the average q-value changes for the last X steps
//(number defined here). If macro undefined, no averages
//are kept.
#define KEEP_AVG_CHANGE 10000

namespace rl{


/**
 * \brief A pair of action and value, which can be used as a key.
 * The key will be the Action, therefore this datatype has
 * to support the < operator.
 * The value is simply associated with the action and plays
 * no role for the key.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class Action, typename Value>
class ActionValuePair{
public:
	typedef Action ActionT;
	typedef Value ValueT;

	ActionValuePair(){}
	ActionValuePair(const ActionT& _a, const ValueT& _v): a(_a),v(_v){}
	ActionValuePair(const ActionValuePair& o): a(o.a),v(o.v){}

	friend std::ostream& operator<<(std::ostream& o, const ActionValuePair& p){
		o<<"Action="<<p.a<<", value="<<p.v;
		return o;
	}
	ActionValuePair& operator=(const ActionValuePair& o){
		if (this==&o) return *this;
		a=o.a;
		v=o.v;
		return *this;
	}

	bool operator < (const ActionValuePair& p) const{
		return (a<p.a);
	}
	ActionT a;
	ValueT v;
private:
};



/**
 * Implementation of a LearningController for the q learning algorithm.
 *
 * State and Action template parameters have to be usable as a key in a map (i.e.
 * support the < operator). They should both also implement the = operator and
 * copy constructor.
 * 
 * \author Jennifer Buehler
 * \date May 2011
 * \param Domain must be the class type of the domain used (NOT the base domain class!)
 * \param UtilityType utility value to use for the q-table entries
 */
template<class Domain,typename UtilityType=float>
class QLearningController: public LearningController<Domain,UtilityType> {
public:
	typedef Domain DomainT;
	typedef typename DomainT::StateT StateT;
	typedef typename DomainT::ActionT ActionT;
	typedef typename DomainT::RewardValueTypeT RewardValueTypeT;
	typedef typename DomainT::DomainConstPtrT DomainConstPtrT;

	typedef UtilityType UtilityDataTypeT;	
	typedef LearningController<DomainT,UtilityDataTypeT> LearningControllerT;

	typedef QLearningController<DomainT,UtilityDataTypeT> QLearningControllerT;
	typedef ActionGenerator<ActionT> ActionGeneratorT;
	typedef unsigned int FreqCntT; //datatype to count the frequency of events 
	typedef Exploration<UtilityDataTypeT,FreqCntT> ExplorationT;
	typedef Policy<StateT, ActionT> PolicyT;
	typedef Utility<StateT,UtilityDataTypeT> UtilityT;

	typedef LearnableTransitionMap<StateT,ActionT> LearnableTransitionMapT;

	typedef typename ActionGeneratorT::ActionGeneratorPtrT ActionGeneratorPtrT;
	typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;
	typedef typename PolicyT::PolicyConstPtrT PolicyConstPtrT;
	typedef typename PolicyT::PolicyPtrT PolicyPtrT;
	typedef typename UtilityT::UtilityConstPtrT UtilityConstPtrT;
	typedef typename ExplorationT::ExplorationConstPtrT ExplorationConstPtrT;
	typedef typename LearningRate::LearningRatePtrT LearningRatePtrT;
	typedef typename std::shared_ptr<StateT> StatePtrT;
	typedef typename std::shared_ptr<const StateT> StateConstPtrT;

	/**
	 * \param _defaultQ default q value for state-action pairs which haven't been encountered so far
	 * \param _exploration exploration function to be used
	 * \param _domain the domain to be used
	 * \param _learnRate the learn rate to use
	 * \param _discount discount factor
	 * \param _train initial value for training (set setTraining())
	 * \param _epsilonGreedy value 0..1 indicating a probability that not best, but a random
	 * action is chosen. This adds to the exploration function. It is NOT
	 * implemented as Exploration interface because it does not matter how often
	 * an action has been tried before. ALWAYS a not optimal action will be chosen,
	 * i.e. there will constantly be exploration.
	 */
	QLearningController(DomainConstPtrT _domain, const LearningRatePtrT& _learnRate, const float _discount, const UtilityDataTypeT& _defaultQ, 
		ExplorationConstPtrT _exploration, float _epsilonGreedy, bool _train=true): 
		LearningControllerT(_domain,_train),
		lastState(NULL),learnRate(_learnRate), discount(_discount),
		defaultQ(_defaultQ), actionGenerator(this->domain->getActionGenerator()), exploration(_exploration),epsilonGreedy(_epsilonGreedy),
		policy(new LookupPolicyT()),
		initialised(false)
#ifdef LEARN_TRANSITION
		,learnedTransition(new LearnableTransitionMapT())
#endif
	{
		if (discount>=1.0f) discount=1.0f-std::numeric_limits<float>::epsilon();
		if (discount<0.0f) discount=0.0f;
	} 


	virtual bool isOnlineLearner(){
		return true;
	}

	virtual ActionT getBestLearnedAction(const StateT& currentState) const{
		QMap_const_iterator qit=q.find(currentState); //get the q-entry for the state
		if (qit==q.end()){
			PRINTMSG("WARNING: There is no action learned for state "<<currentState<<". Choosing random action.");
			return actionGenerator->randomAction();
		}
		ActionValuePairT bestActionForState=getMaxQValue(qit->second);
		return bestActionForState.a;
	}


	virtual PolicyConstPtrT getPolicy()const{
		return getLearnedPolicy();
	}
	virtual UtilityConstPtrT getUtility()const{
		return NULL;
	}

	virtual void resetStartState(const StateT& startState){
		lastState=NULL;
	}

	virtual short finishedLearning()const{
		return 0;
	}

	virtual void printValues(std::ostream& o) const{
#ifdef LEARN_TRANSITION
		o<<"## Learned transition: "<<std::endl;
		learnedTransition->print(o);
#endif
		std::stringstream strng;
		strng << "## Trials: " << std::endl;
		NSA_const_iterator nit;
		for (nit=nsaFreq.begin(); nit!=nsaFreq.end(); ++nit){
			strng<<nit->first<<" : "<<nit->second<<std::endl;
		} 

		strng << "## Q-Table: " << std::endl;
		printQValues(strng);

		strng << std::endl << "## Current policy:" << std::endl;
		PolicyPtrT policy=getLearnedPolicy();
		policy->print(strng);
		o << strng.str() << std::endl;
	}


	/**	
	 * Returns the learned policy from applying the q-learning algorithm
	 */
	PolicyPtrT getLearnedPolicy() const{
		PolicyPtrT retPolicy(new LookupPolicyT());
		QMap_const_iterator it;
		//for each state entry in the q-table, find the best associated action
		for (it=q.begin(); it!=q.end(); ++it){
			const ActionValueSetT& setRef=it->second; //keep a reference for better code readability
			if (setRef.empty()){ //the set should NOT be empty!
				PRINTERROR("No actions were assigned it state "<<it->first<<". This is an inconsistency.");
				continue;
			}
			ActionValuePairT bestActionForState=getMaxQValue(setRef);
			retPolicy->bestAction(it->first,bestActionForState.a,bestActionForState.v,1.0); //last parameter (confidence) irrelevant for LookupP	
		}
		return retPolicy;	
	}

	virtual void printStats(std::ostream& o) const{
		o<<"size of q-table: "<<q.size();
#ifdef KEEP_AVG_CHANGE
		o<<" average q-change in the last "<<KEEP_AVG_CHANGE<<" updates: "<<getAvgChange();
#endif
	}

protected:
	typedef ActionValuePair<ActionT,UtilityDataTypeT> ActionValuePairT;
	typedef std::set<ActionValuePairT> ActionValueSetT;
	typedef LookupPolicy<StateT, ActionT> LookupPolicyT;

	typedef std::map<StateT,ActionValueSetT> QMap; 
	typedef typename QMap::iterator QMap_iterator;
	typedef typename QMap::const_iterator QMap_const_iterator;



	/**
	 * Finds the action with the maximum expected utility, that is, finds 
	 * argmax_over_a{ expl(Q[s,a],freq[s,a]) } where expl is the exploration function,
	 * and freq is the frequency of action a tried from state s.
	 * 
	 * Only X percent of the time, this maximum action is NOT found, but instead a random
	 * action is returned (where X = QLearningController::epsilonGreedy), thereby adding an epsilon-greedy
	 * strategy.
	 *
	 * IMPORTANT: This class should only be used internally, while the qlearning object
	 * passed in constructor is valid and unchanged
	 */
	class MaxExpectedUtility: public ActionAlgorithm<ActionT>{
	public:
		/**
	 	 * \param _qlearn the QLearningController object. Reference is kept internally!
		 * \param _s the state from which the optimal action will be chosen
	 	 */
		explicit MaxExpectedUtility(const QLearningControllerT& _qlearn, const StateT& _s):
			qlearn(_qlearn),qit(qlearn.getQEntry(_s)),s(_s),applied(false){
	
			//generate a random number [0..1] to see whether we should try the best
			//action, or rather a random action.
			float rdm = (float)(RAND_MAX-RandomNumberGenerator::random()) / (float)RAND_MAX;
			if (rdm >= qlearn.epsilonGreedy){
				returnBest=true;
			} else {
				//PRINTMSG("Return RANDOM");
				returnBest=false;
				ActionT a=qlearn.actionGenerator->randomAction(); //generate random action
				UtilityDataTypeT actionUtility = qlearn.defaultQ; 
				//find q-entry for this action:
				qlearn.getQValue(qit,a,actionUtility); //if no q-value exists yet, actionUtility will remain unchanged
				maxAction=ActionValuePairT(a,actionUtility);
			}

		}
		virtual ~MaxExpectedUtility(){}

		virtual bool apply(const ActionT& a){
			if (!returnBest) {
				if (!applied) applied=true;
				return true; //we don't have to explore, because a random action is to be returned
			}

			UtilityDataTypeT actionUtility = qlearn.defaultQ; 
			//find q-entry:
			qlearn.getQValue(qit,a,actionUtility); //if no q-value exists yet, actionUtility will remain unchanged
			//get freqency of action a tried from state s
			FreqCntT freq=qlearn.getFrequency(s,a);
			//get estimated reward determined by exploration function
			UtilityDataTypeT ut=qlearn.exploration->getEstimatedReward(actionUtility,freq);
			//PRINTMSG("Checking action "<<a<<" for state "<<s<<", yields in utility "<<ut);
			if (!applied) {//first action: this will be the best found so far
				maxAction.a=a;
				maxAction.v=ut;
				applied=true;
				return true;
			}
			if (ut>maxAction.v){
				maxAction.a=a;
				maxAction.v=ut;
			}
			return true;
		}
		//returns false if no max action was found (i.e. no actions were generated).
		//then, getBestAction() will return an undefined action.
		bool hasResult(){
			return applied;
		}
		/**
		 * Returns the action found with the best utility, given the set exploration function.
		 * In addition to the exploration funciton, a random action is chosen X percent of
		 * the time instead of the best action found, witch probability QLearningController::epsilonGreedy 
		 * (epsilon-greedy exploration)
	 	 *
		 * See also hasResult()
		 */
		ActionValuePairT getBestAction(){
			return maxAction;
	
		}
	private:
		MaxExpectedUtility(){}
		//the assigned exploration function for trying new actions from particular states
		const QLearningControllerT& qlearn;
		QMap_const_iterator qit; //iterator pointing to the q-table entry for state s
		StateT s;
		ActionValuePairT maxAction;
		bool applied;
		//set to true if the best action has to be found. If set to false, 
		//always a random action is returned.
		bool returnBest; 
	};


	/**
	 * Finds the action with the maximum associated q-value, that is, finds 
	 * max_over_a(Q[s,a]). If the best action found so far is not in the q-table yet, 
	 * isNewAction() will return true, and getMaxAction() will then return an action with 
	 * defaultQ as utility. This will be an action which has been tried and was not in 
	 * the q-table yet, when defaultQ was higher than all utilities assigned to other actions
	 * from this state.
	 *
	 * IMPORTANT: This class should only be used internally, while the qlearning object
	 * passed in constructor is valid and unchanged
	 */
	class MaxQValue: public ActionAlgorithm<ActionT>{
	public:
		/**
	 	 * \param _qlearn the QLearningController object. Reference is kept internally!
		 * \param _s the state from which the optimal action will be chosen
	 	 */
		explicit MaxQValue(const QLearningControllerT& _qlearn, const StateT& _s):
			qlearn(_qlearn),qit(qlearn.getQEntry(_s)),applied(false),isNew(false){}
		virtual ~MaxQValue(){}

		virtual bool apply(const ActionT& a){
			UtilityDataTypeT actionUtility = qlearn.defaultQ; 

			//if getQValue returns false, no such action is in the q-table yet,
			//and actionUtility will remain unchanged
			bool newAction=!qlearn.getQValue(qit,a,actionUtility); 

			//PRINTMSG("Checking action "<<a<<" for state "<<s<<", yields in utility "<<ut);
			if (!applied) {//first action tried: This will be the best action found so far
				maxAction.a=a;
				maxAction.v=actionUtility;
				isNew=newAction;
				applied=true;
				return true;
			}
			if (actionUtility>maxAction.v){
				maxAction.a=a;
				maxAction.v=actionUtility;
				isNew=newAction;
			}
			return true;
		}
		//returns false if no max action was found (i.e. no actions were generated).
		//then, getMaxEntry() will return an undefined action.
		bool hasResult(){
			return applied;
		}
		bool isNewAction(){
			return isNew;
		}
		//see also hasResult()
		ActionValuePairT getMaxEntry(){
			return maxAction;
		}
	private:
		MaxQValue(){}
		//the assigned exploration function for trying new actions from particular states
		const QLearningControllerT& qlearn;
		QMap_const_iterator qit; //iterator pointing to the q-table entry for state s
		ActionValuePairT maxAction;
		bool applied;
		bool isNew;
	};




	/**
	 */
	virtual bool learnOnline(const StateT& currentState){
		if (!this->domain->getReward().get()){
			PRINTERROR("Need reward function to update q-table");
			return false;
		}
		float currReward=this->domain->getReward()->getReward(currentState);
		//PRINTMSG("Reward "<<currReward<<" for "<<currentState);
		update(currentState,currReward);
		return true;
	}
	
	virtual ActionT getBestAction(const StateT& currentState)const{
		return lastAction;
	}

	virtual bool initializeImpl(const StateT& startState){
		initialised=true;
		return true;
	}


	ActionT update(const StateT& s, const RewardValueTypeT& reward){
		if (lastState.get()){
#ifdef LEARN_TRANSITION
			//PRINTMSG("Experience "<<*lastState<<" -> "<<s);
			learnedTransition->experienceTransition(*lastState,lastAction,s);
#endif
			updateFreqAndQTable(s,reward);
		}
		if (this->domain->isTerminalState(s)) {
			//PRINTMSG("  ####### Reached terminal state. Recommend old action "<<lastAction<<". Current reward: "<<reward);
			lastState=NULL;
			lastReward=0.0;
		}else{
			MaxExpectedUtility mUt(*this,s);
			actionGenerator->foreachAction(mUt);
			if (mUt.hasResult()){
				lastAction=mUt.getBestAction().a;
				//PRINTMSG("   Maximum expected utility for "<<s<<": "<<lastAction);
				lastState=StatePtrT(new StateT(s)); //XXX possibly debug after changes on 25/10/15
				lastReward=reward;
			}else{
				PRINTMSG("WARNING: No actions were applied on the state "<<s<<", this will reset the Q-learning algorithm. Is it a bug?");
				lastState=NULL;
			}
		}
		return lastAction;
	}





	/**
	 * helper function to update the frequency table and the q-values table for the current state s.
	 */
	void updateFreqAndQTable(const StateT& s, const RewardValueTypeT& reward){
		if (!lastState.get()){
			throw Exception("invalid lastState pointer!",__FILE__,__LINE__);
		}
		//update the frequencies map and the q-value.
		std::pair<NSA_iterator,bool> it = nsaFreq.insert(std::make_pair(StateActionPairT(*lastState,lastAction),1)); 
		if (!it.second){ //Inserting was not successful, so the entry exists already. Update it.
			it.first->second=it.first->second+1; 
		}
		unsigned int numTried=it.first->second-1; //will be at least 0 (this trial does not count yet)
		double adaptedLearnRate=learnRate->get(numTried);
		if (adaptedLearnRate < std::numeric_limits<float>::epsilon()) {
#ifdef KEEP_AVG_CHANGE
			updateAverage(0.0);
#endif
			return; //no learning any more, as learning rate is down
		}

		//update q-value:
		//--- step 1: calculate expected discounted reward for the transition
		//from the last state to the current:
		//  expectedDiscountedReward = reward + discountFactor * max_over_a(Qtable[currentState,a])
		UtilityDataTypeT bestActionUtility=defaultQ;
		if (this->domain->isTerminalState(s)){ //we are at a terminal state. From here, we won't have any transitions from any actions
#ifdef UPDATE_WITH_OLD_REWARD
			bestActionUtility=reward; //we have to pass on the current reward for the update of the old q-value
#else
			bestActionUtility=0.0; //since we use the current reward (terminal) for update, we won't need more future rewards
#endif
		}else{
			MaxQValue mQ(*this,s); //will calculate max_over_a(Qtable[s,a])
			actionGenerator->foreachAction(mQ);
			if (mQ.hasResult()){
				bestActionUtility=mQ.getMaxEntry().v;
			}else{
				PRINTMSG("WARNING: No actions were applied on the state "<<s<<". Is it a bug?");
			}
		}
		//PRINTMSG(" | Maximum expected utility for "<<s<<": "<<bestAction);

#ifdef UPDATE_WITH_OLD_REWARD
		RewardValueTypeT updateReward=lastReward;
#else
		RewardValueTypeT updateReward=reward;
#endif
		UtilityDataTypeT expectedDiscountedReward = updateReward + discount * bestActionUtility;

		//--- step 2: weigh the old value against the expected discounted reward by the learning rate
		//q(lastState,lastAction) = (1-learnRate)*q(lastState,lastAction) + learnRate*expectedDiscountedReward;

		//now, retrieve and update the value in the q-table Q[lastState, lastAction]
		//first, try to insert an empty value. If this fails, we'll have the iterator to the existing value at least.
		std::pair<QMap_iterator,bool> qitI = q.insert(std::make_pair(*lastState,ActionValueSetT())); 
		ActionValueSetT& setRef=qitI.first->second; //For code readability, we'll keep a reference to the action set

		UtilityDataTypeT lastQ=defaultQ; //if no q[lastState,lastAction] exist, we'll assume default q value
		typename ActionValueSetT::iterator setIt = setRef.end();
		if (!qitI.second) { //q-values exist for lastState, so try to find lastAction as well in the set
			setIt=setRef.find(ActionValuePairT(lastAction,0));
		}
		if (setIt!=setRef.end()){ //lastState/lastAction had value assigned
			lastQ=setIt->v;
			/* We have to erase this entry because ActionValuePairT is a key and thus 
			 * the iterators are const. The values can't be changed. Even if only the action has a key
			 * function and basically we could change the value. We could add a const method to 
			 * ActionValuePair which changes only v (havnig no effect on the key) but this is dodgey, so for
			 * now we'll keep the more unefficient way of replacing the set entry altogether */
			setRef.erase(setIt);
		}

		/*if ((numTried>100000) && (fabs(bestActionUtility+reward-lastQ) > 0.1)) {
			PRINTMSG("Strange, we still get quite a big change: "<<(bestActionUtility+reward-lastQ)<<" tried="<<numTried<<", "<<s);
		}*/

		//New Q-Value:		
		//a) version as in Russell&norwig (although I believe they have a mistake using the last reward), and as in wikipedia:
		UtilityDataTypeT qDiff=adaptedLearnRate*(expectedDiscountedReward-lastQ); 
		UtilityDataTypeT newQ=lastQ + qDiff;
		//b) simpler version as in Mitchell:
		//UtilityDataTypeT newQ=expectedDiscountedReward; 
			
#ifdef KEEP_AVG_CHANGE
		updateAverage(qDiff);
#endif		
		//PRINTMSG("new q: "<<newQ<<", adaptedLearnRate="<<adaptedLearnRate<<", reward="<<reward<<", expected reward: "<<expectedDiscountedReward<<", bestAction="<<bestAction.v<<", discount="<<discount);
		
		//insert new value in q-table
		if (!setRef.insert(ActionValuePairT(lastAction,newQ)).second){ //will succeed because this is an empty set
			PRINTERROR("Consistency!"); //this will not happen, but I'll leave it for debugging
		}

		//PRINTMSG(" | Expected reward for "<<*lastState<<" -> "<<s<<": "<<expectedDiscountedReward<<" best Action: "<<bestAction<<" reward="<<reward);
		//PRINTMSG(" | old Q: "<<lastQ<<", new Q: "<<newQ);
	}



	/**
	 * From a set of actions with q-values associated, pick the one
	 * action which has the maximum q-value.
	 */
	ActionValuePairT getMaxQValue(const ActionValueSetT& avSet) const{
		if (avSet.empty()){ //the set should NOT be empty!
			throw Exception("No actions were assigned in set. This is an inconsistency.",__FILE__,__LINE__);
		}

		ActionValuePairT maxAction;
		//find the action which yields in the maximum utility
		typename ActionValueSetT::const_iterator valIt;
		for (valIt=avSet.begin(); valIt!=avSet.end(); ++valIt){
			if (valIt==avSet.begin()) { //first iteration: update maximum 
				maxAction=ActionValuePairT(*valIt);
				continue;
			}	
			if (valIt->v > maxAction.v){
				maxAction=ActionValuePairT(*valIt);
			}
		}
		return maxAction;
	}	

	/**
	 * Helper function, returns iterator to q-entry for the state
	 */
	QMap_const_iterator getQEntry(const StateT& s) const{
		return q.find(s); //get the q-entry for the state
	}	


	/**
	 * Helper function to retrieve the Q-Value for a state-action pair. Returns
	 * false if no such pair is in the Q-table yet, and actionUtility remains unchanged.
	 * If a Q-valueu exists, the function returns true and actionUtility will contain the assigned value.
	 */
	bool getQValue(const StateT& s, const ActionT& a, UtilityDataTypeT& actionUtility) const {
		return getQValue(getQEntry(s),a,actionUtility);	
	}


	/**
	 * Helper function to retrieve the Q-Value for a state-action pair, given that the parameter qit
	 * points to the entry for a state. All actions previously tried from this state will have to be contained
	 * in this entry.  
	 * Returns false if no such pair is in the Q-table yet, and actionUtility remains unchanged.
	 * If a Q-valueu exists, the function returns true and actionUtility will contain the assigned value.
	 */
	bool getQValue(QMap_const_iterator qit, const ActionT& a, UtilityDataTypeT& actionUtility) const {
		if (qit!=q.end()){  //current state exists in the q-table
			const ActionValueSetT& setRef=qit->second; //for code readability, we'll get the reference
			typedef typename ActionValueSetT::iterator SetItT;
			//find the action in the set. Utiltiy value does not matter as it is not used for sorting.
			SetItT setIt = setRef.find(ActionValuePairT(a,0)); 
			if (setIt!=setRef.end()){
				actionUtility=setIt->v;
				return true;
			}
		}	
		return false;
	}



	/**
	 * Helper function to retrieve the frequency for a state-action pair. 
	 */
	FreqCntT getFrequency(const StateT& s, const ActionT& a) const{
		NSA_const_iterator it=nsaFreq.find(StateActionPairT(s,a));
		if (it==nsaFreq.end()) return 0;
		return it->second;
	}

	void printQValues(std::ostream& o) const{
		QMap_const_iterator it;
		for (it=q.begin(); it!=q.end(); ++it){
			const ActionValueSetT& setRef=it->second; //keep a reference for better code readability
			typename ActionValueSetT::iterator sIt;
			for (sIt=setRef.begin(); sIt!=setRef.end(); ++sIt){
				o<<it->first<<" / "<<*sIt<<std::endl;
			}
		}
	}



	float getAvgChange() const{
#ifdef KEEP_AVG_CHANGE
		UtilityDataTypeT sum=0;
		typename std::deque<UtilityDataTypeT>::const_iterator it;
		for (it=avg.begin(); it!=avg.end(); ++it) {
			sum+=*it;
		}
		return sum/avg.size();
#else
		return 0.0;
#endif
	}

	void updateAverage(UtilityDataTypeT diff){
#ifdef KEEP_AVG_CHANGE
		if (avg.size()>= KEEP_AVG_CHANGE) {
			avg.pop_front();
		}
		avg.push_back(diff);
#endif
	}
private:

#ifdef KEEP_AVG_CHANGE
	std::deque<UtilityDataTypeT> avg;
#endif
	typedef StateActionPair<StateT,ActionT> StateActionPairT;

	typedef std::map<StateActionPairT,FreqCntT>  NSAFreq; 
	typedef typename NSAFreq::iterator NSA_iterator;
	typedef typename NSAFreq::const_iterator NSA_const_iterator;


	//Frequencies of the state-action pairs
	NSAFreq nsaFreq; //number of observed state-action pairs

	//q-map. Because the value type of this map is an ordered set of type ActionValuePairT, 
	//the entries will be ordered by the actions, and actions will be unique.
	QMap q; 

	StatePtrT lastState;//state in the last update step
	ActionT lastAction; //last action performed, with corresponding q-value
	RewardValueTypeT lastReward; //experienced reward in the last update step
	LearningRatePtrT learnRate;
	float discount; 
	UtilityDataTypeT defaultQ; //default q value
	
	//should generte all possible actions for the underlying domain
	ActionGeneratorConstPtrT actionGenerator;

	//the assigned exploration function for trying new actions from particular states
	ExplorationConstPtrT exploration;
	//value 0..1 indicating a probability that not best, but a random
	//action is chosen. This adds to the exploration function. It is NOT
	//implemented as Exploration interface because it does not matter how often
	//an action has been tried before. ALWAYS a not optimal action will be chosen,
	//i.e. there will constantly be epxolration.
	float epsilonGreedy;

	PolicyPtrT policy;
	
	bool initialised;
#ifdef LEARN_TRANSITION
	std::shared_ptr<LearnableTransitionMapT> learnedTransition;
#endif

};

}

#endif
