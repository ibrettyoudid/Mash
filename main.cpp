// Copyright (c) 2023 Brett Curtis
// main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "classes.h"
#include "Parser.h"
#include "Parser2.h"
#include "TimerFrame.h"
#include "InterpL.h"
#include "InterpS.h"

using namespace std;

struct Test
{
   virtual void vfunc()
   {
      cout << "hello Test::vfunc" << endl;
   }

   void func()
   {
      cout << "hello Test::func" << endl;
   }
};

struct Test1
{
   virtual void vfunc()
   {
      cout << "hello Test1::vfunc" << endl;
   }

   void func()
   {
      cout << "hello Test1::func" << endl;
   }
};

struct Test2 : Test, Test1
{
   virtual void vfunc()
   {
      cout << "hello Test2::vfunc" << endl;
   }
   
   void func()
   {
      cout << "hello Test2::func" << endl;
   }
};

#ifdef __linux__
int main(int argc, char* argv[])
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
   setup();
   Struct::setupInterp();
   Test t;
   Test2 t2;
   Any f(&Test::vfunc);
   cout << f.hex() << endl << endl;
   Any g(&Test1::vfunc);
   cout << g.hex() << endl << endl;
   Any h(&Test2::vfunc);
   cout << h.hex() << endl << endl;
   cout << typeid(&Test ::vfunc).name() << &Test ::vfunc << endl;
   cout << typeid(&Test1::vfunc).name() << &Test1::vfunc << endl;
   cout << typeid(&Test2::vfunc).name() << &Test2::vfunc << endl;
   f(t);
   parser.lang = &scheme;
   List::InterpL interp;
   Frame* userFrame = new Frame;
   userFrame->context = globalFrame;

   int64_t lang = 1;

   for (;;)
   {
      cout << "\033[32mscheme>\033[0m ";
      cout.flush();
      char inputChars[1000];
      cin.getline(inputChars, 1000);
      string input(inputChars);
      switch (lang)
      {
         case 1:
         {
            ParseResult* r = parse(expr, input);
            if (r)
            {
               Struct::compile(r->ast, Struct::stack);
               cout << Struct::eval(r->ast, Struct::stack) << endl;
            }
            else
               cout << "parse error" << endl;
            break;
         }
         case 2:
         {
            Any expr = parser.setProgram(input)->initUnit()->parseSExpr();//parser.setProgram(user)->initUnit()->parseExpr();
            List::compile(expr, userFrame);
            cout << expr << endl;

            Any res = List::eval(expr, userFrame);
            cout << res << endl;
            cout << toText(res.typeInfo) << endl;
            break;
         }
      }
   }
   return 0;
}
