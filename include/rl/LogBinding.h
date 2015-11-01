#ifndef __LOGBINDING_H__
#define __LOGBINDING_H__
// Copyright Jennifer Buehler

#include <iostream>
#include <sstream>

/**
 * \brief Class to bind to a certain type of logging.
 * \author Jennifer Buehler
 * \date May 2011
 */
class Log
{
public:
    static void print(const std::stringstream& str)
    {
        if (!Singleton) std::cerr << "Initialise Log Singleton!!" << std::endl;
        else Singleton->implPrint(str);
    }
    static void printError(const std::stringstream& str)
    {
        if (!Singleton) std::cerr << "Initialise Log Singleton!!" << std::endl;
        else Singleton->implPrintError(str);
    }
    static void print(const char * str)
    {
        if (!Singleton) std::cerr << "Initialise Log Singleton!!" << std::endl;
        else Singleton->implPrint(str);
    }
    static void printError(const char * str)
    {
        if (!Singleton) std::cerr << "Initialise Log Singleton!!" << std::endl;
        else Singleton->implPrintError(str);
    }
    static void printLn(const std::stringstream& str)
    {
        if (!Singleton) std::cerr << "Initialise Log Singleton!!" << std::endl;
        else
        {
            Singleton->implPrint(str);
            Singleton->implPrint("\n");
        }
    }
    static void printErrorLn(const std::stringstream& str)
    {
        if (!Singleton) std::cerr << "Initialise Log Singleton!!" << std::endl;
        else
        {
            Singleton->implPrintError(str);
            Singleton->implPrint("\n");
        }
    }
    static void printLn(const char * str)
    {
        if (!Singleton) std::cerr << "Initialise Log Singleton!!" << std::endl;
        else
        {
            Singleton->implPrint(str);
            Singleton->implPrint("\n");
        }
    }
    static void printErrorLn(const char * str)
    {
        if (!Singleton) std::cerr << "Initialise Log Singleton!!" << std::endl;
        else
        {
            Singleton->implPrintError(str);
            Singleton->implPrintError("\n");
        }
    }


    static std::shared_ptr<Log> Singleton;

protected:
    virtual void implPrint(const std::stringstream& str) = 0;
    virtual void implPrintError(const std::stringstream& str) = 0;
    virtual void implPrint(const char * str) = 0;
    virtual void implPrintError(const char * str) = 0;

};

std::shared_ptr<Log> Log::Singleton(NULL);


class StdLog: public Log
{
    virtual void implPrint(const std::stringstream& str)
    {
        std::cout << str.str();
    }
    virtual void implPrintError(const std::stringstream& str)
    {
        std::cerr << str.str();
    }
    virtual void implPrint(const char* str)
    {
        std::cout << str;
    }
    virtual void implPrintError(const char* str)
    {
        std::cerr << str;
    }

};

#define PRINT_INIT() {\
    if (Log::Singleton) {\
        std::cerr<<"Singleton already set, overwriting!"<<std::endl;\
    }\
    Log::Singleton=std::shared_ptr<Log>(new StdLog()); \
}

#define PRINTMSG(msg) {\
    std::stringstream _str_; \
    _str_<<msg<<" - "<< __FILE__<<", "<<__LINE__; \
    Log::printLn(_str_); \
}


#define PRINTERROR(msg) {\
    std::stringstream _str_; \
    _str_<<"ERROR: "<<msg<<" - "<< __FILE__<<", "<<__LINE__; \
    Log::printLn(_str_); \
}



#endif
