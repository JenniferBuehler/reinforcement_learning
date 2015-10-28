#ifndef __GRID_WORLD_H__
#define __GRID_WORLD_H__

#include <rl/StateAlgorithms.h>
#include <rl/State.h>
#include <rl/Domain.h>

#include <math/RandomNumber.h>
#include <general/Exception.h>

#include <stdlib.h>
#include <time.h>

namespace rl{

/**
 * \brief State of the grid world.
 *
 * \author Jennifer Buehler
 * \date May 2011
 */
class GridWorldState: public StateBase {
public:
	typedef StateBase StateT;

        GridWorldState(int _x=0, int _y=0): x(_x),y(_y){}
        GridWorldState(const GridWorldState& o): x(o.x),y(o.y){}
        virtual ~GridWorldState(){}

	unsigned int getX() const {
		return x;
	}        
	unsigned int getY() const {
		return y;
	}        

	GridWorldState& operator=(const GridWorldState& o) {
		if (this==&o) return *this;
		assignImpl(o);
		return *this;
	}


protected:

        virtual bool less(const StateT& s) const{
		const GridWorldState * scast = dynamic_cast<const GridWorldState*>(&s);
		if (!scast) throw Exception("Cannot compare incompatible state types",__FILE__,__LINE__);
		return (x<scast->x) || ((x==scast->x) && (y<scast->y));
	}
	
	virtual void assign(const StateBase& s){
		const GridWorldState * scast = dynamic_cast<const GridWorldState*>(&s);
		if (!scast) throw Exception("Cannot compare incompatible state types",__FILE__,__LINE__);
		assignImpl(*scast);
	}

       
	void assignImpl(const GridWorldState& o){
		x=o.x;
		y=o.y;
	}

	 /**
         * print description of the state into ostream o, and return o.
         */
        virtual void print(std::ostream& o) const{
		o<<x<<"/"<<y;
	}

public:
        unsigned int x,y;
};

/**
 * \brief Action to move in the grid world.
 * \author Jennifer Buehler
 * \date May 2011
 */
class MoveAction : public ActionBase{
public:
	typedef enum Moves {Right, Up, Down, Left} MovesT;

	MoveAction():ActionBase(),mv(Up){}
	explicit MoveAction(MovesT m): ActionBase(),mv(m){}
	MoveAction(const MoveAction& o): ActionBase(o), mv(o.mv){}

	virtual ~MoveAction(){}

	MovesT getMove() const{
		return mv;
	}
	
	MoveAction& operator=(const MoveAction& o) {
		if (this==&o) return *this;
		assignImpl(o);
		return *this;
	}


protected:

	virtual bool less(const ActionBase& a) const{
		const MoveAction * dcast = dynamic_cast<const MoveAction*>(&a);
		if (!dcast) throw Exception("Cannot compare incompatible action types",__FILE__,__LINE__);
		return mv < dcast->mv;
	}

	/**
	 * print description of the state into ostream o, and return o.
	 */
	virtual void print(std::ostream& o) const{
		switch(mv){
			case Up: {o<<"UP"; break; }
			case Down: {o<<"DOWN"; break; }
			case Left: {o<<"LEFT"; break; }
			case Right: {o<<"RIGHT"; break; }
		}	
	}

	virtual void assign(const ActionBase& a){
		const MoveAction * cast = dynamic_cast<const MoveAction*>(&a);
		if (!cast) throw Exception("Cannot compare incompatible state types",__FILE__,__LINE__);
		assignImpl(*cast);
	}

       
	void assignImpl(const MoveAction& o){
		mv=o.mv;
	}

private:
	MovesT mv;	
};

/**
 * \brief Generates states for the grid world
 * \author Jennifer Buehler
 * \date May 2011
 */
class GridWorldStateGenerator: public StateGenerator<GridWorldState>{
public:
	typedef StateAlgorithm<GridWorldState> StateAlgorithmT;
	/**
	 * \param _maxX and _maxY: dimensions of the grid world
	 * \param _blockX and _blockY: where the block is placed in the world.
	 */
	GridWorldStateGenerator(unsigned int _maxX,  unsigned int _maxY, 
		unsigned int _blockX, unsigned int _blockY): 
		maxX(_maxX),maxY(_maxY),blockX(_blockX),blockY(_blockY){}

	virtual ~GridWorldStateGenerator(){}

	virtual bool foreachState(StateAlgorithmT& s) const{
		for (unsigned int x=0; x<maxX; ++x) {
			for (unsigned int y=0; y<maxY; ++y) {
				if ((x==blockX) && (y==blockY)) continue;
				//PRINTMSG("------- Generate "<<x<<", "<<y);
				if (!s.apply(GridWorldState(x,y))){
					return false;
				}
			}
		}
		return true;
	}
	virtual GridWorldState randomState()const{
		unsigned int numX,numY;
		do{
			numX = RandomNumberGenerator::random() % maxX; //generate 0..(maxX-1)
			numY = RandomNumberGenerator::random() % maxY; //generate 0..(maxY-1)
		}while ((numX==blockX) && (numY==blockY));
		return GridWorldState(numX,numY);
	}

private:
	unsigned int maxX,maxY,blockX,blockY,goalX,goalY,pitX,pitY;
};

/**
 * \brief Generates actions for the grid world
 * \author Jennifer Buehler
 * \date May 2011
 */
class GridWorldActionGenerator: public ActionGenerator<MoveAction> {
public:
	typedef ActionAlgorithm<MoveAction> ActionAlgorithmT;

	GridWorldActionGenerator(){
	}
	virtual ~GridWorldActionGenerator(){}

	virtual bool foreachAction(ActionAlgorithmT& a)const{
		if (!a.apply(MoveAction(MoveAction::Up)) ||
		    !a.apply(MoveAction(MoveAction::Right)) ||
		    !a.apply(MoveAction(MoveAction::Down)) ||
		    !a.apply(MoveAction(MoveAction::Left))){
			return false;
		}
		return true;
	}
	virtual MoveAction randomAction()const{
		int num = RandomNumberGenerator::random() % 4;
		//PRINTMSG("Random: "<<num);
		switch(num){
			case 0: return MoveAction(MoveAction::Up);
			case 1: return MoveAction(MoveAction::Down);
			case 2: return MoveAction(MoveAction::Left);
			case 3: return MoveAction(MoveAction::Right);
		}
		PRINTERROR("DEBUG: Should not get here!");
		return MoveAction(MoveAction::Left);
	}

};


 

 
/**
 * \brief Simple transition function which is known a priori for the grid world
 * and does not have to be learned. 
 *
 * It would also be possible to use TransitionStlMap and pre-fill it with all
 * known transitions, but this would be overshoot for this simple problem.
 *
 * \author Jennifer Buehler
 * \date May 2011
 */
class GridWorldTransition: public Transition<GridWorldState,MoveAction>{
public:
	typedef GridWorldState  StateT;
	typedef Transition<GridWorldState,MoveAction> ParentT;
	typedef GridWorldState State;
	typedef MoveAction Action;
	typedef ParentT::StateTransitionT StateTransitionT;
	typedef ParentT::StateTransitionListT StateTransitionListT;
	typedef ParentT::StateActionStateValueT StateActionStateValueT;

	typedef typename TransitionT::StateTransitionListPtrT StateTransitionListPtrT;
	
	/**
	 * \param _maxX and _maxY: dimensions of the grid world
	 * \param _goalX and _goalY: coordinates of goal (index starts with 0 and can be maximum _maxX! and _maxY-1 respectively)
	 * \param _blockX and _blockY: coordinates of block/wall (index starts with 0 and can be maximum _maxX! and _maxY-1 respectively)
	 * \param _sideActionProbability probability that action to be perform fails [0..1]. With this probability, other available
	 * side actions are executed, if they are possible. If no side action is possible, the main action is performed with possibility 1.
	 * Example: if _sideActionProbability=0.1, and Action is "UP", and both "LEFT" and "RIGHT" are accessible states, 
	 * "UP" is going to be performed with probability 0.8, and "LEFT" and "RIGHT" with probabilities 0.1 repsectively.
	 */
	GridWorldTransition(unsigned int _maxX, unsigned int _maxY, 
	                    unsigned int _goalX, unsigned int _goalY, 
	                    unsigned int _blockX, unsigned int _blockY, 
	                    unsigned int _pitX, unsigned int _pitY, 
			    float _sideActionProbability): 
		maxX(_maxX), maxY(_maxY), goalX(_goalX),goalY(_goalY),blockX(_blockX),blockY(_blockY),pitX(_pitX),pitY(_pitY),
		sideActionProbability(_sideActionProbability){}
	GridWorldTransition(const GridWorldTransition& o){}
	virtual ~GridWorldTransition(){}


	virtual bool getTransitionStates(const State& s, const Action& a, StateTransitionListPtrT& ret)const{
		if (!ret.get()){
			ret=StateTransitionListPtrT(new StateTransitionListT());
		}
		if ((s.getX()==goalX) && (s.getY()==goalY)) return false; //can't get out of goal state
		if ((s.getX()==pitX) && (s.getY()==pitY)) return false; //can't get out of pit
		if ((s.getX()==blockX) && (s.getY()==blockY)) { //can't get out of block, but should not get in either
			PRINTERROR("Consistency: We should not even try the block as a source state!");
			return false; 
		}

		int actionCnt=0;
		bool canLeft,canRight,canUp,canDown;

		//float pMain=1.0-sideActionProbability*(actionCnt-1); //probability for main action, if it's possible to perform it
		float pMain=1.0-sideActionProbability*2; 
		float pSide=sideActionProbability; //default probability for side action
		float bumpP=0.0f;

		if (a.getMove()==MoveAction::Up){
			if ((canLeft=actionPossible(MoveAction::Left,s))) ++actionCnt;
			if ((canRight=actionPossible(MoveAction::Right,s))) ++actionCnt;
			if ((canUp=actionPossible(MoveAction::Up,s))) ++actionCnt;
			
			if (canUp) ret->push_back(StateTransitionT(StateT(s.getX(),s.getY()+1),pMain));
			else bumpP+=pMain;
			
			if (canRight) ret->push_back(StateTransitionT(StateT(s.getX()+1,s.getY()),pSide));
			else bumpP+=pSide;
			if (canLeft) ret->push_back(StateTransitionT(StateT(s.getX()-1,s.getY()),pSide));
			else bumpP+=pSide;
		}else if (a.getMove()==MoveAction::Down){
			if ((canLeft=actionPossible(MoveAction::Left,s))) ++actionCnt;
			if ((canRight=actionPossible(MoveAction::Right,s))) ++actionCnt;
			if ((canDown=actionPossible(MoveAction::Down,s))) ++actionCnt;

			if (canDown) ret->push_back(StateTransitionT(StateT(s.getX(),s.getY()-1),pMain));
			else bumpP+=pMain;

			if (canRight) ret->push_back(StateTransitionT(StateT(s.getX()+1,s.getY()),pSide));
			else bumpP+=pSide;
			if (canLeft) ret->push_back(StateTransitionT(StateT(s.getX()-1,s.getY()),pSide));
			else bumpP+=pSide;
		}else if (a.getMove()==MoveAction::Right){
			if ((canRight=actionPossible(MoveAction::Right,s))) ++actionCnt;
			if ((canUp=actionPossible(MoveAction::Up,s))) ++actionCnt;
			if ((canDown=actionPossible(MoveAction::Down,s))) ++actionCnt;
			

			if (canRight) ret->push_back(StateTransitionT(StateT(s.getX()+1,s.getY()),pMain));
			else bumpP+=pMain;
			
			if (canDown) ret->push_back(StateTransitionT(StateT(s.getX(),s.getY()-1),pSide));
			else bumpP+=pSide;
			if (canUp) ret->push_back(StateTransitionT(StateT(s.getX(),s.getY()+1),pSide));
			else bumpP+=pSide;
		}else if (a.getMove()==MoveAction::Left){
			if ((canLeft=actionPossible(MoveAction::Left,s))) ++actionCnt;
			if ((canUp=actionPossible(MoveAction::Up,s))) ++actionCnt;
			if ((canDown=actionPossible(MoveAction::Down,s))) ++actionCnt;
			
			if (canLeft) ret->push_back(StateTransitionT(StateT(s.getX()-1,s.getY()),pMain));
			else bumpP+=pMain;
			
			if (canDown) ret->push_back(StateTransitionT(StateT(s.getX(),s.getY()-1),pSide));
			else bumpP+=pSide;
			if (canUp) ret->push_back(StateTransitionT(StateT(s.getX(),s.getY()+1),pSide));
			else bumpP+=pSide;
		}

		if (!equalFloats(bumpP,0.0f,(StateActionStateValueT)ZERO_EPSILON)){
			ret->push_back(StateTransitionT(s,bumpP));
		}
		if (ret->empty()) return false;

		return true;
	}

	virtual void setTransitionState(const State& s1, const Action& a, const State& s2, StateActionStateValueT p=1){
		throw Exception("This implementation of transition function is not suitable for learning",__FILE__,__LINE__);
	}
	virtual void print(std::ostream& o)const{
		o<<"No transition print provided for grid world "<<std::endl;
	}
protected:
	bool actionPossible(const MoveAction::MovesT& a, const State& s) const{
		if (a==MoveAction::Up){
			return (s.getY()<(maxY-1)) && ((s.getX()!=blockX) || ((s.getY()+1)!=blockY)); 
		}else if (a==MoveAction::Down){
			return (s.getY()>0) && ((s.getX()!=blockX) || ((s.getY()-1)!=blockY)) ;
		}else if (a==MoveAction::Right){
			return (s.getX()<(maxX-1)) && (((s.getX()+1)!=blockX) || (s.getY()!=blockY));
		}else if (a==MoveAction::Left){
			return (s.getX()>0) && (((s.getX()-1)!=blockX) || (s.getY()!=blockY));
		}
		return false;
	} 
private:
	unsigned int maxX,maxY,goalX,goalY, blockX, blockY,pitX,pitY;
	float sideActionProbability;
};


/**
 * \brief the Domain of the grid world.
 * \author Jennifer Buehler
 * \date May 2011
 */
class GridDomain: public Domain<GridWorldState,MoveAction>{
public:
	typedef GridWorldState StateT;
	typedef MoveAction ActionT;
	typedef float RewardValueTypeT;
	typedef Transition<StateT,ActionT,float> TransitionT;

	typedef StateGenerator<StateT> StateGeneratorT;
	typedef ActionGenerator<ActionT> ActionGeneratorT;
	typedef Reward<StateT,RewardValueTypeT> RewardT;
	typedef SelectedReward<StateT>  SelectedRewardT;

	
	typedef Domain<StateT,ActionT> DomainT;
	typedef typename DomainT::DomainPtrT DomainPtrT;
	typedef typename DomainT::DomainConstPtrT DomainConstPtrT;

	typedef std::shared_ptr<GridDomain> GridDomainPtrT;
	typedef std::shared_ptr<const GridDomain> GridDomainConstPtrT;
	
	typedef typename TransitionT::TransitionPtrT TransitionPtrT;
	typedef typename TransitionT::TransitionConstPtrT TransitionConstPtrT;
	typedef typename RewardT::RewardConstPtrT RewardConstPtrT;
	typedef typename StateGeneratorT::StateGeneratorConstPtrT StateGeneratorConstPtrT;
	typedef typename ActionGeneratorT::ActionGeneratorConstPtrT ActionGeneratorConstPtrT;

	GridDomain( unsigned int _gridX, unsigned int _gridY,
	unsigned int _goalX,unsigned int _goalY,
	unsigned int _blockX, unsigned int _blockY,
	unsigned int _pitX,unsigned int _pitY,
	float _defaultReward,float _goalReward, float _pitReward, float _sideActionProbability=0.1):
		gridX(_gridX), gridY(_gridY), goalX(_goalX),goalY(_goalY),blockX(_blockX),blockY(_blockY),pitX(_pitX),pitY(_pitY),
		defaultReward(_defaultReward),goalReward(_goalReward),pitReward(_pitReward),
		transition(new GridWorldTransition(gridX,gridY, goalX,goalY,blockX,blockY,pitX,pitY,_sideActionProbability))
	{
	}

	virtual ~GridDomain(){}
	
	virtual TransitionConstPtrT getTransition()const{
		return transition;
		//float sideActionProbability=0.1;	
		//return new GridWorldTransition(gridX,gridY, goalX,goalY,blockX,blockY,pitX,pitY,sideActionProbability);
	}
	virtual RewardConstPtrT getReward()const{
		SelectedRewardT * rwd = new SelectedRewardT(defaultReward);
		rwd->addSpecificReward(GridWorldState(goalX,goalY),goalReward);
		rwd->addSpecificReward(GridWorldState(pitX,pitY),pitReward);
		return RewardConstPtrT(rwd);
	}

	virtual StateGeneratorConstPtrT getStateGenerator()const{
		return StateGeneratorConstPtrT(new GridWorldStateGenerator(gridX,gridY,blockX,blockY));
	}
	virtual ActionGeneratorConstPtrT getActionGenerator()const{
		return ActionGeneratorConstPtrT(new GridWorldActionGenerator());
	}
	virtual StateT getStartState()const{
		return GridWorldState(0,0);
	}
	/**
	 * Uses a pre-known transition table to transfer the state in a probablistic manner
	 */
	virtual StateT transferState(const StateT& currState, const ActionT& action){
		if (isTerminalState(currState)) return currState;

		//first, see in which state this action brings us, according to the transition
		//probabilities.
		//Get transition probabilities:
		typedef TransitionT::StateTransitionListT StateTransitionListT;
		std::shared_ptr<StateTransitionListT> possibleTransitions;
		if (!transition->getTransitionStates(currState, action, possibleTransitions) || possibleTransitions->empty()) {
			//PRINTMSG("transition from "<<currSquare<<" with action "<<currAction<<" is not possible.");
			return currState;
		}

		//now, we have to decide which target state to pick, and it must be according
		//to the probabilities.
		//random number [0..1] for destination state to pick: pick the first one 
		//one which causes the cumulation of all probabilites to exceed pRange.
		float pRange = (float)(RAND_MAX-RandomNumberGenerator::random()) / (float)RAND_MAX;
		//PRINTMSG("probability range: "<<pRange);
		StateTransitionListT::iterator sit;	
		float cumProb=0; //cumulated probabilities
		StateT newState;
		for (sit=possibleTransitions->begin(); sit!=possibleTransitions->end(); ++sit){
			newState=sit->s; //assume this state and maybe stick to it
			cumProb+=sit->p;
			//PRINTMSG("   considering "<<sit->s<<" with p="<<sit->p<<", cumP="<<cumProb<<" (pRange="<<pRange<<")");
			if (cumProb>=pRange) { //pick this state and stop checking the others
				break;
			}
		}
		return newState;
	}
	
	virtual bool isTerminalState(const StateT& s)const{
		if ((s.x==goalX) && (s.y==goalY)) return true;
		if ((s.x==pitX) && (s.y==pitY)) return true;

		//use < operators, because it's not guaranteed that the == operator is implemented
		/*StateT goal(goalX,goalY);
		if (!(s<goal) && !(goal<s)) return true;
		StateT pit(pitX,pitY);
		if (!(s<pit) && !(pit<s)) return true;*/

		return false;
	}
private:
	unsigned int gridX, gridY; //dimensions of grid
	unsigned int goalX, goalY; //goal coordinates (starting with index 0)
	unsigned int blockX, blockY; //coordinates of block (starting with index 0)
	unsigned int pitX, pitY; //coordinates of pit (starting with index 0)


	float defaultReward;
	float goalReward;
	float pitReward;

	TransitionPtrT transition;
};


}

#endif
