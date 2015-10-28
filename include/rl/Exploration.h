#ifndef __EXPLORATION_H__
#define __EXPLORATION_H__

#include <memory>


namespace rl{
/**
 * \brief Interface for an exploration function, taking an utility
 * value (e.g. q-value) and a frequency of an action tried in a state
 * as input, and returning a utility value.
 * 
 * The function return value should increase with utility value and
 * decrease with an increasing n value.
 *
 * \author Jennifer Buehler
 * \date May 2011
 */
template<typename UtilityValue=float, typename FrequencyType=unsigned int>
class Exploration {
public:
	typedef UtilityValue UtilityT;
	typedef FrequencyType FrequencyT;

	typedef Exploration<UtilityT,FrequencyT> ExplorationT;
	typedef std::shared_ptr<ExplorationT> ExplorationPtrT;
	typedef std::shared_ptr<const ExplorationT> ExplorationConstPtrT;

	Exploration(){}
	Exploration(const Exploration& o){}
	virtual ~Exploration(){}
	/**
	 * returns an estimation of the expected reward given the utility u
	 * (e.g. of doing an action in a particular state) and the frequency
	 * f how often this has been tried before.
	 */
	virtual UtilityT getEstimatedReward(const UtilityT& u, const FrequencyT& f) const=0; 
};

/**
 * Simple exploration function which returns an optimistic expected reward
 * if this evaluation has been tried less than a threshold, and the
 * utility itself otherwise.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<typename UtilityValue=float, typename FrequencyType=unsigned int>
class SimpleExploration: public Exploration<UtilityValue,FrequencyType>{
public:
	typedef UtilityValue UtilityT;
	typedef FrequencyType FrequencyT;

	explicit SimpleExploration(const FrequencyT _freqThreshold, const UtilityT& _maxReward=1.0): 
		freqThreshold(_freqThreshold),maxReward(_maxReward){}
	SimpleExploration(const SimpleExploration& o){}
	virtual ~SimpleExploration(){}
	virtual UtilityT getEstimatedReward(const UtilityT& u, const FrequencyT& f) const{
		if (f<freqThreshold) return maxReward;
		return u;
	}
private:
	SimpleExploration(){}
	FrequencyT freqThreshold;
	UtilityT maxReward;
};

/**
 * No exploration to be performed by strategy function. Always the
 * same utility is returned as passed into getEstimateReward().
 * \author Jennifer Buehler
 * \date May 2011
 */
template<typename UtilityValue=float, typename FrequencyType=unsigned int>
class NoExploration: public Exploration<UtilityValue,FrequencyType>{
public:
	typedef UtilityValue UtilityT;
	typedef FrequencyType FrequencyT;

	explicit NoExploration(){}
	NoExploration(const NoExploration& o){}
	virtual ~NoExploration(){}
	virtual UtilityT getEstimatedReward(const UtilityT& u, const FrequencyT& f) const{
		return u;
	}
};


/**
 * Interface to keep a learning rate, either decaying
 * in different ways (subclasses of this) or not deaying (this class)
 */ 
class LearningRate{
public:

	typedef std::shared_ptr<LearningRate> LearningRatePtrT;
	typedef std::shared_ptr<const LearningRate> LearningRateConstPtrT;

	LearningRate(float initialValue):learnRate(initialValue){
		if (learnRate>1.0f) learnRate=1.0f;
		if (learnRate<0.0f) learnRate=0.0f;
	}
	virtual ~LearningRate(){}

	
	//update learn rate using a freqency value
	virtual float get(unsigned int freq) const{
		return learnRate;
	}
protected:
	float learnRate;
};

class DecayLearningRate: public LearningRate{
public:
	DecayLearningRate(float _decayRate=1e-01):LearningRate(0.99),decayRate(_decayRate){} //use intitial value of 0.99 because the learn rate is decaying
	virtual ~DecayLearningRate(){}
	virtual float get(unsigned int freq) const{ 
		//double decayFact=learnRate*learnRate*decayRate;
		double decayFact=this->learnRate*decayRate;
		float adaptedLearnRate=(double)learnRate/(1.0+decayFact*freq);
		return adaptedLearnRate;	

	} 
protected:
	float decayRate;
};






}

#endif
