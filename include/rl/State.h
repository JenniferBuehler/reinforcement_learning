#ifndef __STATE_H__
#define __STATE_H__
// Copyright Jennifer Buehler

#include <iostream>
#include <memory>

namespace rl
{

/**
 * \brief Base class for all states possible in a domain. This must
 * include ALL possible states in a domain within the SAME
 * implementing subclass.
 *
 * Example for the 2D grid world: A derived class can contain
 * 2D coordinates of the position on the grid..
 *
 * All subclasses should have the copy constructor and assignment
 * operators implemented (using the same class type as paramter
 * for the assignment operator).
 * \author Jennifer Buehler
 * \date May 2011
 */
class StateBase
{
public:

    StateBase() {}
    StateBase(const StateBase& o) {}
    virtual ~StateBase() {}

    typedef std::shared_ptr<StateBase> StateBasePtrT;
    typedef std::shared_ptr<const StateBase> const StateBaseConstPtrT;

    bool operator < (const StateBase& s) const
    {
        return less(s);
    }
    friend std::ostream& operator<<(std::ostream& o, const StateBase& s)
    {
        s.print(o);
        return o;
    }

    StateBase& operator=(const StateBase& o)
    {
        if (this == &o) return *this;
        assign(o);
        return *this;
    }


protected:

    virtual bool less(const StateBase& s) const = 0;

    /**
     * print description of the state into ostream o, and return o.
     */
    virtual void print(std::ostream& o) const = 0;

    /**
     * Assign another object of the same type (to work as a copy constructor).
     * This will need a dynamic_cast in the subclass. If it's not of the
     * same type or can't be assigned, an exception could be thrown or a warning
     * printed.
     * It is also stronly recommended to implement a = operator for all subclasses,
     * for efficiency reasons when using templates. This method is only supplied
     * to enforce consistency. It will be called from this base class = operator.
     */
    virtual void assign(const StateBase& o) = 0;
private:

};


}
#endif

