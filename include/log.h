/* Arbitrary Navn Tool -- Logging Prototype
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#pragma once
#ifndef LOG_H
#define LOG_H
#include "includes.h"

class Log
{
protected:
    std::fstream log;
public:
    LogType type;
    Flux::string filename;
    User *u;
    Command *c;
    std::stringstream buffer;
    std::stringstream logstream;
    static Flux::string TimeStamp();
    Log(LogType type = LOG_NORMAL);
    Log(LogType, User*);
    Log(User*);
    Log(User*, Command*);
    Log(LogType, User*, Command*);
    ~Log();

    template<typename T> Log &operator<<(T val)
    {
	this->buffer << val;
	return *this;
    }
};

#endif