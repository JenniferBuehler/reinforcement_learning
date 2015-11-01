#ifndef RL_ACTION_H
#define RL_ACTION_H
// Copyright Jennifer Buehler

#include <iostream>


namespace rl
{

/**
 * \brief Base class for all action types to be performed. This must
 * include ALL possible actions in a domain within the SAME
 * implementing subclass.
 *
 * Example for the grid world: A derived class can contain all
 * actions up/down/left/right.
 *
 * All subclasses should have the copy constructor and assignment
 * operators implemented (using the same class type as paramter
 * for the assignment operator).
 *
 * \author Jennifer Buehler
 * \date May 2011
 *
 * Inheriting classes MUST support the copy operator!
 */
class ActionBase
{
public:
    typedef unsigned int SubActionHandleT;

    ActionBase() {}
    ActionBase(const ActionBase& o) {}
    virtual ~ActionBase() {}

    ActionBase& operator=(const ActionBase& o)
    {
        if (this == &o) return *this;
        assign(o);
        return *this;
    }
    bool operator < (const ActionBase& s) const
    {
        return less(s);
    }
    friend std::ostream& operator<<(std::ostream& o, const ActionBase& s)
    {
        s.print(o);
        return o;
    }

protected:
    /**
     * Assign another object of the same type (to work as a copy constructor).
     * This will need a dynamic_cast in the subclass. If it's not of the
     * same type or can't be assigned, an exception could be thrown or a warning
     * printed.
     * It is also stronly recommended to implement a = operator for all subclasses,
     * for efficiency reasons when using templates. This method is only supplied
     * to enforce consistency. It will be called from this base class = operator.
     */
    virtual void assign(const ActionBase& o) = 0;

    virtual bool less(const ActionBase& s) const = 0;

    /**
     * print description of the state into ostream o, and return o.
     */
    virtual void print(std::ostream& o) const = 0;
};

}  // namespace rl
#endif  //  RL_ACTION_H

