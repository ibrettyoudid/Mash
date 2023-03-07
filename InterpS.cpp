// Copyright (c) 2023 Brett Curtis

#include "stdafx.h"
#include "classes.h"
#include "Parser.h"
#include "Parser2.h"
#include "TimerFrame.h"
#include "InterpS.h"

using namespace std;

Multimethod<Any> eval;

namespace Struct
{
   Any evalLiteral(Literal* expr, Stack* env)
   {
      return expr->value;
   }

   Any evalIdentifier(Identifier* ident, Stack* stack)
   {
      Member* m = (*stack->frame.typeInfo)[ident->span.str()];
      if (m)
         return stack->frame.member(m);
      if (stack->parent)
         return evalIdentifier(ident, stack->parent);
      throw "IDENTIFIER NOT FOUND";
   }

   Any evalLambda(Lambda* lambda, Stack* stack)
   {
      return new Closure(lambda, stack);
   }

   Any evalApply(Apply* apply, Stack* stack)
   {
      int64_t argCount = apply->elems.size();
      Any* argVals = new Any[argCount];
      const Any** argPtrs = new const Any*[argCount];
      for (int64_t ai = 0; ai < argCount; ++ai)
      {
         argVals[ai] = eval(apply->elems[ai], stack);
         argPtrs[ai] = &argVals[ai];
      }
      return argVals[0].call(argPtrs + 1, argCount - 1);
   }
}