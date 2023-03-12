#pragma once

#include "any.h"

namespace Struct
{

   extern Multimethod<Any> eval;
   extern Multimethod<void> compile;

   Any applyClosure(Closure* closure, Any* args, int64_t nargs);
      
   void setupInterp();

}