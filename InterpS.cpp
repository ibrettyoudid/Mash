// Copyright (c) 2023 Brett Curtis

#include "stdafx.h"
#include "classes.h"
#include "Parser.h"
#include "Parser2.h"
#include "TimerFrame.h"
#include "InterpS.h"

using namespace std;

namespace Struct
{
   Stack* stack;

   Multimethod<Any>  eval   ("eval");
   Multimethod<void> compile("compile");

   Any evalLiteral(Literal* expr, Stack* env)
   {
      return expr->value;
   }

   Any evalIdentifier(Identifier* ident, Stack* stack)
   {
      TimerFrameD t("evalIdentifier", toText(ident)+" "+toText(stack));
      Member* m = stack->frame.findMember(ident->span.str());
      if (m)
         return t.exit(stack->frame.getMemberRef(m));
      if (stack->parent)
         return t.exit(evalIdentifier(ident, stack->parent));
      throw "IDENTIFIER NOT FOUND";
   }

   Any evalLambda(Lambda* lambda, Stack* stack)
   {
      TimerFrameD t("evalLambda", toText(lambda)+" "+toText(stack));
      return t.exit(new Closure(lambda, stack));
   }

   Any evalApply(Apply* apply, Stack* stack)
   {
      TimerFrameD t("evalApply", toText(apply)+" "+toText(stack));
      int64_t argCount = apply->elems.size();
      Any* argVals = new Any[argCount];
      for (int64_t ai = 0; ai < argCount; ++ai)
      {
         argVals[ai] = eval(apply->elems[ai], stack);
      }
      return t.exit(argVals[0].call(argVals + 1, argCount - 1));
   }

   Any evalWhiteSpaceE(WhiteSpaceE* whiteSpaceE, Stack* stack)
   {
      return eval(whiteSpaceE->inner, stack);
   }

   Any applyAnyClosure(Any* anyClosure, Any* args, int64_t nargs)
   {
      return applyClosure(*anyClosure, args, nargs);
   }

   Any applyClosure(Closure* closure, Any* args, int64_t nargs)
   {
      Stack* stack = new Stack;
      stack->parent = closure->context;
      Any& sf(stack->frame);
      sf.typeInfo = closure->lambda->type;
      sf.payload  = new char[sf.typeInfo->size];
      for (int64_t ai = 0; ai < nargs; ++ai)
         sf.typeInfo->copyCon(sf.getMemberAddress(sf.findMember(ai)), &args[ai], sf.typeInfo);
         //sf.typeInfo->copyCon(sf.getMemberAddress(sf.findMember(ai)), args[ai].payload, sf.typeInfo);
      return eval(closure->lambda->body, stack);
   }

   void compileLiteral(Literal* literal, Stack*)
   {

   }

   void compileIdentifier(Identifier* ident, Stack* stack)
   {
      while (stack)
      {
         ident->member = stack->frame.findMember(ident->span.str());
         if (ident->member) return;
         stack = stack->parent;
      }
      throw "IDENTIFIER NOT FOUND";
   }

   void compileLambda(Lambda* lambda, Stack* stack)
   {
      lambda->type->allocate();
      Stack* stackNew = new Stack;
      stackNew->parent = stack;
      stackNew->frame.typeInfo = lambda->type;
      compile(lambda->body, stackNew);
      delete stackNew;
   }

   void compileApply(Apply* apply, Stack* stack)
   {
      for (auto e : apply->elems)
         compile(e, stack);
   }  

   void compileWhiteSpaceE(WhiteSpaceE* whiteSpaceE, Stack* stack)
   {
      compile(whiteSpaceE->inner, stack);
   }



   void setupInterp()
   {
      stack = new Stack;
      stack->frame = Struct::globalFrame;
      stack->parent = nullptr;

      eval.add(evalLiteral);
      eval.add(evalIdentifier);
      eval.add(evalLambda);
      eval.add(evalApply);
      eval.add(evalWhiteSpaceE);

      compile.add(compileLiteral);
      compile.add(compileIdentifier);
      compile.add(compileLambda);
      compile.add(compileApply);
      compile.add(compileWhiteSpaceE);
   }
}