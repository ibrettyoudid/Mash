// Copyright (c) 2023 Brett Curtis
#include <vector>
#include <string>

#include "any.h"

//this version uses Any to represent programs

struct LC
{
   int64_t      line;
   int64_t      col;
            LC(int64_t line, int64_t col) : line(line), col(col) {}
};

typedef int64_t Pos;

struct Module
{
   std::string      text;
   std::vector<int64_t> lines;

   LC       lcofpos(int64_t);
   int64_t  posoflc(int64_t);
   void     getLines();
};

struct State
{
   Module*  module;
   int64_t  pos;
   bool     startSingle;
   bool     startGroup;
   LC       min;

   State(Module* module) : module(module), pos(0), min(0, 0) {}

   State    advance(int64_t);
};

struct Span
{
   Module*  module;
   int64_t  sPos;
   int64_t  ePos;
   //Rule*    rule;

   Span(Module* module, int64_t sPos, int64_t ePos) : module(module), sPos(sPos), ePos(ePos) {}

   std::string str() { return module->text.substr(sPos, ePos - sPos); }
   int64_t     size() { return ePos - sPos; }
};

struct ParseResult;

struct Iterator;

struct Rule
{
   std::string        name;
   
   Rule() {}
   
   virtual  ParseResult*  parse (State st);
   virtual  ParseResult*  parse1(State st);
   virtual  void          accept(Iterator* iterator) {};

   Rule*    setRF(Any f);
   Rule*    addRF(Any f);
   Rule*    setName(std::string n) { name = n; return this; }
};

struct NonTerminal : public Rule
{
};

struct Terminal : public Rule
{
};

struct Iterator
{
   virtual ~Iterator() {};

   virtual void visit(Rule*& rule) = 0;
};

typedef std::map<std::string, Rule*> NameMap;

struct CollectNamesIterator : public Iterator
{
   std::set<Rule*> ruleSet;
   NameMap* nameMap;

   CollectNamesIterator() : nameMap(new NameMap()) {}

   virtual void visit(Rule*& rule);
};

struct ReplaceNamesIterator : public Iterator
{
   std::set<Rule*> ruleSet;
   NameMap& nameMap;

   ReplaceNamesIterator(NameMap& nameMap) : nameMap(nameMap) {}

   virtual void visit(Rule*& rule);
};

struct Inner : public NonTerminal
{
   Rule* inner;

   Inner(Rule* inner) : inner(inner) {}

   virtual void accept(Iterator* iterator) { iterator->visit(inner); }
};

struct Elems : public NonTerminal
{
   std::vector<Rule*> elems;

   Elems() {}
   Elems(std::vector<Rule*> elems) : elems(elems) {}

   virtual void accept(Iterator* iterator) { for (auto &elem : elems) iterator->visit(elem); }
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

struct Alt : public Elems
{
   Alt();
   Alt(std::vector<Rule*> elems);

   ParseResult*  parse1(State st);
};

struct Seq : public Elems
{
   Seq      ();
   Seq      (std::vector<Rule*> elems);

   ParseResult*  parse1(State st);
};

struct IndentGroup : public Inner
{
   Rule*    inner;

   IndentGroup   (Rule* inner) : Inner(inner) {}

   ParseResult*  parse1(State st);
};

struct IndentItem : public Inner
{
   Rule*    inner;

   IndentItem   (Rule* inner) : Inner(inner) {}

   ParseResult*  parse1(State st);
};

struct CheckIndent : public Terminal
{
   CheckIndent() {}

   ParseResult*  parse1(State st);
};

struct ApplyP : public Inner
{
   Any   func;

   ApplyP() : Inner(nullptr) {}
   ApplyP(Any func, Rule* inner) : func(func), Inner(inner) {}

   ParseResult*  parse1(State st);
   //        void  visit(Iterator* iterator);
};

struct ApplyP2 : public Inner
{
   Any   func;

   ApplyP2() : Inner(nullptr) {}
   ApplyP2(Any func, Rule* inner) : func(func), Inner(inner) {}

   ParseResult*  parse1(State st);
   //        void  visit(Iterator* iterator);
};

struct ForwardTo : public NonTerminal
{
   std::string target;

   ForwardTo(std::string target) : target(target) {}
};

struct Skip : public Inner
{
   Skip(Rule* inner) : Inner(inner) {}

   ParseResult* parse1(State st);
   //void visit(Iterator* iterator);
};

/*
Pros
easy to handle literals
can get addresses of member functions
easy to add new types

Cons:
not type safe
*/
struct Expr;

struct ParseResult
{
   State  endS;   //this is needed for parser to work
   Any    ast;

   //ParseResult() {}
   ParseResult(State endS, Any ast) : endS(endS), ast(ast) {}
};

struct ExprIterator
{
   virtual ~ExprIterator() {};

   virtual void visit(Expr* expr) = 0;
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
   Span  before;
   Any   inner;
   Span  after;
   WhiteSpaceE(Span before, Any inner, Span after) : before(before), inner(inner), after(after) {}
   virtual void accept(ExprIterator* iterator) { iterator->visit(inner); };
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

struct Identifier : public Expr
{
   Span span;
   Member* member = nullptr;
   int64_t frameN;
   Identifier(Span span) : span(span) {}
};

struct IfTerm : public Expr
{
   Expr* test;
   Expr* eval;
   IfTerm(Expr* test, Expr* eval) : test(test), eval(eval) {}
   virtual void accept(ExprIterator* iterator) { iterator->visit(test); iterator->visit(eval); };
};

struct IfExpr : public Expr
{
   std::vector<IfTerm*> cases;
   Expr*           else_;
   IfExpr(std::vector<IfTerm*> cases, Expr* else_) : cases(cases), else_(else_) {}
   virtual void accept(ExprIterator* iterator) { for (auto c : cases) iterator->visit(c); iterator->visit(else_); };
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

struct Apply : public Expr
{
   std::vector<Expr*>  elems;
   Apply(std::vector<Expr*> elems) : elems(elems) {}
   virtual void accept(ExprIterator* iterator) { for (auto e : elems) iterator->visit(e); };
};

struct Skipped : public Expr
{
};

void setupParser2();
Any parse(Rule* p, std::string s);

extern Rule* expr;
extern Rule* whiteSpace;
extern Rule* identifier;
