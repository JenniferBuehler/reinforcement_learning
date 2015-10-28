#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <sstream>
#include <map>
#include <memory>
#include <rl/LogBinding.h>

namespace rl{

/**
 * \brief Implements the utility function which expresses the utility of a state.
 * This can be a function or a lookup in a table.
 *
 * This interface can also be used for learning a utility function, by using method
 * experienceUtility().
 *
 * \param State template class for State description. Prerequisite: Must support < operator,
 * an be uniquely identifiable, i.e. suitable to use in a std::map as key. Even if the Utility
 * is being calculated as a function by the underlying implementation, it is to be made sure
 * that a map-lookup can be used as well for each state.
 *
 * \param Value the value type to be used as utility 
 * \author Jennifer Buehler
 * \date May 2011
 *
 */
template<class State, typename Value=float>
class Utility {
public:
	typedef State StateT;
	typedef Value ValueT;
	typedef Utility<StateT,ValueT> UtilityT;
	typedef std::shared_ptr<UtilityT> UtilityPtrT;
	typedef std::shared_ptr<const UtilityT> UtilityConstPtrT;

	Utility(){}
	Utility(const Utility& o){}
	virtual ~Utility(){}


	static UtilityPtrT makePtr(UtilityT * ref){
		return UtilityPtrT(ref);
	}
	

	/**
	 * Returns utility for the state, alogn with a mean and variance expressing
	 * the confidence of this utility. Some subclasses may not support mean and variance.
	 * Find out with method supportsMeanVariance(); If not supported, the values of
	 * parameters mean and variance will be undefined.
	 */
	virtual ValueT getUtility(const StateT& s, float& mean, float& variance)const =0;
	/**
	 * A specific utility was experienced in this state. It depends on the
	 * implementing subclass whether this will trigger any internal changes 
	 * (e.g. learning), though in most implementations this should have an effect.
	 */
	virtual void experienceUtility(const StateT& s, const ValueT& v)=0;

	/**
	 * Should return true for subclasses which return mean (and variance) in function getUtility().
	 * \param onlyMean will be set to true if the function supports only the mean, and no variance.
	 *   If the function returns false, the value of this parameter is undefined.
	 */
	virtual bool supportsMeanVariance(bool& onlyMean){
		onlyMean=false;
		return false;
	}

	virtual UtilityPtrT clone()const=0;
	virtual void print(std::stringstream& str)const=0;
protected:
	
};


/**
 * Each state is uniquely associated with one utility that is assigned in experienceUtility().
 * This is a simple map of State objects to values.
 * \author Jennifer Buehler
 * \date May 2011
 */
template<class State, typename Value=float>
class MappedUtility: public Utility<State,Value> {
public:
	typedef Value ValueT;
	typedef State StateT;
	typedef Utility<StateT,ValueT> UtilityT;
	typedef typename UtilityT::UtilityPtrT UtilityPtrT;
	
	typedef MappedUtility<StateT,ValueT> MappedUtilityT;

	/**
	 * \param _defaultValue the default utility to be returned if there is no other utility assigned
	 */
	explicit MappedUtility(const ValueT& _defaultValue=0): UtilityT(),defaultValue(_defaultValue){}
	explicit MappedUtility(const MappedUtility& o): UtilityT(o),specificUtilities(o.specificUtilities),defaultValue(o.defaultValue){}
	virtual ~MappedUtility(){}
	


	virtual ValueT getUtility(const StateT& s, float& mean, float& variance)const{
		typename SpecificUtilitiesMapT::const_iterator it = specificUtilities.find(s);
		if (it==specificUtilities.end()){ //no specific utility for this state
			return defaultValue;
		}
		return it->second;
	}
	
	virtual void experienceUtility(const StateT& s, const ValueT& v){
		//PRINTMSG("Experience utility "<<s<<" -> "<<v);
		typename SpecificUtilitiesMapT::iterator it;
		it=specificUtilities.find(s);
		if (it==specificUtilities.end()){ //no association for this state yet: insert
			specificUtilities.insert(std::make_pair(s,v)); //will not return false, as not encountered in map yet
			return;
		}
		//update utility for state
		it->second=v;
	}
	
	virtual void print(std::stringstream& strng)const{
		typename SpecificUtilitiesMapT::const_iterator it;
		for (it=specificUtilities.begin(); it!=specificUtilities.end(); ++it){
			strng<<it->first<<" -> "<<it->second<<std::endl;
		}
	}
	virtual UtilityPtrT clone()const{
		return UtilityPtrT(new MappedUtilityT(*this));
	}

protected:

	typedef std::map<StateT,ValueT> SpecificUtilitiesMapT; 
	SpecificUtilitiesMapT specificUtilities;

	ValueT defaultValue;
 
private:
	MappedUtility(){}
};

} //namespace
#endif

