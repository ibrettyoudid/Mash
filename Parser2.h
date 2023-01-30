// Copyright (c) 2023 Brett Curtis
#include <vector>
#include <string>

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
   std::string      text;
   std::vector<int> lines;

   LC       lcofpos(int);
   int      posoflc(int);
   void     getLines();
};

struct State
{
   Module*  module;
   int      pos;
   bool     startSingle;
   bool     startGroup;
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

struct ParseResult;

struct Visitor;

struct Rule
{
   std::string        name;
   std::vector<Rule*> parents;
   
   Rule() {}
   
   virtual  ParseResult*  parse (State st);
   virtual  ParseResult*  parse1(State st);
   virtual  void          visit (Visitor* visitor) { }

   Rule*    setRF(Any f);
   Rule*    addRF(Any f);
   Rule*    setName(std::string n) { name = n; return this; }
};

struct NonTerminal : public Rule
{
   std::vector<Rule*> kids;
};

struct Terminal : public Rule
{
};

struct Epsilon : public Terminal
{
   ParseResult*  parse1(State st);

   Epsilon();
};

struct Char : public Terminal
{
   char c;

   Char(char c);

   ParseResult*  parse1(State st);
};

struct Range : public Terminal
{
   char min;
   char max;

   Range(char min, char max);

   ParseResult*  parse1(State st);
};

struct String : public Terminal
{
   std::string text;

   String(std::string text);

   ParseResult*  parse1(State st);
};

struct Alt : public NonTerminal
{
   std::vector<Rule*> elems;

   Alt();
   Alt(std::vector<Rule*> elems);

   ParseResult*  parse1(State st);
};

struct Seq : public NonTerminal
{
   std::vector<Rule*> elems;

   Seq      ();
   Seq      (std::vector<Rule*> elems);

   ParseResult*  parse1(State st);
};

struct IndentGroup : public NonTerminal
{
   Rule*    inner;

   IndentGroup   (Rule* inner) : inner(inner) {}

   ParseResult*  parse1(State st);
};

struct IndentItem : public NonTerminal
{
   Rule*    inner;

   IndentItem   (Rule* inner) : inner(inner) {}

   ParseResult*  parse1(State st);
};

struct CheckIndent : public Terminal
{
   CheckIndent() {}

   ParseResult*  parse1(State st);
};

struct ApplyP : public NonTerminal
{
   Any   func;
   Rule* inner;

   ApplyP() {}
   ApplyP(Any func, Rule* inner) : func(func), inner(inner) {}

   ParseResult*  parse1(State st);
   //        void  visit(Visitor* visitor);
};

struct Skip : public NonTerminal
{
   Rule* inner;

   Skip(Rule* inner) : inner(inner) {}

   ParseResult* parse1(State st);
   //void visit(Visitor* visitor);
};

/*
Pros
easy to handle literals
can get addresses of member functions
easy to add new types

Cons:
not type safe
*/
#if 0
struct Expr;

struct ParseResult
{
   State  endS;   //this is needed for parser to work
   Any    ast;

   //ParseResult() {}
   ParseResult(State endS, Any a) : endS(endS), ast(a) {}
};

struct Expr
{
   virtual Any eval(Any) = 0;
};

struct Literal : public Expr
{
   Any    value;
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

struct If : public Expr
{
   Vec terms;
   Any else_;

   If(Vec terms, Any else_) : terms(terms), else_(else_) {}
};

struct CaseTerm : public Expr
{
   Any match;
   Any eval;

   CaseTerm(Any match, Any eval) : match(match), eval(eval) { }
}; 

struct Case : public Expr
{
   Vec terms;
   Any else_;

   If(Vec terms, Any else_) : terms(terms), else_(else_) {}
};

struct ApplyAST : public Expr
{
   Vec elems;

   ApplyAST(Vec elems) : elems(elems) {}
};

struct Lambda : public Expr
{
   std::vector<string> arguments;
   Any            body;
   Lambda* context;
};
#else
struct Expr;

struct ParseResult
{
   State  endS;   //this is needed for parser to work
   Any    ast;

   //ParseResult() {}
   ParseResult(State endS, Any ast) : endS(endS), ast(ast) {}
};

struct Expr
{
//   virtual Any eval(Expr*);
};

struct EpsilonE : public Expr
{
   EpsilonE() {}
};

struct SkipE : public Expr
{
   Expr *inner;

   SkipE(Expr* inner) : inner(inner) {}
};

struct WhiteSpaceE : public Expr
{
   std::string before;
   Expr* expr;
   std::string after;
};

struct Literal : public Expr
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
   std::vector<IfTerm*> cases;
   Expr*           else_;

   IfExpr(std::vector<IfTerm*> cases, Expr* else_) : cases(cases), else_(else_) {}
};

struct CaseTerm : public Expr
{
   Any   match;
   Expr* eval;

   CaseTerm(Any match, Expr* eval) : match(match), eval(eval) {}
};

struct Case : public Expr
{
   Expr* case_;
   std::vector<CaseTerm*> terms;
   Expr* else_;

   Case(Expr* case_, std::vector<CaseTerm*> terms, Expr* else_) : case_(case_), terms(terms), else_(else_) { }
};

struct ApplyExpr : public Expr
{
   std::vector<Expr*>  elems;

   ApplyExpr(std::vector<Expr*> elems) : elems(elems) {}
};

struct Lambda : public Expr
{
   TypeInfo* type;
   Expr*     body;
   
   Lambda(TypeInfo* type, Expr* body) : type(type), body(body) { }
};

struct Skipped : public Expr
{
};
#endif
void initParser2();
void parse(Rule* p, std::string s);

extern Rule* expr;
extern Rule* whiteSpace;
extern Rule* identifier;
