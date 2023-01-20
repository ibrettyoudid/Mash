#pragma once

#include <set>

#include "Parser2.h"

struct State
{
   Rule* rule;
   int   dot;
   int   origin;

   State() {}
   State(Rule* rule, int dot, int origin) : rule(rule), dot(dot), origin(origin) {}

   bool  finished();
   Rule *next();
   //bool  terminal;
};

typedef std::set   <State> StateSet;
typedef std::vector<State> StateVec;

typedef char PEChar;

struct Word
{
   StateSet stateSet;
   StateVec stateVec;
   PEChar   pechar;

   Word(PEChar pechar) : pechar(pechar) {}

   void insert(State state);
};

typedef std::vector<Word> Words;
