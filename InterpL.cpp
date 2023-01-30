// Copyright (c) 2023 Brett Curtis
// main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "InterpL.h"
#include "classes.h"
#include "Parser.h"
#include "Parser2.h"
#include "TimerFrame.h"

using namespace std;

void testDeque()
{
   deque<int> d;
   d.push_back(1);

   int* dp = &d[0];

   for (int i = 0; i < 10000; ++i) d.push_back(i);

   if (dp == &d[0])
      cout << "deque iterators are valid" << endl;
   else
      cout << "DEQUE ITERATORS ARE NOT VALID" << endl;
}

namespace List
{

Any evalMulti(Any expr, Frame* env);

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


void InterpL::push(Frame* newFrame)
{
   newFrame->returnFrame = stack;
   stack = newFrame;
}

void InterpL::push(Any expr, Frame* env)
{
   push(new Frame(expr, env));
}

void InterpL::push(Vec* todo, Frame* env)
{
   for (int i = todo->size() - 1; i >= 0; --i)
      push((*todo)[i], env);
}

Frame* InterpL::pop()
{
   Frame* result = stack;
   stack = stack->returnFrame;
   return result;
}

void InterpL::pushR(Any val)
{
   results = cons(val, results);
}

Any InterpL::popR()
{
   Any result = results->car;
   results = results->cdr;
   return result;
}

Any InterpL::eval(Any expr, Frame* env)
{
   Frame* st = stack;
   push(expr, env);
   while (stack != st)
      evalStep();
   return popR();
}

void InterpL::evalStep()
{
   string args(TS+"expr="+toText(stack->prog));
   Frame* env = stack->context;
   if (env != globalFrame)
      args += TS+" env="+toText(env);
   TimerFrameD tf("eval", args);
   evalStep1();
   tf.exit(toText(results));
}

void InterpL::evalStep1()
{
   Frame* top  = pop();
   Any    expr = top->prog;
   Frame* env  = top->context;

   if (expr.typeInfo == tiSymbol)
   {
      string symText  = symbolText(expr);
      if (symText == "_") { pushR(symBlankArg); return; }
      
      for (Frame* lookIn = env; lookIn; lookIn = lookIn->context)
      {
         for (DequeVar::iterator v = lookIn->vars.begin(); v != lookIn->vars.end(); ++v)
         {
            if (v->name == symText)
            {
               pushR(v->value);
               return;
            }
         }
      }
      throw "variable not found";
   }
   else if (expr.typeInfo == tiCons)
   {
      string headText;
      Any head = car(expr);
      if (head.typeInfo == tiSymbol)
      {
         switch (static_cast<Keyword>(head.as<Symbol>().id))
         {
            case Keyword::list:
               pushR(cdr(expr));
               return;
            case Keyword::_if:
               push(cons(getSymbol("if1"), drop2(expr)), env);
               push(second(expr), env);
               return;
            case Keyword::_if1:
               if (popR().as<bool>())
                  push(third(expr), env);
               else
                  push(fourth(expr), env);
               return;
            case Keyword::let:
            {
               push(cons(getSymbol("let1"), drop1(expr)), env);
               Any name = second(expr);
               if (name.typeInfo == tiSymbol)
               {
                  expr = cdr(expr);
               }
               Cons* defs = second(expr);
               Cons* body = drop2(expr);
               defs = reverse(defs);
               while (defs != nil)
               {
                  Cons* def = car(defs);
                  push(second(def), env);
                  defs = cdr(defs);
               }
               return;
            }
            case Keyword::let1:
            {
               Any name = second(expr);
               if (name.typeInfo == tiSymbol)
               {
                  expr = cdr(expr);
               }
               Cons* defs = second(expr);
               Cons* body = drop2(expr);

               Frame* newEnv = new Frame;
               newEnv->context  = env;
               while (defs != nil)
               {
                  Cons* def = car(defs);
                  newEnv->vars.push_back(Var(toText(car(def)), popR()));
                  defs = cdr(defs);
               }
               push(body, newEnv);
               return;
            }
            case Keyword::define:
               push(mylist(getSymbol("define1"), second(expr)), env);
               push(third(expr), env);
               return;
            case Keyword::define1:
            {
               Any val = popR();
               env->vars.push_back(Var(symbolText(second(expr)), val));
               pushR(val);
               return;
            }
            case Keyword::lambda:
            {
               Closure* newlam  = new Closure;
               Any     params  = second(expr);
               newlam->minArgs = 0;
               newlam->body    = drop2(expr);
               newlam->context = env;
               while (!isNil(params))
               {
                  newlam->params.push_back(Var(symbolText(car(params))));
                  ++newlam->minArgs;
                  params = cdr(params);
               }
               if (params.typeInfo == tiSymbol)
               {
                  newlam->maxArgs = 1000000000;
                  newlam->rest    = symbolText(params);
               }
               else
                  newlam->maxArgs = newlam->minArgs;
               pushR(newlam);
               return;
            }
            case Keyword::apply1:
            {
               int nArgs = second(expr);
               --nArgs;
               Any func = popR();

               Vec args;
               for (int i = 0; i < nArgs; ++i)
                  args.push_back(popR());
               apply(func, &args);
               return;
            }
            case Keyword::applyTo:
            {
               Any   func = popR();
               Cons* argL = drop1(expr);
               Vec   args;
               while (argL)
               {
                  args.push_back(car(argL));
                  argL = cdr(argL);
               }
               apply(func, &args);
               return;
            }
            case Keyword::begin:
            {
               Vec todo;
               Cons* body = drop1(expr);
               while (body)
               {
                  todo.push_back(car(body));
                  todo.push_back(mylist(getSymbol("pop")));
                  body = cdr(body);
               }
               todo.pop_back();
               push(&todo, env);
               return;
            }
            case Keyword::pop:
               popR();
               return;
            case Keyword::callCC:
               pushR(*this);
               push(second(expr), env);
         }
      }

      push(mylist(getSymbol("apply1"), lengthCons(expr)), env);
      while (!isNil(expr))
      {
         push(car(expr), env);
         expr = cdr(expr);
      }
   }
   else
      pushR(expr);
}

void InterpL::apply(Any func, Vec* a)
{
   Vec&     args(*a);
   int      maxArgs;
   int      minArgs;
   int      nArgs;
   Closure*  lambda;
   bool     blankArgs;
   bool     canApply;

   nArgs  = args.size();
   lambda = nullptr;
   if (func.typeInfo == tiListClosure)
   {
      lambda  = func;
      minArgs = lambda->minArgs;
      maxArgs = lambda->maxArgs;
   }
   else if (func.typeInfo->multimethod)
   {
      MMBase& mm = (MMBase&)func;
      maxArgs = mm.maxArgs;
      minArgs = mm.minArgs;
   }
   else if (func.typeInfo->kind == kFunction)
   {
      maxArgs = minArgs = func.typeInfo->args.size();
   }
   else
   {
      cout << "attempt to apply " << toText(func) << endl;
      throw "attempt to apply wrong type";
   }

   blankArgs = false;
   canApply  = nArgs >= minArgs;
   for (int i = 0; i < nArgs; ++i)
      if (isBlank(args[i]))
      {
         blankArgs = true;
         if (i < maxArgs)
            canApply = false;
         break;
      }
   if (canApply)
   {
      if (nArgs > maxArgs)
      {
         Cons* rest = nil;
         for (int i = nArgs - 1; i >= maxArgs; --i)
            rest = cons(args[i], rest);
         push(cons(getSymbol("applyTo"), rest), nullptr);
      }

      if (func.typeInfo == tiListClosure)
      {
         Closure* lambda = func;

         Frame* newenv = new Frame;
         newenv->context  = lambda->context;

         int na = args.size();
         int np = lambda->params.size();
         for (int i = 0; i < na; ++i)
            newenv->vars.push_back(Var(lambda->params[i].name, args[i]));

         push(cons(getSymbol("begin"), lambda->body), newenv);
      }
      else
         pushR(func.call(&*args.begin(), min(nArgs, maxArgs)));
      return;
   }

   Closure* result = new Closure;  //lambda for missing args
   Frame*  newEnv = new Frame;   //frame for existing args
   if (lambda)
   {
      newEnv->context = lambda->context;

      for (int i = 0; i < nArgs; ++i)
         if (isBlank(args[i]))
            result->params.push_back(lambda->params[i]);
         else                                                        
            newEnv->vars.push_back(Var(lambda->params[i].name, args[i]));

      result->minArgs  = result->maxArgs = result->params.size();
      result->body       = lambda->body;
      result->context = newEnv;
   }
   else
   {
      Cons*   resArgList = nil;

      for (int i = 0; i < nArgs; ++i)
      {
         if (isBlank(args[i]))
            result->params.push_back(Var(string("arg")+i));
         //else                                                        
            //newEnv->vars.push_back(Var(string("arg")+i, args[i]));
      }
      for (int i = nArgs - 1; i >= 0; --i)
      {
         if (isBlank(args[i]))
            resArgList = cons(getSymbol(string("arg")+i), resArgList);
         else
            resArgList = cons(args[i], resArgList);
      }
      result->minArgs = result->maxArgs = result->params.size();
      result->body    = mylist(cons(func, resArgList));
      result->context = newEnv;
   }
   pushR(result);
}

void compile(Any& expr, Frame* env)
{
   if (expr.typeInfo == tiSymbol)
   {
      Symbol* sym = expr;
      if (sym->name == "_") return; //symBlankArg;

      int frameC = 0;
      for (Frame* lookIn = env; lookIn; lookIn = lookIn->context, ++frameC)
      {
         int varC = 0;
         for (DequeVar::iterator v = lookIn->vars.begin(); v != lookIn->vars.end(); ++v, ++varC)
            if (sym->name == v->name) 
            {
               expr = new VarRef(sym->name, frameC, varC);
               return;   //expr
            }
      }
      throw "variable not found";
   }
   else if (expr.typeInfo == tiCons)
   {
      string headText;
      Any head = car(expr);
      if (head.typeInfo == tiSymbol)
      {
         headText = symbolText(head);
         switch (static_cast<Keyword>(head.as<Symbol>().id))
         {
            case Keyword::let:
            {
               Any name = second(expr);
               Any ex1 = expr;
               if (name.typeInfo == tiSymbol)
                  ex1 = cdr(ex1);

               Any  defs = second(ex1);
               Any& body = drop2(ex1);

               if (defs.typeInfo == tiCons)
               {
                  Frame* newenv = new Frame;
                  newenv->context  = env;

                  while (defs.typeInfo == tiCons)
                  {
                     Any def = car(defs);
                     newenv->vars.push_back(Var(toText(first(def)), eval(second(def), env)));
                     defs = cdr(defs);
                  }
                  compile(body, newenv);
                  return;
               }
               cout << "syntax error in let" << endl;
               throw "weird";

            }
            case Keyword::lambda:
            {
               Frame* newenv = new Frame;
               newenv->context = env;

               Any  params = second(expr);
               Any& body   = drop2 (expr);
               while (!isNil(params))
               {
                  newenv->vars.push_back(Var(symbolText(car(params))));
                  params = cdr(params);
               }
               if (params.typeInfo == tiSymbol)
                  newenv->vars.push_back(Var(symbolText(params)));
               compile(body, newenv);
               return;
            }
            case Keyword::define:
            {
               compile(third(expr), env);
               string symText = symbolText(second(expr));
               DequeVar::iterator v;
               for (v = env->vars.begin(); v != env->vars.end(); ++v)
                  if (v->name == symText) 
                     break;
               if (v == env->vars.end())
                  env->vars.push_back(Var(symText));
               return;
            }
            case Keyword::_if:
            {
               compile(second(expr), env);
               compile(third(expr), env);
            }
         }
      }
      //function call
      for (Any* p = &expr; !isNil(*p); p = &cdr(*p))
      {
         compile(car(*p), env);
      }
   }
}

Any eval1(Any expr, Frame* env)
{
   if (expr.typeInfo == tiVarRef)
   {
      VarRef* vr = expr;
      Frame* lookIn = env;
      for (int frameC = vr->frameC; frameC; --frameC)
         lookIn = lookIn->context;
      return lookIn->vars[vr->varC].value;
      //for (int varC = vr->varC; varC; --varC)
      //   v++;
      //return v->value;
   }
   else if (expr.typeInfo == tiSymbol)
   {
      string symText = symbolText(expr);
      if (symText == "_") return symBlankArg;

      for (Frame* lookIn = env; lookIn; lookIn = lookIn->context)
      {
         for (DequeVar::iterator v = lookIn->vars.begin(); v != lookIn->vars.end(); ++v)
            if (v->name == symText) return v->value;
      }
      throw "variable not found";
   }
   else if (expr.typeInfo == tiCons)
   {
      string headText;
      Any head = car(expr);
      if (head.typeInfo == tiSymbol)
      {
         headText = symbolText(head);
         switch (static_cast<Keyword>(head.as<Symbol>().id))
         {
            case Keyword::_if:
            {
               if (second(eval(expr, env)).as<bool>())
                  return eval(third(expr), env);
               else
                  return eval(fourth(expr), env);
            }
            case Keyword::let:
            {
               Any name = second(expr);
               Any defs;
               Any body;
               if (name.typeInfo == tiSymbol)
               {
                  expr = cdr(expr);
               }
               defs = second(expr);
               body = cdr(cdr(expr));
               if (expr.typeInfo == tiCons)
               {
                  Frame* newenv = new Frame;
                  newenv->context  = env;

                  while (defs.typeInfo == tiCons)
                  {
                     Any def = car(defs);
                     newenv->vars.push_back(Var(toText(car(def)), eval(car(cdr(def)), env)));
                     defs = cdr(defs);
                  }
                  return evalMulti(body, newenv);
               }
               cout << "syntax error in let" << endl;
               throw "weird";

            }
            case Keyword::lambda:
            {
               Closure* newclos = new Closure;
               Any      params  = second(expr);
               newclos->minArgs = 0;
               newclos->body    = cdr(cdr(expr));
               newclos->context = env;
               while (params.typeInfo == tiCons && !isNil(params))
               {
                  newclos->params.push_back(Var(symbolText(car(params))));
                  ++newclos->minArgs;
                  params = cdr(params);
               }
               if (params.typeInfo == tiSymbol)
               {
                  newclos->maxArgs = INT_MAX;
                  newclos->rest    = symbolText(params);
               }
               else
                  newclos->maxArgs = newclos->minArgs;
               return newclos;
            }
            case Keyword::define:
            {
               Any val = eval(third(expr), env);
               string symText = symbolText(second(expr));
               DequeVar::iterator v;
               for (v = env->vars.begin(); v != env->vars.end(); ++v)
                  if (v->name == symText) 
                     break;
               if (v == env->vars.end())
                  env->vars.push_back(Var(symText, val));
               else
                  v->value = val;
               return val;
            }
            default:
               if (headText == "quit" || headText == "exit")
                  exit(0);
         }
      }
      Any func = eval(head, env);

      Vec args;
      for (Any it = cdr(expr); !isNil(it); it = cdr(it))
         args.push_back(eval(car(it), env));
      return apply(func, &args);
   }
   else
      return expr;
}
Any eval(Any expr, Frame* env)
{
   string args(TS+"expr="+toText(expr));
   if (env != globalFrame)
      args += TS+" env="+toText(env);
   TimerFrameD tf("eval", args);
   Any result = eval1(expr, env);
   tf.exit(toText(result));
   return result;
}

Any evalMulti1(Any expr, Frame* env)
{
   Any result;
   while (!isNil(expr))
   {
      result = eval(car(expr), env);
      expr = cdr(expr);
   };
   return result;
}
Any evalMulti(Any expr, Frame* env)
{
   string args(TS+"expr="+toText(expr));
   if (env != globalFrame)
      args += TS+" env="+toText(env);
   TimerFrameD tf("evalMulti", args);
   Any result = evalMulti1(expr, env);
   tf.exit(toText(result));
   return result;
}

Closure* createLambda(Any func, int nArgs)
{
   Closure* lambda = new Closure;
   for (int i = 0; i < nArgs; ++i)
      lambda->params.push_back(Var(string("arg")+i));

   Cons* argList = nullptr;
   for (int i = nArgs - 1; i >= 0; --i)
      argList = cons(getSymbol(string("arg")+i), argList);

   lambda->body = cons(func, argList);
   return lambda;
}

Any apply1(Any func, Vec* a)
{
   Vec&     args(*a);
   int      maxArgs;
   int      minArgs;
   int      nArgs;
   Closure*  closure;
   bool     blankArgs;
   bool     canApply;

   for (;;)
   {
      nArgs   = args.size();
      closure  = nullptr;
      minArgs = func.minArgs();
      maxArgs = func.maxArgs();
      if (func.typeInfo == tiListClosure)
      {
         closure  = func;
      }
      else if (!func.callable())
      {
         cout << "attempt to apply " << toText(func) << endl;
         throw "attempt to apply wrong type";
      }

      blankArgs = false;
      canApply  = nArgs >= minArgs;
      for (int i = 0; i < nArgs; ++i)
         if (isBlank(args[i]))
         {
            blankArgs = true;
            if (i < maxArgs)
               canApply = false;
            break;
         }

      if (!canApply) break;

      if (closure)
      {
         Frame* newenv   = new Frame;
         newenv->context = closure->context;

         int na = args.size();
         int np = closure->params.size();
         for (int i = 0; i < na; ++i)
            newenv->vars.push_back(Var(closure->params[i].name, args[i]));

         func = evalMulti(closure->body, newenv);
      }
      else
         func = func.call(&*args.begin(), min(nArgs, maxArgs));

      if (nArgs <= maxArgs) return func;

      args.erase(args.begin(), args.begin() + maxArgs);
   }
   Closure* result = new Closure;  //lambda for missing args
   Frame*  newEnv = new Frame;   //frame for existing args
   if (closure)
   {
      newEnv->context = closure->context;

      for (int i = 0; i < nArgs; ++i)
         if (isBlank(args[i]))
            result->params.push_back(closure->params[i]);
         else                                                        
            newEnv->vars.push_back(Var(closure->params[i].name, args[i]));

      result->minArgs = result->maxArgs = result->params.size();
      result->body    = closure->body;
      result->context = newEnv;
      return result;
   }
   else
   {
      Cons*   resArgList = nil;

      for (int i = 0; i < nArgs; ++i)
      {
         if (isBlank(args[i]))
            result->params.push_back(Var(string("arg")+i));
         //else                                                        
            //newEnv->vars.push_back(Var(string("arg")+i, args[i]));
      }
      for (int i = nArgs - 1; i >= 0; --i)
      {
         if (isBlank(args[i]))
            resArgList = cons(getSymbol(string("arg")+i), resArgList);
         else
            resArgList = cons(args[i], resArgList);
      }
      result->minArgs = result->maxArgs = result->params.size();
      result->body    = cons(cons(func, resArgList), nil);
      result->context = newEnv;
      return result;
   }
}
/*
   else if (blankArgs)
   {
      if (!lambda) lambda = createLambda(func, nArgs);

      Frame*  newEnv = new Frame;   //frame for existing args
      Closure* result = new Closure;  //lambda for blank args

      for (int i = 0; i < nArgs; ++i)
         if (args[i].typeInfo == tiSymbol && args[i] == symblankArg)
            result->params.push_back(lambda->params[i]);
         else                                                        
            newEnv->vars.push_back(Var(lambda->params[i].name, args[i]));

      result->body       = lambda->body;
      result->context = newEnv;
      return result;
   }
*/
Any apply(Any func, Vec* args/*, Frame* env*/)
{
   TimerFrameD tf("apply", toText(func) + " " + toText(args));
   Any result(apply1(func, args));
   tf.exit(toText(result));
   return result;
}

}

int testFunc(int x, int y)
{
   cout << "x=" << x << " y=" << y << endl;
   return x+y;
}

void testFuncArgs(const Any& in)
{
   cout << "in=" << in << endl;
}

int global;

struct typeidTest
{
   virtual void func() {};
};

struct typeidTest2 : public typeidTest
{

};

void testConv1(int*** i)
{
   cout << i << " " << *i << " " << **i << " " << ***i << endl;
}

void testConv2(int** i)
{
   cout << i << " " << *i << " " << **i << endl;
}

void testConv3(int* i)
{
   cout << i << " " << *i << endl;
}

void testConv4(int i)
{
   cout << i << endl;
}

#ifdef __linux__
int main(int argc, char* argv[])
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
   int local = 0;
   int* newint = new int(55);
   Any anewint(&newint);
   testConv1(anewint);
   testConv2(anewint);
   testConv3(anewint);
   testConv4(anewint);
   wcout << L"global = " << &global << endl;
   wcout << L"local  = " << &local  << endl;
   wcout << L"param  = " << &argc   << endl;
   wcout << L"newint = " << newint  << endl;

   typeidTest* t  = new typeidTest2;
   typeidTest  ta = *t;
   typeidTest& tb = *t;
   cout << typeid( t).name() << endl;//gives static type
   cout << typeid(*t).name() << endl;//gives dynamic type
   cout << typeid(ta).name() << endl;//gives static type (not surprisingly)
   cout << typeid(tb).name() << endl;//gives dynamic type
 
   setup();
   cout << endl;
   Any a = makePartApply(testFunc, 10);
   cout << a << endl;
   cout << a(5) << endl;

   testFuncArgs(17);

   parser.lang = &scheme;
   List::InterpL interp;
   Frame* userFrame = new Frame;
//   cout << toTextAny(userFrame) << endl;
   userFrame->context = globalFrame;
   Var v("testvar", 5);
   cout << " Var                 : " << toTextAny(v) << endl;
   cout << "&globalFrame->context: " << (int*)&globalFrame->context << endl;
   cout << "&globalFrame->context: " << toTextAny(&globalFrame->context) << endl;
   cout << " globalFrame->context: " << toTextAny(globalFrame->context) << endl;
   cout << "&globalFrame->vars   : " << toTextAny(&globalFrame->vars) << endl;
   cout << " globalFrame         : " << toTextAny(globalFrame) << endl;

   cout << "INT_MAX=" << INT_MAX << endl;
   cout << "sizeof(int)=" << sizeof(int) << endl;

   cout << getTypeAdd<int[100]>() << endl;
   //cout << getTypeAdd<int[]>() << endl;

   for (;;)
   {
      cout << "\033[32mscheme>\033[0m ";
      cout.flush();
      char inputChars[1000];
      cin.getline(inputChars, 1000);
      string input(inputChars);
#if 1
      parse(expr, input);
#else
      Any expr = parser.setProgram(input)->initUnit()->parseSExpr();//parser.setProgram(user)->initUnit()->parseExpr();
      List::compile(expr, userFrame);
      cout << expr << endl;

      Any res = List::eval(expr, userFrame);
      cout << res << endl;
      cout << toText(res.typeInfo) << endl;
#endif
   }
   cin.get();
   return 0;
}
