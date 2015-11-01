#ifndef MATH_RANDOMNUMBER_H
#define MATH_RANDOMNUMBER_H
// Copyright Jennifer Buehler

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * Interface to random number generator, as defined
 * by srand() and rand() functions.
 * srand(time(NULL)) is called in constructor, and function
 * rand() behaves as the c rand() function.
 */
class RandomNumberGenerator
{
public:
    RandomNumberGenerator()
    {
        srand(time(NULL));
    }
    ~RandomNumberGenerator() {}
    static int random()
    {
        return rand();
    }
};


/**
 * static interface to use for random number generation.
 * This makes sure the constructor of RandomNumberGenerator
 * will be called at the beginning of runtime. However,
 * it can also be used as an interface.
 */
extern RandomNumberGenerator _randomNumber;

#endif  // MATH_RANDOMNUMBER_H

