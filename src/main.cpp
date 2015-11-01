/*
 * \author Jennifer Buehler
 * \copyright Jennifer Buehler, GPL
 */

#include <rl/ValueIteration.h>
#include <rl/PolicyIteration.h>
#include <rl/LogBinding.h>
#include <rl/GridWorld.h>
#include <rl/Utility.h>
#include <rl/QLearning.h>

#include <string>

using rl::ValueIterationController;
using rl::PolicyIterationController;
using rl::QLearningController;
using rl::GridDomain;
using rl::LearningController;
using rl::Exploration;
using rl::SimpleExploration;
using rl::NoExploration;
using rl::LearningRate;
using rl::DecayLearningRate;

/**
 * \param useAlgorithm 0 for value iteration, 1 for policy iteration, 2 for q-learning
 */
int testGridWorldLearning(unsigned int useAlgorithm)
{

    //### 1. Initialise grid world
    unsigned int gridX = 4;
    unsigned int gridY = 3;

    unsigned int goalX = 3;
    unsigned int goalY = 2;

    unsigned int blockX = 1;
    unsigned int blockY = 1;

    unsigned int pitX = 3;
    unsigned int pitY = 1;

    float sideActionP = 0.1;
    float defaultReward = -0.04;
    float goalReward = 1;
    float pitReward = -1;

    PRINTMSG("Initialising gridworld");
    GridDomain::GridDomainPtrT gridWorld(new GridDomain(gridX, gridY, goalX, goalY, 
                                         blockX, blockY, pitX, pitY, defaultReward, 
                                         goalReward, pitReward, sideActionP));

    typedef GridDomain::StateT StateT;
    typedef GridDomain::ActionT ActionT;


    //### 2. initialise learning controller
    typedef LearningController<GridDomain, float> LearningControllerT;
    typedef typename LearningControllerT::LearningControllerPtrT LearningControllerPtrT;

    LearningControllerPtrT learningController;
    switch (useAlgorithm)
    {
    case 0:   //value iteration
    {
        PRINTMSG("Using value iteration");
        float defaultUtility = 0;
        float discount = 1.0;
        float maxErr = 0.01;
        typedef ValueIterationController<GridDomain> ValueIterationControllerT;
        learningController = LearningControllerPtrT(new ValueIterationControllerT(gridWorld, defaultUtility, discount, maxErr));
        break;
    }
    case 1:   //policy iteration
    {
        PRINTMSG("Using policy iteration");
        float defaultUtility = 0;
        float discount = 1.0;
        typedef PolicyIterationController<GridDomain> PolicyIterationControllerT;
        learningController =  LearningControllerPtrT(new PolicyIterationControllerT(gridWorld, defaultUtility, discount));
        break;
    }
    case 2:   //q-learning
    {
        PRINTMSG("Using q learning");
        typedef Exploration<float, unsigned int> ExplorationT;
        typedef SimpleExploration<float, unsigned int> SimpleExplorationT;
        typedef NoExploration<float, unsigned int> NoExplorationT;

        typedef typename ExplorationT::ExplorationPtrT ExplorationPtrT;
        typedef typename LearningRate::LearningRatePtrT LearningRatePtrT;

        float discount = 1.0;
        float defaultQ = 0.0;
        float epsilonGreedy = 0.1;
        float decay = 0.1;
        LearningRatePtrT learnRate(new DecayLearningRate(decay));
        typedef QLearningController<GridDomain> QLearningControllerT;

        unsigned int freqThreshold = 20; //minimum number of times an action is tried from a state
        //(that is, if this state is visited at all, and there is enough iterations
        // to allow for so many trials)
        ExplorationPtrT explore(new SimpleExplorationT(freqThreshold, gridWorld->getReward()->getOptimisticReward()));
        //ExplorationPtrT explore(new NoExplorationT());

        learningController = LearningControllerPtrT(new QLearningControllerT(gridWorld, learnRate, discount, defaultQ, explore, epsilonGreedy));
        break;
    }
    }

    PRINTMSG("Initialising controller");

    StateT currState = gridWorld->getStartState();  //initialise current state
    learningController->initialize(gridWorld->getStartState());

    PRINTMSG("Initialized.");

    unsigned int numTrials = 10000; //number of iterations for online learning algorithms which can't determine whether it's learned
    int learned = 0;
    unsigned int i = 0; //iteration count
    unsigned int doneTrials = 0;
    bool stop = false;
    //In this simple example, we iterate only if the learning has not converged yet, or if the
    //LearningController implementation does not know when the function is learned (this would be an online
    //learner then. Here we will assume a fixed number of iterations for those online learners. See
    //LearningController documentation!).
    // --  NOTE -- we can always continue this loop if we want to simulate the world by transferring from
    //one state to the next! This loop also serves as simulation, simultaneously to the learning!
    //We only use the fixed number of iterations for online learners (see LearningController documentation!)
    //because we don't want a simulation in this simple example.
    while (((learned = learningController->finishedLearning()) == 1) || ((learned == 0) && (doneTrials < numTrials)))
    {
        ++i;
        //PRINTMSG("--- At state "<<currState);
        ActionT currAction = learningController->updateAndGetAction(currState);
        if (gridWorld->isTerminalState(currState))  //XXX do we need this?? The same state should be returned!
        {
            ++doneTrials;
            //PRINTMSG("TEST: Reached terminal state "<<currState<<" after "<<i<<" iterations");
            //currState=gridWorld->getStartState(); //go back to start state
            while (gridWorld->isTerminalState(currState))
            {
                currState = gridWorld->getStateGenerator()->randomState();
            }
            learningController->resetStartState(currState);
            /*static int terminalCnt=0;
            ++terminalCnt;
            if (terminalCnt==10000) { PRINTMSG("Force abort"); stop=true; }
            else continue;*/
        }
        //PRINTMSG("... will perform action "<<currAction);
        currState = gridWorld->transferState(currState, currAction);
        if (stop) break;
    }

    PRINTMSG("RESULT:");
    std::stringstream strng;
    learningController->printValues(strng);
    PRINTMSG(strng.str());
    if (useAlgorithm == 2) PRINTMSG("Number of trials: " << doneTrials << " of " << numTrials << " max. " << i << " iterations altogether");
    return 0;
}


void printHelp(const char*argv0)
{
    PRINTMSG("Usage: " << argv0 << " --value-iteration | --policy-iteration | --q-learning");
}


int main(int argc, char **argv)
{
    PRINT_INIT();
    if (argc < 2)
    {
        PRINTERROR("Not enough arguments");
        printHelp(argv[0]);
        return 1;
    }

    /**********************+
    * LEARNING TEST
    * pass 0 for value iteration,
    * 1 for policy iteration, and
    * 2 for q-learning
    **********************/
    int type = 0;

    if (std::string(argv[1]) == "--value-iteration")
    {
        type = 0;
    }
    else if (std::string(argv[1]) == "--policy-iteration")
    {
        type = 1;
    }
    else if (std::string(argv[1]) == "--q-learning")
    {
        type = 2;
    }

    PRINTMSG("Running test on learning type=" << type);
    return testGridWorldLearning(type);
}
