// Copyright (c) 2023 Brett Curtis
#include "stdafx.h"
#include "classes.h"
#include "Parser.h"
#include "Parser2.h"
#include "InterpL.h"
//#include "io.h"

using namespace std;

Symbols symbols;
Symbol* symBlankArg = getSymbol("_", 0, 0);

Symbol* getSymbol(string name, int group, int id)
{
   Symbol s(name);
   auto i = symbols.find(&s);
   if (i == symbols.end())
   {
      Symbol* sp = new Symbol(name, group, id);
      symbols.insert(sp);
      return sp;
   }
   return *i;
}
Cons* nil = nullptr;

MultimethodAny       massign     ("assign");
Multimethod<string>  toTextAux   ("toTextAux");
Multimethod<void>    findCycles  ("findCycles");
MultimethodAny       pushBack    ;
MultimethodAny       insertElemX ;
MultimethodAny       eraseElemX  ;
MultimethodAny       getElem     ;
MultimethodAny       setElemX    ;
MultimethodAny       iterBegin   ;
MultimethodAny       iterEnd     ;

MultimethodAny       append      ;
MultimethodAny       fold        ;

MultimethodAny       mderef      ;
MultimethodAny       minc        ;
MultimethodAny       mdec        ;
Multimethod<int>     length      ;
MultimethodAny       madd        ;
MultimethodAny       msubtract   ;
MultimethodAny       mmultiply   ;
MultimethodAny       mdivide     ;
Multimethod<bool>    mless       ;
Multimethod<bool>    mlessEqual  ;
Multimethod<bool>    mmore       ;
Multimethod<bool>    mmoreEqual  ;
Multimethod<bool>    mequal      ;
Multimethod<bool>    mnotEqual   ;
Multimethod<Any>     convert     ;

bool operator==(Any a, Any b) { return mequal(a, b); }
bool operator!=(Any a, Any b) { return mnotEqual(a, b); }
Any  operator++(Any a)        { return minc(a); }
Any  operator* (Any a)        { return mderef(a); }


TypeInfo* tiAny     ;
TypeInfo* tiSymbol  ;
TypeInfo* tiCons    ;
TypeInfo* tiChar    ;
TypeInfo* tiInt8    ;
TypeInfo* tiInt16   ;
TypeInfo* tiInt     ;
TypeInfo* tiInt64   ;
TypeInfo* tiUInt8   ;
TypeInfo* tiUInt16  ;
TypeInfo* tiUInt    ;
TypeInfo* tiUInt64  ;
TypeInfo* tiFloat   ;
TypeInfo* tiDouble  ;
TypeInfo* tiSigned  ;
TypeInfo* tiUnsigned;
TypeInfo* tiFloating;
TypeInfo* tiInteger ;
TypeInfo* tiNumber  ;
TypeInfo* tiListClosure  ;
TypeInfo* tiStructClosure;
TypeInfo* tiParseEnd;
TypeInfo* tiString  ;
TypeInfo* tiVarRef  ; 
TypeInfo* tiVarFunc ;
TypeInfo* tiPartApply;
TypeInfo* tiTypeInfo;
TypeInfo* tiVec;

Frame* globalFrame;

set<void*> marked;
map<void*, int> cycles;
int cycleCounter;

List::Closure::Closure(Any _params, Any _body, Frame* _context) : body(_body), context(_context)
{
   while (_params.typeInfo == tiCons && _params.as<Cons*>() != nil)
   {
      params.push_back(Var(symbolText(car(_params))));
      _params = cdr(_params);
   }
   minArgs = maxArgs = params.size();
   if (_params.typeInfo == tiSymbol)
   {
      rest = Var(symbolText(_params));
      maxArgs = INT_MAX;
   }
}

Any& car(Cons* c)
{
   return c->car;
}

Any& cdr(Cons* c)
{
   return c->cdr;
}

Any& caar(Cons* c) { return car(car(c)); }
Any& cadr(Cons* c) { return cdr(car(c)); }
Any& cdar(Cons* c) { return car(cdr(c)); }
Any& cddr(Cons* c) { return cdr(cdr(c)); }

Any& caaar(Cons* c) { return car(car(car(c))); }
Any& caadr(Cons* c) { return cdr(car(car(c))); }
Any& cadar(Cons* c) { return car(cdr(car(c))); }
Any& caddr(Cons* c) { return cdr(cdr(car(c))); }
Any& cdaar(Cons* c) { return car(car(cdr(c))); }
Any& cdadr(Cons* c) { return cdr(car(cdr(c))); }
Any& cddar(Cons* c) { return car(cdr(cdr(c))); }
Any& cdddr(Cons* c) { return cdr(cdr(cdr(c))); }

Any& caaaar(Cons* c) { return car(car(car(car(c)))); }
Any& caaadr(Cons* c) { return cdr(car(car(car(c)))); }
Any& caadar(Cons* c) { return car(cdr(car(car(c)))); }
Any& caaddr(Cons* c) { return cdr(cdr(car(car(c)))); }
Any& cadaar(Cons* c) { return car(car(cdr(car(c)))); }
Any& cadadr(Cons* c) { return cdr(car(cdr(car(c)))); }
Any& caddar(Cons* c) { return car(cdr(cdr(car(c)))); }
Any& cadddr(Cons* c) { return cdr(cdr(cdr(car(c)))); }
Any& cdaaar(Cons* c) { return car(car(car(cdr(c)))); }
Any& cdaadr(Cons* c) { return cdr(car(car(cdr(c)))); }
Any& cdadar(Cons* c) { return car(cdr(car(cdr(c)))); }
Any& cdaddr(Cons* c) { return cdr(cdr(car(cdr(c)))); }
Any& cddaar(Cons* c) { return car(car(cdr(cdr(c)))); }
Any& cddadr(Cons* c) { return cdr(car(cdr(cdr(c)))); }
Any& cdddar(Cons* c) { return car(cdr(cdr(cdr(c)))); }
Any& cddddr(Cons* c) { return cdr(cdr(cdr(cdr(c)))); }

Any& first (Cons* c) { return car(c); }
Any& second(Cons* c) { return car(cdr(c)); }
Any& third (Cons* c) { return car(cdr(cdr(c))); }
Any& fourth(Cons* c) { return car(cdr(cdr(cdr(c)))); }

Any& drop1(Cons* c) { return cdr(c); }
Any& drop2(Cons* c) { return cdr(cdr(c)); }
Any& drop3(Cons* c) { return cdr(cdr(cdr(c))); }
Any& drop4(Cons* c) { return cdr(cdr(cdr(cdr(c)))); }

Cons* mylist() { return nil; }
Cons* mylist(Any i0) { return new Cons(i0, nil); }
Cons* mylist(Any i0, Any i1) { return new Cons(i0, new Cons(i1, nil)); }
Cons* mylist(Any i0, Any i1, Any i2) { return new Cons(i0, new Cons(i1, new Cons(i2, nil))); }
Cons* mylist(Any i0, Any i1, Any i2, Any i3) { return new Cons(i0, new Cons(i1, new Cons(i2, new Cons(i3, nil)))); }
Cons* mylistN(Any* args, int np)
{
   Cons* res(nil);
   while (np)
      res = new Cons(args[--np], res);
   return res;
}


Cons* cons(Any a, Cons* b)
{
   return new Cons(a, b);
}

Any pushBackCons(Cons* col, Any x)
{
   if (col)
      return new Cons(col->car, pushBack(col->cdr, x));
   else
      return new Cons(x, nil);
}

bool isNil(Any c)
{
   if (c.typeInfo == tiCons && (Cons*)c == nullptr) return true;
   return false;
}

bool isBlank(Any c)
{
   if (c.typeInfo == tiSymbol && *(Symbol*)c == symBlankArg) return true;
   return false;
}

int lengthCons(Cons* col)
{
   int r = 0;
   while (col)
   {
      col = col->cdr;
      ++r;
   }
   return r;
}

Any getElemCons(Cons* c, int n)
{
   while (n)
   {
      c = c->cdr;
      --n;
   }
   return c->car;
}

Cons* reverse(Cons* c)
{
   Cons* res = nil;
   while (c)
   {
      res = cons(c->car, res);
      c = c->cdr;
   }
   return res;
}

Any drop(Cons* c, int n)
{
   while (n)
   {
      c = c->cdr;
      --n;
   }
   return c;
}

template <class T> auto tassign  (T& a, T b) { return a = b; }
template <class T> T tadd     (T a, T b) { return a+b; }
template <class T> T tsubtract(T a, T b) { return a-b; }
template <class T> T tmultiply(T a, T b) { return a*b; }
template <class T> T tdivide  (T a, T b) { return a/b; }

template <class T> bool tequal    (T a, T b) { return a == b; }
template <class T> bool tnotEqual (T a, T b) { return a != b; }
template <class T> bool tless     (T a, T b) { return a <  b; }
template <class T> bool tlessEqual(T a, T b) { return a <= b; }
template <class T> bool tmore     (T a, T b) { return a >  b; }
template <class T> bool tmoreEqual(T a, T b) { return a >= b; }

template <class T> string ttoText(T a, int max) 
{
   std::stringstream ss; 
   ss << a;
   return ss.str();
}
template <class T, class U> U tto(T a) { return (U)a; }

template <class T>
void addMathOps(bool toTextOps = true)
{
   massign   .add(tassign   <T>);

   madd      .add(tadd      <T>);
   msubtract .add(tsubtract <T>);
   mmultiply .add(tmultiply <T>);
   mdivide   .add(tdivide   <T>);

   mless     .add(tless     <T>);
   mlessEqual.add(tlessEqual<T>);
   mmore     .add(tmore     <T>);
   mmoreEqual.add(tmoreEqual<T>);
   mequal    .add(tequal    <T>);
   mnotEqual .add(tnotEqual <T>);

   if (toTextOps)
      toTextAux .add(ttoText<T>);
}

string symbolText(Symbol* symbol)
{
   return symbol->name;
}

/*
toText -> findCycles
       -> toTextAux

toTextAux is a MultiMethod of

non-containers:
toTextCons
toTextBool
toTextSym
toTextOp
toTextParseEnd
toTextString
toTextLambda
toTextTypeInfo
toTextMultimethod
toTextVar

fallthrough = toTextAny which uses the member pointers in the typeInfo to display any type at all
*/

string toText(Any a, int max)
{
   marked.clear();
   cycles.clear();
   cycleCounter = 0;
   findCycles(a);
   return toTextAux(a, max);
}

template <class C>
void findCyclesCont(C* c)
{
   if (marked.count(c))
   {
      if (!cycles.count(c))
         cycles[c] = ++cycleCounter;
      return;
   }
   marked.insert(c);
   typename C::iterator b = c->begin();
   typename C::iterator e = c->end();
   for (typename C::iterator i = b; i != e; ++i)
      findCycles(&*i);
   marked.erase(c);
}

void findCyclesAny(Any)
{
}

string indent(string in, unsigned int n)
{
   for (int i = in.size() - 2; i >= 0; --i)
      if (in[i] == '\n')
         in.insert((unsigned int)i + 1, n, ' ');
   return in;
}

template <class C>
string toTextCont(C* c, int max = INT_MAX)
{
   string result("[");
   vector<string> elemStrs;
   typename C::iterator b = c->begin();
   typename C::iterator e = c->end();
   int len = 2;
   for (typename C::iterator i = b; i != e;)
   {
      string elemStr(toTextAux(*i, max - len));
      if (cycles.count(&*i))
         elemStr = string("#") + cycles[&*i] + "=" + elemStr;
      ++i;
      if (i != e) elemStr += ",";
      elemStrs.push_back(elemStr);
      len += elemStr.size();
   }
   int count = elemStrs.size();
   if (len > max)
   {
      for (int i = 0; i < count;)
      {
         result += indent(elemStrs[i], 1);
         ++i;
         if (i < count) result += "\n";
      }
   }
   else
   {
      for (int i = 0; i < count;)
      {
         result += elemStrs[i];
         ++i;
         if (i < count) result += " ";
      }
   }                    
   result += "]";
   return result;
}

string toTextInt(int n, int max)
{
   return TS+n;
}

string toTextIntR(int& n, int max)
{
   return TS+n;
}

string toTextBool(bool b, int max)
{
   return b ? "true" : "false";
}

string toTextCons(Cons* c, int max)
{
   if (c == nullptr) return "()";
   string result(TS+"("+toTextAux(c->car, max));
   Any a = c->cdr;
   for (;;)
   {
      if (isNil(a))
      {
         result += ")";
         return result;
      }
      if (a.typeInfo == tiCons)
      {
         c = a;
         result += " "+toTextAux(c->car, max);
         a = c->cdr;
      }
      else
      {
         result += " . "+toTextAux(a, max)+")";
         return result;
      }
   }
}

string toTextCons1(Cons* c, int max)
{
   return TS+"("+toTextAux(c->car, max)+" . "+toTextAux(c->cdr, max)+")";
}

string toTextSym(Symbol* s, int max)
{
   return s->name;
}

string toTextOp(Op* op, int max)
{
   return op->name;
}

string toTextParseEnd(ParseEnd p, int max)
{
   return "$$$ParseEnd$$$";
}

string toTextChar(char c, int max)
{
   return TS+"'"+string(1, c)+"'";
}

string toTextString(string s, int max)
{
   return TS+"\""+s+"\"";
}

string toTextLambda(List::Closure* l, int max)
{
   string result = "\\";
   for (int i = 0; i < (int)l->params.size(); ++i)
   {
      result += l->params[i].name + " ";
   }
   result += "-> ";
   result += toTextAux(l->body, max);
   return result;
}


string toTextTypeInfo(TypeInfo* ti, int max)
{
   string result;
   result += TS+"TypeInfo {\n   name = "+ti->name+"\n   size = "+ti->size+"\n   kind = ";
   switch (ti->kind)
   {
      case kNormal   :  result += "kNormal   \n"; break;
      case kPointer  :  result += "kPointer  \n"; break;
      case kReference:  result += "kReference\n"; break;
      case kFunction :  result += "kFunction \n"; break;
   }
   if (ti->of)
      result += TS+"   of = "+indent(toTextTypeInfo(ti->of), 3);
   for (int i = 0; i < (int)ti->args.size(); ++i)
      result += TS+"   arg["+i+"] = "+indent(toTextTypeInfo(ti->args[i]->typeInfo), 3);
   if (ti->linear.size())
   {
      result += TS+"   linear = ";
      for (int i = 0; i < (int)ti->linear.size(); ++i)
      {
         if (i > 0) result += ", ";
         result += ti->linear[i]->base->name;
      }
   }
   result += "}\n";
   return result;
}

string toTextFrame(Frame* f, int max)
{
   if (f == nullptr)
      return "nullptr";
   else if (f == globalFrame)
      return "global";
   else if (f == (Frame*)0xCDCDCDCD)
      return "0xCDCDCDCD";
   else if (f->visible)
      return TS+"Frame { "+toTextAny(f, max)+" }";
   else
      return TS+"HiddenFrame";
}

string toTextMultimethod(MultimethodAny m, int max)
{
   return TS+"Multimethod("+m.name+")";
}

string toTextVar(Var v, int max)
{
   return TS+v.name+" = "+toText(v.value, max);
}

string toTextAny(Any a, int max)
{
   ostringstream o;
   if (a.typeInfo->kind == kFunction)
      o << a.typeInfo->getName() << " = " << *(int**)a.ptr;
   else
   {
      while (a.prDepth() > 0)
      {
         o << *(int**)a.ptr;
         switch (a.typeInfo->kind)
         {
            case kReference:
               o << "&";
               break;
            case kPointer:
               o << "*";
               break;
         }
         o << " -> ";
         if (*(void**)a.ptr == nullptr)
         {
            o << "***";
            return o.str();
         }
         if (a.prDepth() == 0) break;
         a = a.derp();
      }
#if 1
      if (a.prDepth() == 0)
      {
      }
      if (a.prDepth() == 0)
      {
         o << a.typeInfo->getName() << " ";
         o << "{";
         for (int i = 0; i < (int)a.typeInfo->args.size(); ++i)
         {
            if (i > 0) o << ", ";
            o << a.typeInfo->args[i]->symbol->name << "=";
            //Any v(a.typeInfo->of->args[i]->ptrToMember(a));
            //Any v(a[i]);
            //v = v.derp();
            //v.showhex();
            o << toTextAux(a[i].derp(), max);
            //vars.push_back(Var(a.typeInfo->args[i]->name, a.typeInfo->args[i]->ptrToMember(a)));
         }
         o << "}";
      }
#else
      Vec vars;
      TypeInfo* ti = a.typeInfo;
      vars.push_back(ti->name);
      for (int l = 0; l < (int)ti->linear.size(); ++l)
      {
         TypeInfo* lti = ti->linear[l]->base;
         for (int i = 0; i < (int)lti->args.size(); ++i)
         {
            Any val(lti->args[i]->ptrToMember(a));
            vars.push_back(Var(lti->args[i]->symbol->name, val));
         }
      }
      return toText(vars);
#endif
   }
   return o.str();
}
void insertElemVec(Vec* vec, VecI at, Any val)
{
   vec->insert(*at, val);
}

template <class C> void                    tinsertElemIX  (C* col, typename C::iterator i, typename C::value_type elem) { col->insert(i, elem);                }
template <class C> void                    teraseElemIX   (C* col, typename C::iterator i                             ) { col->erase (i      );                }
template <class C> void                    tinsertElemIntX(C* col, int n, typename C::value_type elem                 ) { col->insert(col->begin() + n, elem); }
template <class C> void                    teraseElemIntX (C* col, int n                                              ) { col->erase (col->begin() + n      ); }
template <class C> void                    tinsertRangeIX (C* col, typename C::iterator d, Any s0, Any s1             ) { col->insert(d, s0, s1);              }
template <class C> void                    teraseRangeIX  (C* col, typename C::iterator i0, typename C::iterator i1   ) { col->erase (i0, i1);                 }
template <class C> void                    tsetElemX      (C* col, int n, typename C::value_type elem                 ) { (*col)[n] = elem;                    }
template <class C> typename C::value_type& tgetElem       (C* col, int n                                              ) { return (*col)[n];                    }
template <class C> typename C::iterator    titerBegin     (C* col                                                     ) { return col->begin();                 }
template <class C> typename C::iterator    titerEnd       (C* col                                                     ) { return col->end();                   }
template <class C> C*                      tpushFrontX    (C* col, typename C::value_type elem                        ) { col->insert(col->begin(), elem); return col; }
template <class C> C*                      tpushBackX     (C* col, typename C::value_type elem                        ) { col->insert(col->end  (), elem); return col; }

template <class C> C*                      tappend        (C* a, C* b) { C* res = new C(*a); res->insert(res->end(), b->begin(), b->end()); return res; }

template <class C> typename C::value_type  tfold          (Any fn, typename C::value_type zero, C* col) { typename C::value_type t(zero); for (auto v : *col) t = fn(t, v); return t; }

template <class I> typename I::value_type& tderef         (I i                                                        ) { return *i;                           }
template <class I> I&                      iinc           (I i                                                        ) { return ++i;                          }
template <class I> I&                      idec           (I i                                                        ) { return --i;                          }
template <class I> bool                    iequal         (I i, I j                                                   ) { return i == j;                       }
template <class I> bool                    inotEqual      (I i, I j                                                   ) { return i != j;                       }

template <class C>
TypeInfo* addContainer(bool textOps = true)
{
   typedef typename C::iterator I;
   massign     .add(tassign        <C>);
   massign     .add(tassign        <I>);
   insertElemX .add(tinsertElemIX  <C>);
   insertElemX .add(tinsertElemIntX<C>);
   eraseElemX  .add(teraseElemIX   <C>);
   eraseElemX  .add(teraseElemIntX <C>);
   setElemX    .add(tsetElemX      <C>);
   getElem     .add(tgetElem       <C>);
   iterBegin   .add(titerBegin     <C>);
   iterEnd     .add(titerEnd       <C>);

   append      .add(tappend        <C>);
   fold        .add(tfold          <C>);

   mderef      .add(tderef   <I>);
   minc        .add(iinc     <I>);
   mdec        .add(idec     <I>);
   mequal      .add(iequal   <I>);
   mnotEqual   .add(inotEqual<I>);
   if (textOps)
   {
      toTextAux   .add(toTextCont<C>);
   }

   findCycles  .add(findCyclesCont<C>);

   return getTypeAdd<C>();

   vector<int> x;
   vector<int>::iterator i = x.begin();
   i.operator++();

}

template <class C>
TypeInfo* addContainer1()
{
   typedef typename C::value_type     T;
   typedef typename C::iterator       I;
   typedef typename C::const_iterator CI;
   typedef typename C::size_type      S;

   typedef vector<int>::iterator (vector<int>::*InsertElem)(vector<int>::const_iterator, const int&);
   //InsertElem f = &vector<int>::insert;
   //C c;
   //c.insert(0, 0);
   insertElemX .add((I   (C::*)(CI, const T&)      )&C::insert    );
   eraseElemX  .add((I   (C::*)(CI          )      )&C::erase     );
   getElem     .add((T&  (C::*)(S           )      )&C::operator[]);
   iterBegin   .add((I   (C::*)(            )      )&C::begin     );
   iterEnd     .add((I   (C::*)(            )      )&C::end       );

   mderef      .add((T&  (I::*)(            )      )&I::operator* );
   minc        .add((I&  (I::*)(            )      )&I::operator++);
   mdec        .add((I&  (I::*)(            )      )&I::operator--);
   mequal      .add((bool(I::*)(const CI&   ) const)&I::operator==);
   mnotEqual   .add((bool(I::*)(const CI&   ) const)&I::operator!=);

   vector<Any> c;
   vector<Any>::const_iterator ci;

   return getTypeAdd<C>();
}

Cons* iterBeginCons(Cons* c)
{
   return c;
}

const Cons* iterEndCons(Cons* c)
{
   return nil;
}

Any derefConsIter(Cons* c)
{
   return c->car;
}

Cons*& incConsIter(Cons*& c)
{
   c = c->cdr;
   return c;
}

bool equalConsIter(Cons* l, Cons* r)
{
   return l == r;
}

bool notEqualConsIter(Cons* l, Cons* r)
{
   return l != r;
}

void addList()
{
   iterBegin   .add(iterBeginCons);
   iterEnd     .add(iterEndCons);
   mderef      .add(derefConsIter);
   minc        .add(incConsIter);
   mequal      .add(equalConsIter);
   mnotEqual   .add(notEqualConsIter);
}

bool equalSymbol(Symbol* a, Symbol* b)
{
	return a == b;;
}

bool notEqualSymbol(Symbol* a, Symbol* b)
{
	return a != b;
}

/*
template <class T>
void addContainer()
{
   insertElem.add((T::*(T::iterator, &T::insert);
}
*/
void addGlobal(string name, Any f)
{
   globalFrame->vars.push_back(Var(name, f));
}

template <class ParseResult>
void addGlobal(string name, Multimethod<ParseResult>& m)
{
   m.name = name;
   globalFrame->vars.push_back(Var(name, &m));
}

bool isNum(Any a)
{
   return a.typeInfo == tiInt || a.typeInfo == tiInt64 || a.typeInfo == tiFloat || a.typeInfo == tiDouble;
}

Cons* mapCons(Any f, Cons* c)
{
   Cons *res = nil;
   Cons **r  = &res;
   while (c != nil)
   {
      *r = cons(f(c->car), nil);
      r = &(*r)->cdr.as<Cons*>();
      c = c->cdr;
   }
   return res;
}

Any List::closureDeleg(Any* lambdaP, Any* params, int np)
{
   Vec vp(params, params + np);
   return List::apply(*lambdaP, &vp);
}

Vec* toVector(Cons* c)
{
   Vec* v = new Vec;
   while (c != nil)
   {
      v->push_back(c->car);
      c = c->cdr;
   }
   return v;
}

Gen toGen(Vec* c)
{
   return Gen();
}   

Vec* fileNames(string path)
{
   Vec* result = new Vec;
#ifdef WINDOWS
   struct _finddata_t fileData;
   intptr_t hFile;

   // Find first .c file in current directory 
   if ((hFile = _findfirst((path + "\\*.*").c_str(), &fileData)) == -1L)
   {
      //no files
   }
   else
   {
      do
      {
         /*
         char buffer[30];
         printf( ( fileData.attrib & _A_RDONLY ) ? " Y  " : " N  " );
         printf( ( fileData.attrib & _A_HIDDEN ) ? " Y  " : " N  " );
         printf( ( fileData.attrib & _A_SYSTEM ) ? " Y  " : " N  " );
         printf( ( fileData.attrib & _A_ARCH )   ? " Y  " : " N  " );
         ctime_s( buffer, _countof(buffer), &fileData.time_write );
         printf( " %-12s %.24s  %9ld\n", fileData.name, buffer, fileData.size );
         */
         result->push_back(string(fileData.name));
      } while (_findnext(hFile, &fileData) == 0);
      _findclose(hFile);
   }
#endif
   return result;
}

TypeInfo* typeOf(Any a)
{
   return a.typeInfo;
}

Any applyC(Any a, Any b)
{
   return a(b);
}

List::Closure* compose(Any a, Any b)
{
   List::Closure* lambda = new List::Closure;
   lambda->params.push_back(Var("a"));
   lambda->body = mylist(a, mylist(b, getSymbol("a")));
   lambda->context = nullptr;
   lambda->minArgs = 1;
   lambda->maxArgs = 1;
   return lambda;
}

ostream& operator<<(ostream& Stream, Any any)
{
   operator<<(Stream, toText(any));
   return Stream;
}

/*
Array concatArrays(Any inp, int dim)
{
}
*/
void setup()
{
   tiAny       = getTypeAdd<Any       >();
   tiParseEnd  = getTypeAdd<ParseEnd  >();
   tiChar      = getTypeAdd<char      >();
   tiInt8      = getTypeAdd<int8_t    >();
   tiInt16     = getTypeAdd<int16_t   >();
   tiInt       = getTypeAdd<int32_t   >();
   tiInt64     = getTypeAdd<int64_t   >();
   tiUInt8     = getTypeAdd<uint8_t   >()->setName("uint8");
   tiUInt16    = getTypeAdd<uint16_t  >()->setName("uint16");
   tiUInt      = getTypeAdd<uint32_t  >()->setName("uint32");
   tiUInt64    = getTypeAdd<uint64_t  >()->setName("uint64");
   tiFloat     = getTypeAdd<float     >();
   tiDouble    = getTypeAdd<double    >();
   tiSymbol    = getTypeAdd<Symbol   *>();
   tiCons      = getTypeAdd<Cons     *>();
   tiListClosure   = getTypeAdd<List::Closure*>();
   tiString    = getTypeAdd<string    >()->setName("string");
   tiVarRef    = getTypeAdd<VarRef   *>();
   tiVarFunc   = getTypeAdd<VarFunc   >();
   tiPartApply = getTypeAdd<PartApply*>();
   tiTypeInfo  = getTypeAdd<TypeInfo *>();

   addMember(&Cons::car, "car");
   addMember(&Cons::cdr, "cdr");

   addMember(&VarRef::name  , "name");
   addMember(&VarRef::frameC, "frameC");
   addMember(&VarRef::varC  , "varC");

   addMember(&Frame::vars   , "vars");
   addMember(&Frame::context, "context");

   addMember(&TypeInfo::args       , "args");
   addMember(&TypeInfo::bases      , "bases");
   addMember(&TypeInfo::copyCon    , "copyCon");
   addMember(&TypeInfo::count      , "count");
   addMember(&TypeInfo::delegPtr   , "delegPtr");
   addMember(&TypeInfo::derived    , "derived");
   addMember(&TypeInfo::fullName   , "fullName");
   addMember(&TypeInfo::kind       , "kind");
   addMember(&TypeInfo::linear     , "linear");
   addMember(&TypeInfo::member     , "member");
   addMember(&TypeInfo::multimethod, "multimethod");
   addMember(&TypeInfo::name       , "name");
   addMember(&TypeInfo::nKind      , "nKind");
   addMember(&TypeInfo::number     , "number");
   addMember(&TypeInfo::of         , "of");
   addMember(&TypeInfo::size       , "size");

   tiNumber   = new TypeInfo("number");
   tiInteger  = new TypeInfo("integer");
   tiSigned   = new TypeInfo("signed");
   tiUnsigned = new TypeInfo("unsigned");
   tiFloating = new TypeInfo("floating");

   addTypeLink(tiNumber  , tiInteger );
   addTypeLink(tiNumber  , tiFloating);
   addTypeLink(tiInteger , tiSigned  );
   addTypeLink(tiInteger , tiUnsigned);
   addTypeLink(tiSigned  , tiInt8    );
   addTypeLink(tiSigned  , tiInt16   );
   addTypeLink(tiSigned  , tiInt     );
   addTypeLink(tiSigned  , tiInt64   );
   addTypeLink(tiUnsigned, tiUInt8   );
   addTypeLink(tiUnsigned, tiUInt16  );
   addTypeLink(tiUnsigned, tiUInt    );
   addTypeLink(tiUnsigned, tiUInt64  );
   addTypeLink(tiFloating, tiFloat   );
   addTypeLink(tiFloating, tiDouble  );

   tiInt   ->nKind = kSigned;
   tiInt64 ->nKind = kSigned;
   tiInt   ->nKind = kUnsigned;
   tiInt64 ->nKind = kUnsigned;
   tiFloat ->nKind = kFloat;
   tiDouble->nKind = kFloat;

   toTextAux.add(toTextCons);
   toTextAux.add(toTextBool);
   toTextAux.add(toTextSym);
   toTextAux.add(toTextOp);
   toTextAux.add(toTextParseEnd);
   toTextAux.add(toTextChar);
   toTextAux.add(toTextString);
   toTextAux.add(toTextLambda);
   toTextAux.add(toTextTypeInfo);
   toTextAux.add(toTextFrame);
   toTextAux.add(toTextMultimethod);
   toTextAux.add(toTextVar);

   pushBack.add(pushBackCons);
   mequal.add(equalSymbol);
   mnotEqual.add(notEqualSymbol);

   addContainer<vector<int> >()->setName("vector<int>");
   tiVec = addContainer<vector<Any> >()->setName("vector<Any>");
   addContainer<deque <int> >()->setName("deque<int>");
   addContainer<deque <Var> >()->setName("deque<Var>");
   addContainer<string      >(false)->setName("string");
   addList();

   globalFrame = new Frame;
   globalFrame->context = nullptr;
   globalFrame->visible = false;
   addMathOps<   char>(false);
   addMathOps< int8_t>();
   addMathOps<int16_t>();
   addMathOps<int32_t>();
   addMathOps<int64_t>();
   addMathOps< double>();
   addMathOps<  float>();
   addGlobal("set!"    , massign   );
   addGlobal("+"       , madd      );
   addGlobal("-"       , msubtract );
   addGlobal("*"       , mmultiply );
   addGlobal("/"       , mdivide   );
   addGlobal("<"       , mless     );
   addGlobal("<="      , mlessEqual);
   addGlobal(">"       , mmore     );
   addGlobal(">="      , mmoreEqual);
   addGlobal("=="      , mequal    );
   addGlobal("!="      , mnotEqual );
   addGlobal("/="      , mnotEqual );
   addGlobal("car"     , car       );
   addGlobal("cdr"     , cdr       );
   addGlobal("cons"    , cons      );
   addGlobal("first"   , first     );
   addGlobal("second"  , second    );
   addGlobal("third"   , third     );
   addGlobal("fourth"  , fourth    );
   addGlobal("list"    , VarFunc   (mylistN, 0, INT_MAX));
   addGlobal("toText"  , toText    );
   addGlobal("map"     , mapCons   );

   addGlobal("insertElemX" , insertElemX );
   addGlobal("eraseElemX"  , eraseElemX  );
   addGlobal("getElem"     , getElem     );
   addGlobal("setElemX"    , setElemX    );
   addGlobal("iterFront"   , iterBegin   );
   addGlobal("iterEnd"     , iterEnd     );

   addGlobal("++"          , append      );
   addGlobal("fold"        , fold        );

   addGlobal("deref"       , mderef      );
   addGlobal("inc"         , minc        );
   addGlobal("dec"         , mdec        );
   addGlobal("toVector"    , toVector    );
   addGlobal("typeOf"      , typeOf      );

   addGlobal("$"           , applyC      );
   addGlobal("."           , compose     );
   /*
   addGlobal("
   addGlobal("
   addGlobal("
   addGlobal("
   addGlobal("
   addGlobal("
   addGlobal("
   addGlobal("
   addGlobal("
   addGlobal("
   */
   length.add(lengthCons);
   addGlobal("length", length);
   addGlobal("names", fileNames);
   toTextAux   .add(toTextAny);
   findCycles  .add(findCyclesAny);
   initParser2();
   for (auto &p : typeMap)
      p.second->doC3Lin();

   for (auto &p : typeMap)
      p.second->allocate();
}

/*
int func(int a, int b)
{
   return a + b;
}

print func(2, 3);

struct Func
{
   int a;
   int b;
   int operator()()
   {
      return a + b;
   }
};

print Func(2, 3)();
*/
