#include <vector>
#include <string>

using namespace std;

#include "any.h"

//this version uses Any to represent programs

struct LC
{
   int      line;
   int      col;
   LC(int line, int col) : line(line), col(col) {}
};

typedef int Pos;

struct Module
{
   string      text;
   vector<int> lines;

   LC       lcofpos(int);
   int      posoflc(int);
   void     getLines();
};

struct State
{
   Module*  module;
   int      pos;
   LC       min;

   State(Module* module) : module(module), pos(0), min(0, 0) {}
   State    advance(int);
};

struct AST
{
   Module*  module;
   int      sPos;
   int      ePos;
   //Rule*    rule;

   AST(Module* module, int sPos, int ePos) : module(module), sPos(sPos), ePos(ePos) {}
};

struct Result
{
   State    endS;   //this is needed for parser to work
   Any    ast;

   //Result() {}
   Result(State endS, Any a) : endS(endS), ast(a) {}
};

struct Rule
{
   string   name;
   
   Rule() {}
   
   virtual  Result*  parse(State st);
   virtual  Result*  parse1(State) { throw "bad"; }

   Rule*    setRF(Any f);
   Rule*    addRF(Any f);
   Rule*    setName(string n) { name = n; return this; }
};

struct Epsilon : public Rule
{
   Result*  parse1(State st);

   Epsilon();
};

struct Char : public Rule
{
   char c;

   Result*  parse1(State st);
};

struct Range : public Rule
{
   char min;
   char max;

   Range(char min, char max);

   Result*  parse1(State st);
};

struct String : public Rule
{
   string text;

   String(string text);

   Result*  parse1(State st);
};

struct Alt : public Rule
{
   vector<Rule*> elems;

   Alt();
   Alt(vector<Rule*> elems);

   Result*  parse1(State st);
};

struct Seq : public Rule
{
   vector<Rule*> elems;

   Seq      ();
   Seq      (vector<Rule*> elems);

   Result*  parse1(State st);
};

struct Indent : public Rule
{
   Rule*    inner;
   bool     indentNext;

   Indent   (Rule* inner, bool i) : inner(inner), indentNext(i) {}

   Result*  parse1(State st);
};

struct CheckIndent : public Rule
{
   CheckIndent() {}

   Result*  parse1(State st);
};

struct ApplyP : public Rule
{
   Any* func;
   Rule* inner;

   ApplyP(Any func, Rule* inner) : func(func), inner(inner) {}

   Result*  parse1(State st);
};

struct Ignore
{
};
#if 0
struct Expr
{
   virtual Any eval(Expr*);
};

struct Literal : Expr
{
   Any value;
   Literal(Any v) : value(v) {}

   Any eval(Expr*)
   {
      return value;
   }
};

struct IfTerm
{
   Expr* test;
   Expr* eval;

   IfTerm(Expr* test, Expr* eval) : test(test), eval(eval) {}
};

struct IfExpr : public Expr
{
   vector<IfTerm*> cases;
   Expr*           else_;

   IfExpr(vector<IfTerm*> cases, Expr* else_) : cases(cases), else_(else_) {}
};

struct CaseTerm : public Expr
{
   Any   match;
   Expr* eval;
};

struct Case : public Expr
{
   vector<
};

struct ApplyAST : public Expr
{
   Vec elems;

   ApplyAST(Vec elems) : elems(elems) {}
};

struct Lambda : public Expr
{
   vector<string> arguments;
   Expr*          body;
   Lambda*        context;
};
#else
struct Expr
{
   virtual Any eval(Any);
};

struct Literal : Expr
{
   Any value;
   Literal(Any v) : value(v) {}

   Any eval(Any)
   {
      return value;
   }
};

struct IfTerm
{
   Any test;
   Any eval;

   IfTerm(Any test, Any eval) : test(test), eval(eval) {}
};

struct IfExpr : public Expr
{
   vector<IfTerm*> cases;
   Any else_;

   IfExpr(vector<IfTerm*> cases, Any else_) : cases(cases), else_(else_) {}
};

struct CaseTerm : public Expr
{
   Any match;
   Any eval;
};

struct Case : public Expr
{
   vector <
};

struct ApplyAST : public Expr
{
   Vec elems;

   ApplyAST(Vec elems) : elems(elems) {}
};

struct Lambda : public Expr
{
   vector<string> arguments;
   Any            body;
   Lambda*        context;
};
#endif
void initParser2();
void parse(Rule* p, string s);

extern Rule* expr;
extern Rule* whiteSpace;
extern Rule* identifier;
