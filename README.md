# Reinforcement learning

MDP and Q-Learning algorithms developed during a project at UNSW in 2011.

# Documentation

More documentation can be found in the [PDF](../../Documentation.pdf) and in the html files in the *doc* folder.


# Installation

``mkdir build``

``cd build``

``cmake ..``

``make``


# Execution

To execute the test on the grid world, run

``./demoGridWorldDemo --value-iteration | --poliy-iteration | --q-learning``

# Note

The source code is mainly contained in the header files at the moment, partly contaning several classes per header file. 
This is partly because the use of templates limits the possibilities to put code in cpp files, and partly because I find it easier to work with.

