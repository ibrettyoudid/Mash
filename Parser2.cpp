// Copyright (c) 2023 Brett Curtis
#include "stdafx.h"
#include "Parser2.h"
#include "classes.h"

#include <algorithm>
#include "make_vec.h"

using namespace std;

TypeInfo* tiRule       ;
TypeInfo* tiNonTerminal;
TypeInfo* tiTerminal   ;
TypeInfo* tiElems      ;
TypeInfo* tiInner      ;
TypeInfo* tiAlt        ;
TypeInfo* tiSeq        ;
TypeInfo* tiApplyP     ;
TypeInfo* tiApplyP2    ;
TypeInfo* tiIndentGroup;
TypeInfo* tiIndentItem ;
TypeInfo* tiStringP    ;
TypeInfo* tiCharP      ;
TypeInfo* tiRange      ;
TypeInfo* tiEpsilon    ;
TypeInfo* tiCheckIndent;

TypeInfo* tiExpr       ;
TypeInfo* tiEpsilonE   ; 
TypeInfo* tiSkipE      ;
TypeInfo* tiWhiteSpaceE;
TypeInfo* tiLiteral    ;
TypeInfo* tiIdentifier ;
TypeInfo* tiApply      ;
TypeInfo* tiLambda     ;

Any id(Any x)
{
   return x;
}

LC Module::lcofpos(int64_t pos)
{
   int64_t l = lower_bound(lines.begin(), lines.end(), pos+1) - lines.begin() - 1;

   return LC(l, pos - lines[l]);
}

void CollectNamesIterator::visit(Rule*& rule) 
{ 
   if (ruleSet.contains(rule)) return; 
   ruleSet.insert(rule); 
   if (!rule->name.empty()) (*nameMap)[rule->name] = rule;
   rule->accept(this);
}

void ReplaceNamesIterator::visit(Rule*& rule) 
{ 
   if (ForwardTo* forwardTo = dynamic_cast<ForwardTo*>(rule)) 
   { 
      rule = nameMap[forwardTo->target]; 
      //delete forwardTo; 
   }
   if (ruleSet.contains(rule)) return; 
   ruleSet.insert(rule); 
   rule->accept(this);
}

void Lambda::accept(ExprIterator* iterator) { iterator->visit(body); };

Epsilon::Epsilon()                      {}
Range  ::Range  (char min, char max)    : min(min), max(max) {}
Char   ::Char   (char c)                : c(c) {}
String ::String (string text)           : text(text) {}
Alt    ::Alt    ()                      {}
Alt    ::Alt    (vector<Rule*> _elems)   : Elems(_elems) {}
Seq    ::Seq    ()                      {}
Seq    ::Seq    (vector<Rule*> _elems)   : Elems(_elems) {}

Epsilon*  epsilon  = new Epsilon;
EpsilonE* epsilonE = new EpsilonE;
Skipped*  skipped  = new Skipped;

Alt*  alt(                                                ) { return new Alt(                   ); }
Alt*  alt(Rule* r0                                        ) { return new Alt(vec(r0            )); }
Alt*  alt(Rule* r0, Rule* r1                              ) { return new Alt(vec(r0,r1         )); }
Alt*  alt(Rule* r0, Rule* r1, Rule* r2                    ) { return new Alt(vec(r0,r1,r2      )); }
Alt*  alt(Rule* r0, Rule* r1, Rule* r2, Rule* r3          ) { return new Alt(vec(r0,r1,r2,r3   )); }
Alt*  alt(Rule* r0, Rule* r1, Rule* r2, Rule* r3, Rule* r4) { return new Alt(vec(r0,r1,r2,r3,r4)); }

Seq*  seq(                                                ) { return new Seq(                   ); }
Seq*  seq(Rule* r0                                        ) { return new Seq(vec(r0            )); }
Seq*  seq(Rule* r0, Rule* r1                              ) { return new Seq(vec(r0,r1         )); }
Seq*  seq(Rule* r0, Rule* r1, Rule* r2                    ) { return new Seq(vec(r0,r1,r2      )); }
Seq*  seq(Rule* r0, Rule* r1, Rule* r2, Rule* r3          ) { return new Seq(vec(r0,r1,r2,r3   )); }
Seq*  seq(Rule* r0, Rule* r1, Rule* r2, Rule* r3, Rule* r4) { return new Seq(vec(r0,r1,r2,r3,r4)); }

ApplyP  *applyP (Any func, Rule* r    ) { return new ApplyP (func, r); }
ApplyP2 *applyP2(Any func, Rule* r    ) { return new ApplyP2(func, r); }
Skip    *skip   (          Rule* inner) { return new Skip   (inner  ); }
/* 
Any composeCall(Any f, Any g, Any x)
{
   return f(g(x));
}

Any compose(Any f, Any g)
{
   return makePartApply(composeCall, f, g);
}
*/

Any callWithVecArgs(Any f, Vec args)
{
   return f.call(&args[0], args.size());
}

Any adaptVecArgs(Any f)
{
   return partApply(callWithVecArgs, f);
}

Any constCall(Any r, Any x)
{
   return r;
}

Any constFunc(Any r)
{
   return partApply(constCall, r);
}

ParseResult* Rule::parse(State beginS)
{
   string n = toText(Any(*this, Dynamic()));
   ODSI("-> parse "+toTextInt(beginS.pos)+" "+n);
   ParseResult* r = parse1(beginS);
   if (r)
      ODSO("<- parse "+toTextInt(r->endS.pos)+" "+n+" "+toText(r->ast));
   else
      ODSO("<- parse "+n+" failed");
   return r;
}

ParseResult* Rule::parse1(State beginS)
{
   throw "error";
}

ParseResult* Epsilon::parse1(State beginS)
{
   return new ParseResult(beginS, epsilonE);
}

ParseResult* String::parse1(State beginS)
{
   int64_t l = text.size();
   if (beginS.module->text.substr(beginS.pos, l) == text)
   {
      State endS = beginS.advance(l);
      return new ParseResult(endS, text);
   }
   else
      return nullptr;
}

ParseResult* Char::parse1(State beginS)
{
   if (beginS.pos < beginS.module->text.size())
   {
      if (c == (beginS.module->text)[beginS.pos])
      {
         State endS = beginS.advance(1);
         return new ParseResult(endS, c);
      }
   }
   return nullptr;
}

ParseResult* Range::parse1(State beginS)
{
   if (beginS.pos < beginS.module->text.size())
   {
      char c = (beginS.module->text)[beginS.pos];
      if (c >= min && c <= max)
      {
         State endS = beginS.advance(1);
         return new ParseResult(endS, c);
      }
   }
   return nullptr;
}

ParseResult* Seq::parse1(State beginS)
{
   ParseResult* rinner;
   State s(beginS);
   Vec resElems;
   for (size_t i = 0; i < elems.size(); ++i)
   {
      rinner = elems[i]->parse(s);
      if (rinner == nullptr) 
         return nullptr;
      
      s = rinner->endS;
      resElems.push_back(rinner->ast);
   }
   return new ParseResult(s, resElems);
}

ParseResult* Alt::parse1(State beginS)
{
   for (size_t i = 0; i < elems.size(); ++i)
   {
      ParseResult* r = elems[i]->parse(beginS);
      if (r) 
         return r;
   }
   return nullptr;
}

bool doCheckIndent(LC cur, LC min)
{
   return cur.line > min.line ? cur.col > min.col : cur.col >= min.col;
}

ParseResult* IndentGroup::parse1(State beginS)
{
   LC curLC = beginS.module->lcofpos(beginS.pos);
   //if (!doCheckIndent(curLC, beginS.min)) return nullptr;
   State innerS(beginS);
   //if (indentNext)
   //   innerS.min = curLC;
   //else
   //   innerS.min = LC(INT_MAX, curLC.col);
   innerS.startGroup = true;
   innerS.startSingle = false;   
   ParseResult* r = inner->parse(innerS);
   if (r == nullptr) return nullptr;
   State resS(r->endS);
   resS.min         = beginS.min;
   resS.startGroup  = beginS.startGroup ;
   resS.startSingle = beginS.startSingle;
   return new ParseResult(resS, r->ast);
}

ParseResult* IndentItem::parse1(State beginS)
{
   LC curLC = beginS.module->lcofpos(beginS.pos);
   State innerS(beginS);
   innerS.startSingle = true;
   ParseResult* r = inner->parse(innerS);
   if (r == nullptr) return nullptr;
   State resS(r->endS);
   resS.min         = beginS.min;
   resS.startGroup  = false;
   resS.startSingle = beginS.startSingle;
   return new ParseResult(resS, r->ast);
}

ParseResult* CheckIndent::parse1(State beginS)
{
   LC curLC = beginS.module->lcofpos(beginS.pos);
   if (beginS.startSingle)
   {
      if (beginS.startGroup)
      {
         beginS.min = curLC;
      }
      else
      {
         if (curLC.col < beginS.min.col) return nullptr;
      }
   }
   else
   {
      if (curLC.col <= beginS.min.col) return nullptr;
   }
   return new ParseResult(beginS, epsilonE);
}

Rule* checkIndent = new CheckIndent();

ParseResult* ApplyP::parse1(State beginS)
{
   ParseResult* r = inner->parse(beginS);
   if (r == nullptr) return r;
   return new ParseResult(r->endS, func(r->ast));
}

ParseResult* ApplyP2::parse1(State beginS)
{
   ParseResult* r = inner->parse(beginS);
   if (r == nullptr) return r;
   return func(beginS, r->endS, r->ast);
}

ParseResult* Skip::parse1(State beginS)
{
   ParseResult* r = inner->parse(beginS);
   if (r == nullptr) return r;
   return new ParseResult(r->endS, new SkipE(r->ast));
}

State State::advance(int64_t n)
{
   State result(*this);
   result.pos += n;
   return result;
}

Vec push_front(Vec v, Any a)
{
   v.insert(v.begin(), a);
   return v;
}

Rule* optional(Rule* opt)
{
   return alt(opt, new Epsilon);
}

#if 1
Vec manyR(Any in)
{
   Vec out;
   while (true)
   {
      if (in.typeInfo == tiVec)
      {
         Vec invec(in);
         out.push_back(invec[0]);
         in = invec[1];
      }
      else
         return out;
   }
}

Rule* many(Rule* ru)
{
   Alt* a = alt();
   *a = *alt(seq(ru, a), epsilon);
   return applyP(manyR, a)->setName("many");
}

Rule* manyTill(Rule* ru, Rule* end)
{
   Alt* a = alt();
   *a = *alt(skip(end), seq(ru, a));
   return applyP(manyR, a)->setName("manyTill");
}

#else

Vec resMany(Vec in)
{
   if (Vec* el2 = in[1].any_cast<Vec>())
   {
      el2->insert(el2->begin(), in[0]);
      return *el2;
   }
   else
   {
      in.pop_back();
      return in;
   }
}
 
Rule* many(Rule* ru)
{
   Alt* a = alt();
   *a = *alt(applyP(resMany, seq(ru, a)), new Epsilon);
   return a->setName("many");
}
#endif

Vec many1AR(Vec in)
{
   return push_front(in[1], in[0]);
}

Rule* many1(Rule* ru)
{
   Alt* a = alt();
   *a = *alt(applyP(many1AR, seq(ru, a)), seq(ru));
   return a->setName("many1");
}

Vec sepByR(Vec in)
{
   return push_front(in[1], in[0]);
}

Rule* sepBy1(Rule* term, Rule* sep)
{
   return applyP(sepByR, seq(term, many(seq(sep, term))));
}

Expr* chainrR(Vec in)
{
   Vec ops = in[1];
   if (ops.size() > 0)
   {
      Expr* x;
      Expr* y = nullptr;
      for (int64_t i = ops.size() - 1; i >= 0; --i)
      {
         if (i > 0)
         {
            Vec opsh = ops[i-1];
            x = opsh[1];
         }
         else
            x = in[0];
         Vec opsi = ops[i];
         if (i == ops.size() - 1) y = opsi[1];
         y = new Apply({opsi[0], x, y});
      }
      return y;
   }
   return in[0];
}

Rule* chainr(Rule* term, Rule* op)
{
   return applyP(chainrR, seq(term, many(seq(op, term))))->setName("chainr");
}

Expr* chainlR(Vec r)
{
   Expr* term = r[0];
   Vec ops = r[1];
   for (size_t i = 0; i < ops.size(); ++i)
   {
      Vec opsi = ops[i];
      term = new Apply(vec<Expr*>(opsi[0], term, opsi[1]));
   }
   return term;
}

Rule* chainl(Rule* term, Rule* op)
{
   return applyP(chainlR, seq(term, many(seq(op, term))))->setName("chainl");
}

ParseResult* asSpanR(State startS, State endS, Any ast)
{
   return new ParseResult(endS, Span(startS.module, startS.pos, endS.pos));
}

Rule* asSpan(Rule* inner)
{
   return applyP2(asSpanR, inner);
}

Rule* lineComment = seq(new String("--"), 
                        many(new Range(32, 255)), 
                        new Char('\n'))->setName("lineComment");

Rule* blockComment = seq(new String("{-"), 
                         manyTill(alt(new ForwardTo("blockComment"), new Range(0, 255)), new String("-}")))->setName("blockComment");

Rule* toLineEnd = seq(many(new Char(' ')), 
                      optional(seq(new String("--"), many(new Range(32, 255)))), 
                      optional(new Char('\n')))->setName("lineSpace");

Rule* whiteSpace = many(alt(new Char(' '), 
                            new Char('\n'), 
                            lineComment, 
                            blockComment));

Any lexemeR(Vec in)
{
   Span pre = in[0];
   Span post = in[3];
   if (pre.size() || post.size())
      return new WhiteSpaceE(in[0], in[2], in[3]);
   else
      return in[2];
}

Rule* lexeme(Rule* lex)
{
   return applyP(lexemeR, seq(asSpan(whiteSpace), checkIndent, lex, asSpan(toLineEnd)));
}

Rule* symbol(string s)
{
   return lexeme(new String(s));
}

string stringOfVec(Vec in)
{
   string result;
   for (char c : in)
      result += c;
   return result;
}

int64_t integerR(vector<Any> xs)
{
   istringstream s(stringOfVec(xs));
   int64_t i;
   s >> i;
   return i;
}

int64_t intOfString(string s)
{
   istringstream ss(s);
   int64_t i;
   ss >> i;
   return i;
}

double doubleOfString(string s)
{
   istringstream ss(s);
   double d;
   ss >> d;
   return d;
}

double doubleR(vector<Any> xs)
{
   istringstream s(stringOfVec(xs));
   double d;
   s >> d;
   return d;
}

Literal* numberR(Vec in)
{
   string mantissaS = stringOfVec(in[0]);
   if (in[1].typeInfo == tiVec || in[2].typeInfo == tiVec)
   {
      int64_t exponent;
      if (in[2].typeInfo == tiVec)
      {
         Vec v = in[2];
         exponent = intOfString(stringOfVec(v[2]));
         if (v[1].typeInfo == tiChar && char(v[1]) == '-')
            exponent = -exponent;
      }
      else
         exponent = 0;
      if (in[1].typeInfo == tiVec)
      {
         string decimalS  = stringOfVec(Vec(in[1])[1]);
         mantissaS += decimalS;
         exponent -= decimalS.size();
      }
      return new Literal(doubleOfString(mantissaS) * pow(10, exponent));
   }
   else
      return new Literal(intOfString(mantissaS));
}

Identifier* identR(Span s)
{
   return new Identifier(s);
}

Expr* applicationR(Vec in)
{
   if (in.size() == 1)
      return in[0];
   vector<Expr*> args;
   for (auto e : in)
      args.push_back(e);
   return new Apply(args);
}



Lambda* lambdaR(Vec in)
{
   TypeInfo* type = newTypeInterp("lambdaParams");
   for (Any p : Vec(in[1]))
   {
      Identifier* i;
      if (p.typeInfo == tiWhiteSpaceE->ptr)
         i = p.as<WhiteSpaceE*>()->inner;
      else
         i = p;
      type->add(new Member(getSymbol(i->span.str()), tiAny));
   }
   return new Lambda(type, in[3]);
}

Any parse(Rule* p, string s)
{
   Module* m = new Module;
   m->text = s;
   m->getLines();
   State st(m);
   cout << "parsing '" << s << "'" << endl;
   ParseResult* r = p->parse(st);
#if 0
   if (r)
      cout << r->ast << " col=" << r->endS.pos << endl;
   else
      cout << "parse failed" << endl;
#endif
   return r;
}   

ApplyP* op(string s)
{
   return new ApplyP(identR, asSpan(new String(s)));
}

Expr* bracketR(Vec in)
{
   return in[1];
}

Apply* vecR(Vec in)
{
   Apply* out = new Apply({new Literal(VarFunc(makeVec, 0, LLONG_MAX)), in[1]});
   Vec ops = in[2];
   if (ops.size() > 0)
      for (Vec& op : ops)
         out->elems.push_back(op[1]);
   return out;
}

Apply* leftOpR(Vec in)
{
   return new Apply({new Literal(VarFunc(bind, 0, LLONG_MAX)), in[1], in[0], new Literal(PH(0))});
}

Apply* rightOpR(Vec in)
{
   return new Apply({new Literal(VarFunc(bind, 0, LLONG_MAX)), in[0], new Literal(PH(0)), in[1]});
}

Rule* digit       = new Range('0', '9');
Rule* upper       = new Range('A', 'Z');
Rule* lower       = new Range('a', 'z');
Rule* integer     = lexeme(applyP(integerR, many1(digit)))->setName("integer");
Rule* number      = lexeme(applyP(numberR, seq(many1(digit), 
                                               optional(seq(new Char('.'), many(digit))), 
                                               optional(seq(new Char('e'), alt(new Char('+'), new Char('-'), epsilon), many(digit))))))->setName("number");

Rule* identifier  = lexeme(applyP(identR, asSpan(seq(alt(upper, lower, new Char('_')), 
                                                     many(alt(upper, lower, digit, new Char('_')))))))->setName("identifier");

Rule* anyOp       = new Alt({op("."), op("*"), op("/"), op("+"), op("-"), op("$"), op("=")});

Rule* leftOp      = applyP(leftOpR, seq(new ForwardTo("term"), anyOp))->setName("leftOp");

Rule* rightOp     = applyP(rightOpR, seq(anyOp, new ForwardTo("term")))->setName("rightOp");

Rule* bracketed   = applyP(bracketR, seq(symbol("("), alt(rightOp, anyOp, leftOp, new ForwardTo("statm")), symbol(")")));

Rule* vecP        = applyP(vecR, seq(symbol("["), new ForwardTo("statm"), many(seq(symbol(","), new ForwardTo("statm"))), symbol("]")));

Rule* term        = alt(number, identifier, bracketed, vecP)->setName("term");

Rule* application = applyP(applicationR, many1(term))->setName("application");

Rule* expr9       = chainr(application, op("."))->setName("expr9");

Rule* expr7       = chainl(expr9, alt(op("*"), op("/")))->setName("expr7");

Rule* expr6       = chainl(expr7, alt(op("+"), op("-")))->setName("expr6");

Rule* expr0       = chainr(expr6, op("$"))->setName("expr0");

Rule* expr        = chainr(expr0, op("="))->setName("expr");

Rule* lambda      = applyP(lambdaR, seq(symbol("\\"), 
                                        many1(identifier), 
                                        symbol("->"), 
                                        expr))
                                           ->setName("lambda");

Rule* ifCase      = seq(expr,
                        symbol("then"),
                        expr)
                   ->setName("ifCase");

Rule* ifStat      = seq(symbol("if"), 
                        new IndentGroup(many1(new IndentItem(ifCase))))->setName("ifStat");

Rule* statm       = alt(ifStat, lambda, expr);

Rule* block       = (new IndentGroup(many1(new IndentItem(statm))))->setName("block");

string toTextLiteral(Literal* literal, int64_t width)
{
   return toTextAux(literal->value, width);
}

string toTextIdentifier(Identifier* ident, int64_t width)
{
   return toTextAux(ident->span.str(), width);
}

string toTextWhiteSpaceE(WhiteSpaceE* whiteSpaceE, int64_t width)
{
   return toTextAux(whiteSpaceE->inner, width);
}

void setupParser2()
{
   NameMap nameMap;
   nameMap["blockComment"] = blockComment;
   nameMap["statm"] = statm;
   nameMap["term" ] = term;
   ReplaceNamesIterator rni(nameMap);
   rni.visit(block);
   tiRule        = getTypeAdd<Rule       >();
   tiNonTerminal = getTypeAdd<NonTerminal>();
   tiTerminal    = getTypeAdd<Terminal   >();
   tiElems       = getTypeAdd<Elems      >();
   tiInner       = getTypeAdd<Inner      >();
   tiAlt         = getTypeAdd<Alt        >();
   tiSeq         = getTypeAdd<Seq        >();
   tiApplyP      = getTypeAdd<ApplyP     >();
   tiApplyP2     = getTypeAdd<ApplyP2    >();
   tiIndentGroup = getTypeAdd<IndentGroup>();
   tiIndentItem  = getTypeAdd<IndentItem >();
   tiStringP     = getTypeAdd<String     >();
   tiCharP       = getTypeAdd<Char       >();
   tiRange       = getTypeAdd<Range      >();
   tiEpsilon     = getTypeAdd<Epsilon    >();
   tiCheckIndent = getTypeAdd<CheckIndent>();
   addTypeLink(tiRule       , tiNonTerminal);
   addTypeLink(tiRule       , tiTerminal   );
   addTypeLink(tiNonTerminal, tiElems      );
   addTypeLink(tiNonTerminal, tiInner      );
   addTypeLink(tiElems      , tiAlt        );
   addTypeLink(tiElems      , tiSeq        );
   addTypeLink(tiInner      , tiApplyP     );
   addTypeLink(tiInner      , tiApplyP2    );
   addTypeLink(tiInner      , tiIndentGroup);
   addTypeLink(tiInner      , tiIndentItem );
   addTypeLink(tiTerminal   , tiStringP    );
   addTypeLink(tiTerminal   , tiCharP      );
   addTypeLink(tiTerminal   , tiRange      );
   addTypeLink(tiTerminal   , tiEpsilon    );
   addTypeLink(tiTerminal   , tiCheckIndent);
   addMember(&Rule  ::name, "name");
   addMember(&Inner ::inner, "inner");
   //addMember(&Elems ::elems, "elems");
   addMember(&Char  ::c   , "c"   );
   addMember(&Range ::min , "min" );
   addMember(&Range ::max , "max" );
   addMember(&String::text, "text");

   tiExpr        = getTypeAdd<Expr       >();
   tiEpsilonE    = getTypeAdd<EpsilonE   >();
   tiSkipE       = getTypeAdd<SkipE      >();
   tiWhiteSpaceE = getTypeAdd<WhiteSpaceE>();
   tiLiteral     = getTypeAdd<Literal    >();
   tiIdentifier  = getTypeAdd<Identifier >();
   tiApply       = getTypeAdd<Apply      >();
   tiLambda      = getTypeAdd<Lambda     >();
   getTypeAdd<Apply*>();
   addTypeLink(tiExpr, tiEpsilonE);
   addTypeLink(tiExpr, tiSkipE);
   addTypeLink(tiExpr, tiWhiteSpaceE);
   addTypeLink(tiExpr, tiLiteral);
   addTypeLink(tiExpr, tiIdentifier);
   addTypeLink(tiExpr, tiApply);
   addTypeLink(tiExpr, tiLambda);
   toTextAux.add(toTextLiteral);
   //toTextAux.add(toTextIdentifier);
   toTextAux.add(toTextWhiteSpaceE);
   addMember(&Apply::elems, "elems");
}

void Module::getLines()
{
   int64_t l = text.size() - 1;
   lines.clear();
   lines.push_back(0);
   for (int64_t i = 0; i < l; ++i)
      if (text[i] == '\n')
         lines.push_back(i + 1);
}


