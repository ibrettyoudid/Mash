// Copyright (c) 2023 Brett Curtis
#pragma once
#include "any.h"
#include "classes.h"

namespace List
{

Any eval(Any expr, Frame* env);
Any apply(Any func, Vec* args);
void compile(Any& expr, Frame* env);

struct InterpL
{
   Frame*   stack;
   Cons*    results;

   InterpL  () : stack(nullptr), results(nullptr) {}


   void     push     (Frame* newFrame);
   void     push     (Any expr, Frame* env);
   void     push     (Vec* todo, Frame* env);
   Frame*   pop      ();
   void     pushR    (Any val);
   Any      popR     ();
   void     evalStep ();
   void     evalStep1();
   Any      eval     (Any expr, Frame* env);
   void     apply    (Any func, Vec* a);

};

}