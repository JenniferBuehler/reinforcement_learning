#ifndef __REWARD_H__
#define __REWARD_H__

#include <map>
#include <deque>

#include <rl/LogBinding.h>
#include <general/Exception.h>

namespace rl{

/**
 * \brief Implements the reward function.
 * This can be a function or a lookup in a table.
 *
 * \param State template class for State description. Prerequisite: Must support < operator,
 * an be uniquely identifiable, i.e. suitable to use in a std::map as key. Even if the Reward
 * is being calculated as a function by the underlying implementation, it is to be made sure
 * that a map-lookup can be used as well for each state.
 *
 * \param Value the value type to be used as reward
 * \author Jennifer Buehler
 * \date May 2011
 *
 */
template<class State, typename Value=float>
class Reward{
public:
	typedef Value ValueT;
	typedef State StateT;
	typedef Reward<StateT,ValueT> RewardT;
	typedef typename std::shared_ptr<RewardT> RewardPtrT;
	typedef typename std::shared_ptr<const RewardT> RewardConstPtrT;

	Reward(){}
	Reward(const Reward& o){}
	virtual ~Reward(){}
	/**
	 * Gets the reward given a state
	 */
	virtual ValueT getReward(const State& s)const =0;
	/**
	 * gets an optimistic estimate of the reward, usually
	 * this would be the maximum reward possible in any state.
	 */
	virtual ValueT getOptimisticReward()const =0;

protected:

};


/**
 * Specific states can be associated with a specific reward,
 * and all other states will receive the default reward
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, typename Value=float>
class SelectedReward: public Reward<State,Value> {
public:
	typedef State StateT;
	typedef Value ValueT;
	typedef Reward<StateT,ValueT> ParentT;
	typedef std::pair<StateT,ValueT> StateValuePairT;

	explicit SelectedReward(const ValueT& _defaultValue): ParentT(),defaultValue(_defaultValue),maxReward(_defaultValue){}

	SelectedReward(std::deque<StateValuePairT> _specificRewards, const ValueT& _defaultValue) 
		throw (Exception)
		: defaultValue(_defaultValue), maxReward(_defaultValue),ParentT() {

		if (!addSpecificRewards(_specificRewards)) throw Exception("Could not initialise specific rewards",__FILE__,__LINE__);
	}

	SelectedReward(const SelectedReward& o): defaultValue(o.defaultValue), ParentT(o){}
	virtual ~SelectedReward(){}

	virtual ValueT getReward(const StateT& s)const{
		typename SpecificRewardsMapT::const_iterator it = specificRewards.find(s);
		if (it==specificRewards.end()){ //no specific reward for this state
			return defaultValue;
		}
		return it->second;
	}

	bool addSpecificReward(const StateT& s, const ValueT reward){
		if (!specificRewards.insert(std::make_pair(s,reward)).second){
			PRINTERROR("Double special reward state "<<s<<" encountered!");
			return false;
		}
		if (reward > maxReward) maxReward=reward;
		return true;
	}

	bool addSpecificRewards(std::deque<StateValuePairT>& _specificRewards){
		typename std::deque<StateValuePairT>::iterator it;
		for (it=_specificRewards.begin(); it!=_specificRewards.end(); ++it) {
			if (!addSpecificReward(it->first,it->second)){
				PRINTERROR("Could not insert reward state "<<it->first<<" encountered!");
				return false;
			}
		}
		return true;
	}

	virtual ValueT getOptimisticReward()const{
		return maxReward*1.1; //overestimation of best reward
	}

protected:

	ValueT defaultValue;
	typedef std::map<StateT,ValueT> SpecificRewardsMapT; 
	SpecificRewardsMapT specificRewards;
	ValueT maxReward;
 
private:
	SelectedReward(){}
};

}

#endif
