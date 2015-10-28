#ifndef _FLOATCOMPARISON_H_
#define _FLOATCOMPARISON_H_

#include <math.h>

//according to http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
template<typename FloatingT>
bool equalFloatsRelative(FloatingT A, FloatingT B, FloatingT maxAbsoluteError, FloatingT maxRelativeError){

     if (A==B) return true; //trivial case

    //from realtime collision detection: one expression only!
    //if (Abs(x - y) <= Max(absTol, relTol * Max(Abs(x), Abs(y))))
    //see also http://realtimecollisiondetection.net/blog/?p=89#more-89
    FloatingT diff=A-B;

    if (fabs(diff) < maxAbsoluteError)
        return true;

    FloatingT relativeError;
    if (fabs(B) > fabs(A))
        relativeError = fabs(diff / B);
    else
        relativeError = fabs(diff / A);
    return (relativeError <= maxRelativeError);
}


/**
 * simple absolute error calculation
 * Use equalFloatsSQR if you pass A and B as squared values
 * of their original value! 
 * The reason is, that the equation A-B <= epsilon cannot
 * be simplified to sqr(A)-sqr(B)<=sqr(epsilon), but would
 * mathematically be sqr(A-B)<=sqr(epsilon) which
 * would only be equivalent to sqr(A)-2AB + sqr(B) <= sqr(epsilon),
 * and which still needs the real values of A and B.
 */
template<typename FloatingT>
inline bool equalFloats(FloatingT A, FloatingT B, FloatingT maxAbsoluteError){
    return fabs(A-B) < maxAbsoluteError;
}

//simple absolute error calculation when having only the
//squared value of the real A and B values available.
//This will be less efficient than equalFloats() function, as the square
//root has to be taken, so if you need the real values of A and B somewhere else, 
//it is better to calculate them right away
/*template<typename FloatingT>
inline bool equalFloatsSQR(FloatingT A, FloatingT B, FloatingT maxAbsoluteError){
    return equalFloats(sqrt(A),sqrt(B),maxAbsoluteError);
}*/



template<typename FloatingT>
inline bool zeroFloat(FloatingT A, FloatingT maxAbsoluteError){
    return fabs(A) < maxAbsoluteError;
}


#endif
