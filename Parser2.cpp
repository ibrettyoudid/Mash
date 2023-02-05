// Copyright (c) 2023 Brett Curtis
#include "stdafx.h"
#include "Parser2.h"
#include "classes.h"

#include <algorithm>
#include "make_vec.h"

using namespace std;

Any id(Any x)
{
   return x;
}

LC Module::lcofpos(int64_t pos)
{
   int64_t l = lower_bound(lines.begin(), lines.end(), pos+1) - lines.begin() - 1;

   return LC(l, pos - lines[l]);
}

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

ApplyP *applyP(Any func, Rule* r    ) { return new ApplyP(func, r); }
Skip   *skip  (          Rule* inner) { return new Skip  (inner  ); }
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
   return f.call(&*args.begin(), args.size());
}

Any adaptVecArgs(Any f)
{
   return makePartApply(callWithVecArgs, f);
}

Any constCall(Any r, Any x)
{
   return r;
}

Any constFunc(Any r)
{
   return makePartApply(constCall, r);
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
   vector<Any> out;
   while (Vec* inv = in.any_cast<Vec>())
   {
      out.push_back((*inv)[0]);
      in = (*inv)[1];
   }
   return out;
}

Rule* many(Rule* ru)
{
   Alt* a = alt();
   *a = *alt(seq(ru, a), new Epsilon);
   return applyP(manyR, a)->setName("many");
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

Rule* chainr(Rule* term, Rule* op)
{
   Alt* a = new Alt();
   *a = *alt(seq(term, op, a), term);
   return a;
}

Any chainlR(Vec r)
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

Rule* whiteSpace = many(new Range(0, 32))->setName("whiteSpace");
Rule* lineComment = seq(new String("--"), many(new Range(32, 255)), new Char('\n'))->setName("lineComment");
Rule* blockComment1 = alt(new String("-}"), seq(new Range(0, 255)));
Rule* blockComment = seq(new String("{-"), blockComment1)->setName("blockComment");

Any lexemeR(Vec in)
{
   return in[1];
}

Rule* lexeme(Rule* lex)
{
   return applyP(lexemeR, seq(checkIndent, lex, whiteSpace));
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

Any numberR(Vec in)
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
      return doubleOfString(mantissaS) * pow(10, exponent);
   }
   else
      return intOfString(mantissaS);
}

string identR(Vec v)
{
   return stringOfVec(v);
}

Lambda* lambdaR(Vec in)
{
   TypeInfo* type = new TypeInfo();
   for (string p : Vec(in[0]))
      type->add(new Member(getSymbol(p), tiAny));
   return new Lambda(type, in[1]);
}

void parse(Rule* p, string s)
{
   Module* m = new Module;
   m->text = s;
   m->getLines();
   State st(m);
   cout << "parsing '" << s << "'" << endl;
   ParseResult* r = p->parse(st);
   if (r)
      cout << r->ast << " col=" << r->endS.pos << endl;
   else
      cout << "parse failed" << endl;
}   


Rule* expr = new ApplyP();//forward declaration needed to make a loop

Rule* integer    = lexeme(applyP(integerR, many1(new Range('0', '9'))))->setName("integer");
Rule* number     = lexeme(applyP(numberR , seq(many1(new Range('0', '9')), 
                                               optional(seq(new Char('.'), many(new Range('0', '9')))), 
                                               optional(seq(new Char('e'), alt(new Char('+'), new Char('-'), epsilon), many(new Range('0', '9')))))))->setName("number");

Rule* identifier = lexeme(applyP(identR  , seq(alt(new Range('a', 'z'), new Range('A', 'Z'), new Char('_')), 
                                               many(alt(new Range('a', 'z'), new Range('A', 'Z'), new Range('0', '9'), new Char('_'))))))->setName("identifier");

Rule* lambda     = applyP(lambdaR, seq(skip(symbol("\\")), 
                                       many1(identifier), 
                                       skip(symbol("->")), 
                                       expr))
                                          ->setName("lambda");

Rule* ifCase     = seq(expr,
                       symbol("then"),
                       expr)
                   ->setName("ifCase");

Rule* ifStat     = seq(symbol("if"), 
                       new IndentGroup(many1(new IndentItem(ifCase))))->setName("ifStat");

Rule* term       = alt(number, 
                       identifier, 
                       ifStat, 
                       lambda)
                   ->setName("term");

Rule* terms      = many1(term)->setName("terms");

void initParser2()
{
   *(ApplyP*)expr = *(ApplyP*)chainl(chainl(term, new String("*")), new String("+"))->setName("expr");//and then finish the loop
   ((Seq*)((Alt*)blockComment1)->elems[1])->elems.push_back(blockComment1);
   TypeInfo* tiRule        = getTypeAdd<Rule       >();
   TypeInfo* tiTerminal    = getTypeAdd<Terminal   >();
   TypeInfo* tiNonTerminal = getTypeAdd<NonTerminal>();
   TypeInfo* tiElems       = getTypeAdd<Elems      >();
   TypeInfo* tiInner       = getTypeAdd<Inner      >();
   TypeInfo* tiAlt         = getTypeAdd<Alt        >();
   TypeInfo* tiSeq         = getTypeAdd<Seq        >();
   TypeInfo* tiEpsilon     = getTypeAdd<Epsilon    >();
   TypeInfo* tiChar        = getTypeAdd<Char       >();
   TypeInfo* tiRange       = getTypeAdd<Range      >();
   TypeInfo* tiString      = getTypeAdd<String     >();
   TypeInfo* tiApplyP      = getTypeAdd<ApplyP     >();
   TypeInfo* tiIndentGroup = getTypeAdd<IndentGroup>();
   TypeInfo* tiIndentItem  = getTypeAdd<IndentItem >();
   TypeInfo* tiCheckIndent = getTypeAdd<CheckIndent>();
   addTypeLink(tiRule       , tiTerminal   );
   addTypeLink(tiRule       , tiNonTerminal);
   addTypeLink(tiNonTerminal, tiElems      );
   addTypeLink(tiNonTerminal, tiInner      );
   addTypeLink(tiTerminal   , tiRange      );
   addTypeLink(tiElems      , tiAlt        );
   addTypeLink(tiElems      , tiSeq        );
   addTypeLink(tiTerminal   , tiEpsilon    );
   addTypeLink(tiTerminal   , tiChar       );
   addTypeLink(tiTerminal   , tiString     );
   addTypeLink(tiInner      , tiApplyP     );
   addTypeLink(tiInner      , tiIndentGroup);
   addTypeLink(tiInner      , tiIndentItem );
   addTypeLink(tiTerminal   , tiCheckIndent);
   addMember(&Rule  ::name, "name");
   addMember(&Inner ::inner, "inner");
   //addMember(&Elems ::elems, "elems");
   addMember(&Char  ::c   , "c"   );
   addMember(&Range ::min , "min" );
   addMember(&Range ::max , "max" );
   addMember(&String::text, "text");

   addMember(&Apply::elems, "elems");
   /*
   Rule* p = chainl(chainl(integer, new String("*")), new String("+"));
   parse(p, "123+456*789");
   parse(p, "123*456+789");
   parse(seq(new String("hello")), "hello");
   */
}

void Module::getLines()
{
   int64_t l = text.size();
   lines.clear();
   lines.push_back(0);
   for (size_t i = 0; i < l-1; ++i)
      if (text[i] == '\n')
         lines.push_back(i + 1);
}


