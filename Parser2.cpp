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

LC Module::lcofpos(int pos)
{
   int l = lower_bound(lines.begin(), lines.end(), pos+1) - lines.begin() - 1;

   return LC(l, pos - lines[l]);
}

Epsilon::Epsilon()                      {}
Range  ::Range  (char min, char max)    : min(min), max(max) {}
Char   ::Char   (char c)                : c(c) {}
String ::String (string text)           : text(text) {}
Alt    ::Alt    ()                      {}
Alt    ::Alt    (vector<Rule*> _elems)   : elems(_elems) {}
Seq    ::Seq    ()                      {}
Seq    ::Seq    (vector<Rule*> _elems)   : elems(_elems) {}

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

ParseResult* Rule::parse(State st)
{
   string n = toText(Any(*this, Dynamic()));
   ODSI("parse "+toTextInt(st.pos)+" "+n);
   ParseResult* r = parse1(st);
   if (r)
      ODSO("<- parse "+toTextInt(r->endS.pos)+" "+toText(r->ast)+" parser="+name+" "+n);
   else
      ODSO("<- parse "+n+" failed");
   return r;
}

ParseResult* Rule::parse1(State startS)
{
   throw "error";
}

ParseResult* Epsilon::parse1(State startS)
{
   return new ParseResult(startS, epsilonE);
}

ParseResult* String::parse1(State startS)
{
   int l = text.size();
   if (startS.module->text.substr(startS.pos, l) == text)
   {
      State endS = startS.advance(l);
      return new ParseResult(endS, text);
   }
   else
      return nullptr;
}

ParseResult* Char::parse1(State startS)
{
   if (startS.pos < startS.module->text.size())
   {
      if (c == (startS.module->text)[startS.pos])
      {
         State endS = startS.advance(1);
         return new ParseResult(endS, c);
      }
   }
   return nullptr;
}

ParseResult* Range::parse1(State startS)
{
   if (startS.pos < startS.module->text.size())
   {
      char c = (startS.module->text)[startS.pos];
      if (c >= min && c <= max)
      {
         State endS = startS.advance(1);
         return new ParseResult(endS, c);
      }
   }
   return nullptr;
}

ParseResult* Seq::parse1(State startS)
{
   ParseResult* rinner;
   State s(startS);
   Vec resElems;
   for (int i = 0; i < elems.size(); ++i)
   {
      rinner = elems[i]->parse(s);
      if (rinner == nullptr) 
         return nullptr;
      
      s = rinner->endS;
      resElems.push_back(rinner->ast);
   }
   return new ParseResult(s, resElems);
}

ParseResult* Alt::parse1(State startS)
{
   for (int i = 0; i < elems.size(); ++i)
   {
      ParseResult* r = elems[i]->parse(startS);
      if (r) 
         return r;
   }
   return nullptr;
}

bool doCheckIndent(LC cur, LC min)
{
   return cur.line > min.line ? cur.col > min.col : cur.col >= min.col;
}

ParseResult* IndentGroup::parse1(State startS)
{
   LC curLC = startS.module->lcofpos(startS.pos);
   //if (!doCheckIndent(curLC, startS.min)) return nullptr;
   State innerS(startS);
   //if (indentNext)
   //   innerS.min = curLC;
   //else
   //   innerS.min = LC(INT_MAX, curLC.col);
   innerS.startGroup = true;
   innerS.startSingle = false;   
   ParseResult* r = inner->parse(innerS);
   if (r == nullptr) return nullptr;
   State resS(r->endS);
   resS.min         = startS.min;
   resS.startGroup  = startS.startGroup ;
   resS.startSingle = startS.startSingle;
   return new ParseResult(resS, r->ast);
}

ParseResult* IndentItem::parse1(State startS)
{
   LC curLC = startS.module->lcofpos(startS.pos);
   State innerS(startS);
   innerS.startSingle = true;
   ParseResult* r = inner->parse(innerS);
   if (r == nullptr) return nullptr;
   State resS(r->endS);
   resS.min         = startS.min;
   resS.startGroup  = false;
   resS.startSingle = startS.startSingle;
   return new ParseResult(resS, r->ast);
}

ParseResult* CheckIndent::parse1(State startS)
{
   LC curLC = startS.module->lcofpos(startS.pos);
   if (startS.startSingle)
   {
      if (startS.startGroup)
      {
         startS.min = curLC;
      }
      else
      {
         if (curLC.col < startS.min.col) return nullptr;
      }
   }
   else
   {
      if (curLC.col <= startS.min.col) return nullptr;
   }
   return new ParseResult(startS, epsilonE);
}

Rule* checkIndent = new CheckIndent();

ParseResult* ApplyP::parse1(State s)
{
   ParseResult* r = inner->parse(s);
   if (r == nullptr) return r;
   return new ParseResult(r->endS, func(r->ast));
}

ParseResult* Skip::parse1(State s)
{
   ParseResult* r = inner->parse(s);
   if (r == nullptr) return r;
   return new ParseResult(r->endS, new SkipE(r->ast));
}

State State::advance(int n)
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
   Any term = r[0];
   Vec ops = r[1];
   for (int i = 0; i < (int)ops.size(); ++i)
      term = push_front(ops[i], term);
   return term;
}

Rule* chainl(Rule* term, Rule* op)
{
   return applyP(chainlR, seq(term, many(seq(op, term))))->setName("chainl");
}

Rule* whiteSpace = many(new Range(32, 32))->setName("whiteSpace");

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

int integerR(vector<Any> xs)
{
   istringstream s(stringOfVec(xs));
   int i;
   s >> i;
   return i;
}

int intOfString(string s)
{
   istringstream ss(s);
   int i;
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
      int exponent;
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
      type->add(Member(getSymbol(p), tiAny));
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
Rule* identifier = lexeme(applyP(identR  , many1(new Range('a', 'z'))))->setName("identifier");
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
   TypeInfo* tiRule        = getTypeAdd<Rule       >();
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
   addTypeLink(tiRule, tiRange      );
   addTypeLink(tiRule, tiAlt        );
   addTypeLink(tiRule, tiSeq        );
   addTypeLink(tiRule, tiEpsilon    );
   addTypeLink(tiRule, tiChar       );
   addTypeLink(tiRule, tiString     );
   addTypeLink(tiRule, tiApplyP     );
   addTypeLink(tiRule, tiIndentGroup);
   addTypeLink(tiRule, tiIndentItem );
   addTypeLink(tiRule, tiCheckIndent);
   addMember(&Rule  ::name, "name");
   addMember(&Char  ::c   , "c"   );
   addMember(&Range ::min , "min" );
   addMember(&Range ::max , "max" );
   addMember(&String::text, "text");

   /*
   Rule* p = chainl(chainl(integer, new String("*")), new String("+"));
   parse(p, "123+456*789");
   parse(p, "123*456+789");
   parse(seq(new String("hello")), "hello");
   */
}

void Module::getLines()
{
   int l = text.size();
   lines.clear();
   lines.push_back(0);
   for (int i = 0; i < l-1; ++i)
      if (text[i] == '\n')
         lines.push_back(i + 1);
}


