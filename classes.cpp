// Copyright (c) 2023 Brett Curtis
#include "stdafx.h"
#include "classes.h"
#include "Parser.h"
#include "Parser2.h"
#include "InterpL.h"
#include <io.h>

using namespace std;

Symbols symbols;
Symbol* symBlankArg = getSymbol("_", 0, 0);

Symbol* getSymbol(string name, int64_t group, int64_t id)
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

Multimethod<Any>    convert     ("convert");
Multimethod<Any>    massign     ("assign");
Multimethod<string> toTextAux   ("toTextAux");
Multimethod<void>   findCycles  ("findCycles");
Multimethod<Any>    pushBack    ("pushBack");
Multimethod<void>   insertElemX ("insertElemX");
Multimethod<void>   eraseElemX  ("eraseElemX");
Multimethod<Any>    getElem     ("getElem");
Multimethod<void>   setElemX    ("setElemX");
Multimethod<Any>    iterBegin   ("iterBegin");
Multimethod<Any>    iterEnd     ("iterEnd");

Multimethod<Any>    append      ("append");
Multimethod<Vec>    mtoVec      ("toVec");
Multimethod<Any>    mfromVec    ("fromVec");
Multimethod<Any>    fold        ("fold");
Multimethod<Any>    mmap        ("map");

Multimethod<Any>    mderef      ("deref");
Multimethod<Any>    minc        ("inc");
Multimethod<Any>    mdec        ("dec");
Multimethod<size_t> length      ("length");
Multimethod<Any>    madd        ("+");
Multimethod<Any>    msubtract   ("-");
Multimethod<Any>    mmultiply   ("*");
Multimethod<Any>    mdivide     ("/");
Multimethod<bool>   mless       ("<");
Multimethod<bool>   mlessEqual  ("<=");
Multimethod<bool>   mmore       (">");
Multimethod<bool>   mmoreEqual  (">=");
Multimethod<bool>   mequal      ("==");
Multimethod<bool>   mnotEqual   ("!=");

bool operator==(Any a, Any b) { return mequal(a, b); }
bool operator!=(Any a, Any b) { return mnotEqual(a, b); }
Any  operator++(Any a)        { return minc(a); }
Any  operator* (Any a)        { return mderef(a); }


TypeInfo* tiAny     ;
TypeInfo* tiPH      ;
TypeInfo* tiMissingArg;
TypeInfo* tiSymbol  ;
TypeInfo* tiCons    ;
TypeInfo* tiChar    ;
TypeInfo* tiInt8    ;
TypeInfo* tiInt16   ;
TypeInfo* tiInt32   ;
TypeInfo* tiInt64   ;
TypeInfo* tiInt     ;
TypeInfo* tiUInt8   ;
TypeInfo* tiUInt16  ;
TypeInfo* tiUInt32  ;
TypeInfo* tiUInt64  ;
TypeInfo* tiUInt    ;
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
TypeInfo* tiBind    ;
TypeInfo* tiTypeInfo;
TypeInfo* tiVec;

Frame* globalFrame;

namespace Struct
{
   Any globalFrame;
}

set<void*> marked;
map<void*, int64_t> cycles;
int64_t cycleCounter;

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
Cons* mylistN(Any* args, int64_t np)
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

int64_t lengthCons(Cons* col)
{
   int64_t r = 0;
   while (col)
   {
      col = col->cdr;
      ++r;
   }
   return r;
}

Any getElemCons(Cons* c, int64_t n)
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

Any drop(Cons* c, int64_t n)
{
   while (n)
   {
      c = c->cdr;
      --n;
   }
   return c;
}

template <class From, class To> To tconvert (From from) { return from; }
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

template <class T> string ttoText(T a, int64_t max) 
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

string toText(Any a, int64_t max)
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

string toTextInt(int64_t n, int64_t max)
{
   return TS+n;
}

string toTextIntR(int64_t& n, int64_t max)
{
   return TS+n;
}

string toTextBool(bool b, int64_t max)
{
   return b ? "true" : "false";
}

string toTextCons(Cons* c, int64_t max)
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
         result += TS+" "+toTextAux(c->car, max);
         a = c->cdr;
      }
      else
      {
         result += TS+" . "+toTextAux(a, max)+")";
         return result;
      }
   }
}

string toTextCons1(Cons* c, int64_t max)
{
   return TS+"("+toTextAux(c->car, max)+" . "+toTextAux(c->cdr, max)+")";
}

string toTextSym(Symbol* s, int64_t max)
{
   return s->name;
}

string toTextOp(Op* op, int64_t max)
{
   return op->name;
}

string toTextParseEnd(ParseEnd p, int64_t max)
{
   return "$$$ParseEnd$$$";
}

string toTextChar(char c, int64_t max)
{
   return TS+"'"+string(1, c)+"'";
}

string toTextString(string s, int64_t max)
{
   return TS+"\""+s+"\"";
}

string toTextLambda(List::Closure* l, int64_t max)
{
   string result = "\\";
   for (size_t i = 0; i < l->params.size(); ++i)
   {
      result += l->params[i].name + " ";
   }
   result += "-> ";
   result += toTextAux(l->body, max);
   return result;
}

string indent(string in, uint64_t n)
{
   for (int64_t i = in.size() - 2; i >= 0; --i)
      if (in[i] == '\n')
         in.insert((uint64_t)i + 1, n, ' ');
   return in;
}

string toTextTypeInfo(TypeInfo* ti, int64_t max)
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
   for (size_t i = 0; i < ti->members.size(); ++i)
//      result += TS+"   arg["+i+"] = "+indent(toTextTypeInfo(ti->members[i]->typeInfo), 3);
   if (ti->main.linear.size())
   {
      result += TS+"   linear = ";
      for (size_t i = 0; i < ti->main.linear.size(); ++i)
      {
         if (i > 0) result += ", ";
         result += ti->main.linear[i]->name;
      }
   }
   result += "}\n";
   return result;
}

string toTextFrame(Frame* f, int64_t max)
{
   if (f == nullptr)
      return "nullptr";
   else if (f == globalFrame)
      return "global";
   else if (f == (Frame*)0xCDCDCDCDCDCDCDCD)
      return "0xCDCDCDCDCDCDCDCD";
   else if (f->visible)
      return TS+"Frame { "+toTextAny(f, max)+" }";
   else
      return TS+"HiddenFrame";
}

string toTextVar(Var v, int64_t max)
{
   return TS+v.name+" = "+toText(v.value, max);
}

string toTextArray(Array<string>& array, int64_t max)
{
   Array<size_t> cellLengths;
   Array<size_t> cellHeights;
   vector<size_t> colWidths(array.sizes[0], 1);
   vector<size_t> rowHeights(array.sizes[1]);
   cellLengths.sizes = array.sizes;
   cellHeights.sizes = array.sizes;
   cellLengths.alloc();
   cellHeights.alloc();
   size_t* cellLength = cellLengths.begin();
   for (auto &cellString : array)
      *cellLength++ = cellString.size();
   size_t* cellHeight = cellHeights.begin();
   for (auto &cellLength : cellLengths)
      *cellHeight++ = cellLength;
   for (int64_t y = 0; y < array.sizes[1]; ++y)
   {
      int64_t maxh = 0;
      for (int64_t x = 0; x < array.sizes[0]; ++x)
      {
         //int64_t h1 = (cellLengths({maxx, y}) + colWidths[maxx] - 1) / colWidths[maxx];
         //cellHeights({x, y}) = h1;
         int64_t h1 = cellHeights[{x, y}];
         if (h1 > maxh)
            maxh = h1;
      }
      rowHeights[y] = maxh;
   }
   max -= array.sizes[0];
   for (int64_t i = array.sizes[0]; i < max; ++i)
   {
      vector<int64_t> total(array.sizes[0], 0);
      for (int64_t y = 0; y < array.sizes[1]; ++y)
      {
         int64_t maxh = 0;
         for (int64_t x = 0; x < array.sizes[0]; ++x)
         {
            int64_t h1 = cellHeights[{x, y}];
            if (h1 > maxh)
               maxh = h1;
         }
         if (maxh > 1)
            for (int64_t x = 0; x < array.sizes[0]; ++x)
               if (cellHeights[{x, y}] >= maxh) ++total[x];
      }
      int64_t maxt = 0;
      int64_t maxx = -1;
      for (int64_t x = 0; x < array.sizes[0]; ++x)
      {
         int64_t t1 = total[x];
         if (t1 > maxt)
         {
            maxt = t1;
            maxx = x;
         }
      }
      if (maxx >= 0)
         ++colWidths[maxx];
      else
         break;
      for (int64_t y = 0; y < array.sizes[1]; ++y)
      {
         cellHeights[{maxx, y}] = (cellLengths[{maxx, y}] + colWidths[maxx] - 1) / colWidths[maxx];
         int64_t maxh = 0;
         for (int64_t x = 0; x < array.sizes[0]; ++x)
         {
            int64_t h1 = cellHeights[{x, y}];
            if (h1 > maxh)
               maxh = h1;
         }
         rowHeights[y] = maxh;
      }
   }
   string res;
   for (int64_t y = 0; y < array.sizes[1]; ++y)
   {
      int64_t maxh = rowHeights[y];
      for (int64_t h = 0; h < maxh; ++h)
      {
         for (int64_t x = 0; x < array.sizes[0]; ++x)
         {
            int64_t b = colWidths[x] * h;
            int64_t e = b + colWidths[x];
            int64_t l = cellLengths[{x, y}];
            if (b > l)
               res += string(e - b, ' ');
            else if (e > l)
               res += array[{x, y}].substr(b, colWidths[x]) + string(e - l, ' ');
            else
               res += array[{x, y}].substr(b, colWidths[x]);
            if (x < array.sizes[0] - 1)
               res += "|";
         }
         res += "\n";
      }
   }
   return res;
}

string toTextMultimethod(MMBase &mm, int64_t max)
{
   Array<string> types;
   Array<string> methods;
   int64_t maxti = 0;
   for (int64_t pi = 0; pi < mm.maxArgs; ++pi)
      if (mm.paramTypeLists[pi].size() > maxti) maxti = mm.paramTypeLists[pi].size();
   types.sizes = { mm.maxArgs + 1, maxti };
   types.alloc();
   methods.sizes = mm.table.sizes;
   methods.alloc();
   for (int64_t pti = 0; pti < maxti; ++pti)
      types[{0, pti}] = string(1, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[pti]);
   for (int64_t pi = 0; pi < mm.maxArgs; ++pi)
      for (int64_t pti = 0; pti < mm.paramTypeLists[pi].size(); ++pti)
         types[{pi + 1, pti}] = mm.paramTypeLists[pi][pti]->getName();
   for (int64_t tableIndex = 0; tableIndex < mm.table.totalSize; ++tableIndex)
   {
      vector<int64_t> indices = mm.table.getTableIndices(tableIndex);
      Any m = mm.table[indices];
      string res1;
      if (m.callable())
      {
         for (int64_t pi = 0; pi < m.maxArgs(); ++pi)
         {
            int64_t n = m.paramType(pi, mm.useReturnT)->mmpindices[mm.mmindex][pi];
            res1 += string(1, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[n]);
         }
      }
      else if (m.typeInfo == tiVec)
      {
         res1 = "&";
      }
      methods[indices] = res1;
   }
   return TS+"Multimethod("+mm.name+")\n"+toTextAux(types, max)+toTextAux(methods, max);
}

string shortPtr(void* ptr)
{
   ostringstream o;
   o << (int64_t*)ptr;
   return o.str().substr(12, 4);
}
#if 1
string toTextAny3(AnyBase& a, int64_t max)
{
   ostringstream o;
   if (a.typeInfo->kind == kFunction)
   {
      o << shortPtr(*(void**)a.payload) << "*->" << a.typeInfo->getName();
   }
   else
   {
      if (a.typeInfo->kind == kReference || a.typeInfo->kind == kPointer)
      {
         if (*(void**)a.payload == nullptr)
         {
            o << "nullptr";
            return o.str();
         }
         o << shortPtr(*(void**)a.payload);
         //o << shortPtr(payload);
         switch (a.typeInfo->kind)
         {
         case kReference: o << "&"; break;
         case kPointer  : o << "*"; break;
         }
         AnyBase b = a.derp();
         o << "->" << toTextAny3(b, max);
      }
      else if (a.typeInfo == tiAny)
      {
         //o << shortPtr(*(void**)payload);
         AnyBase b = a.deany();
         o << shortPtr(b.payload);
         o << ":Any->" << toTextAny3(b, max);
      }
      else
      {
         Any& b(static_cast<Any&>(a));
         o << b.typeInfo->getName();
         o << "{ ";
         for (size_t i = 0; i < b.typeInfo->allMembers.size(); ++i)
         {
            Member* m = b.typeInfo->allMembers[i];
            if (i > 0) o << ", ";
            string name = m->symbol->name;
            if (name == "linear")
               __debugbreak();
            o << name << "=";
            //Any v(a.typeInfo->of->members[i]->ptrToMember(a));
            //Any v(a[i]);
            //v = v.derp();
            //v.showhex();
            o << toTextAux(b.getMember(m), max);
            //vars.push_back(Var(a.typeInfo->members[i]->name, a.typeInfo->members[i]->ptrToMember(a)));
         }
         o << " }";
      }
   }
   return o.str();
}
#endif
string toTextAny2(TypeInfo* typeInfo, void* payload, int64_t max)
{
   ostringstream o;
   if (typeInfo->kind == kFunction)
   {
      o << shortPtr(*(void**)payload) << "*->" << typeInfo->getName();
   }
   else
   {
      if (typeInfo->kind == kReference || typeInfo->kind == kPointer)
      {
         if (*(void**)payload == nullptr)
         {
            o << "nullptr";
            return o.str();
         }
         o << shortPtr(*(void**)payload);
         //o << shortPtr(payload);
         switch (typeInfo->kind)
         {
         case kReference: o << "&"; break;
         case kPointer  : o << "*"; break;
         }
         o << "->" << toTextAny2(typeInfo->of, *(void**)payload, max);
      }
      else if (typeInfo == tiAny)
      {
         //o << shortPtr(*(void**)payload);
         Any* a = (Any*)payload;
         o << shortPtr(a->payload);
         o << ":Any->" << toTextAny2(a->typeInfo, a->payload, max);
      }
      else
      {
         Any a(typeInfo, payload);
         o << a.typeInfo->getName();
         o << "{ ";
         for (size_t i = 0; i < a.typeInfo->allMembers.size(); ++i)
         {
            if (i > 0) o << ", ";
            string name = a.typeInfo->allMembers[i]->symbol->name;
            if (name == "linear")
               __debugbreak();
            o << name << "=";
            //Any v(a.typeInfo->of->members[i]->ptrToMember(a));
            //Any v(a[i]);
            //v = v.derp();
            //v.showhex();
            o << toTextAux(a.getMemberRef(i).derp(), max);
            //vars.push_back(Var(a.typeInfo->members[i]->name, a.typeInfo->members[i]->ptrToMember(a)));
         }
         o << " }";
      }
   }
   return o.str();
}

string toTextAny1(const Any& a, int64_t max)
{
   Any& b = const_cast<Any&>(a);
   return toTextAny2(b.typeInfo, b.payload, max);
}

string toTextAny(Any a, int64_t max)
{
#if 1
   //AnyBase& b = (AnyBase&)(a);
   return toTextAny3(a, max);
#else
   return toTextAny2(a.typeInfo, a.payload, max);
#endif
}

void insertElemVec(Vec* vec, VecI at, Any val)
{
   vec->insert(*at, val);
}

template <class C> void                    tinsertElemIX  (C* col, typename C::iterator i, typename C::value_type elem) { col->insert(i, elem);                }
template <class C> void                    teraseElemIX   (C* col, typename C::iterator i                             ) { col->erase (i      );                }
template <class C> void                    tinsertElemIntX(C* col, int64_t n, typename C::value_type elem             ) { col->insert(col->begin() + n, elem); }
template <class C> void                    teraseElemIntX (C* col, int64_t n                                          ) { col->erase (col->begin() + n      ); }
template <class C> void                    tinsertRangeIX (C* col, typename C::iterator d, Any s0, Any s1             ) { col->insert(d, s0, s1);              }
template <class C> void                    teraseRangeIX  (C* col, typename C::iterator i0, typename C::iterator i1   ) { col->erase (i0, i1);                 }
template <class C> void                    tsetElemX      (C* col, int64_t n, typename C::value_type elem             ) { (*col)[n] = elem;                    }
template <class C> typename C::value_type& tgetElem       (C* col, int64_t n                                          ) { return (*col)[n];                    }
template <class C> typename C::value_type  tgetElem1      (C* col, int64_t n                                          ) { return (*col)[n];                    }
template <class C> typename C::iterator    titerBegin     (C* col                                                     ) { return col->begin();                 }
template <class C> typename C::iterator    titerEnd       (C* col                                                     ) { return col->end();                   }
template <class C> C*                      tpushFrontX    (C* col, typename C::value_type elem                        ) { col->insert(col->begin(), elem); return col; }
template <class C> C*                      tpushBackX     (C* col, typename C::value_type elem                        ) { col->insert(col->end  (), elem); return col; }

template <class C> C*                      tappend        (C*  a, C*  b) { C* res = new C(*a); res->insert(res->end(), b->begin(), b->end()); return res; }
template <class C> C*                      tappend1       (Any a, Any b) { C* res = new C(*a); res->insert(res->end(), iterBegin(b), iterEnd(b)); return res; }

template <class C> Vec*                    ttoVec         (C* col) { return new Vec(col->begin(), col->end()); }
template <class C> C*                      tfromVec       (Vec* v) { return new C(v->begin(), v->end()); }

template <class C> typename C::value_type  tfold          (Any fn, typename C::value_type zero, C* col) { typename C::value_type t(zero); for (auto v : *col) t = fn(t, v); return t; }
template <class C> C*                      tmap           (Any fn, C* col) { C* res = new C(col->size(), C::value_type()); transform(col->begin(), col->end(), res->begin(), fn); return res; }

template <class I> typename I::value_type& tderef         (I i                                                        ) { return *i;                           }
template <class I> typename I::value_type  tderef1        (I i                                                        ) { return *i;                           }
template <class I> I&                      iinc           (I i                                                        ) { return ++i;                          }
template <class I> I&                      idec           (I i                                                        ) { return --i;                          }
template <class I> bool                    iequal         (I i, I j                                                   ) { return i == j;                       }
template <class I> bool                    inotEqual      (I i, I j                                                   ) { return i != j;                       }

template <class C>
string toTextCont(C* c, int64_t max = INT_MAX)
{
   string result("[");
   vector<string> elemStrs;
   typename C::iterator b = c->begin();
   typename C::iterator e = c->end();
   size_t len = 2;
   for (typename C::iterator i = b; i != e;)
   {
      string elemStr(toTextAux(*i, max));
      if (cycles.count(&*i))
         elemStr = string("#") + cycles[&*i] + "=" + elemStr;
      ++i;
      if (i != e) elemStr += ",";
      elemStrs.push_back(elemStr);
      len += elemStr.size();
   }
   size_t count = elemStrs.size();
   if (len > max)
   {
      for (size_t i = 0; i < count;)
      {
         result += indent(elemStrs[i], 1);
         ++i;
         if (i < count) result += "\n";
      }
   }
   else
   {
      for (size_t i = 0; i < count;)
      {
         result += elemStrs[i];
         ++i;
         if (i < count) result += " ";
      }
   }                    
   result += "]";
   return result;
}

template <class C>
string toTextCont1(C* c, int64_t max = INT_MAX)
{
   string result("[");
   vector<string> elemStrs;
   typename C::iterator b = c->begin();
   typename C::iterator e = c->end();
   size_t len = 2;
   for (typename C::iterator i = b; i != e;)
   {
      string elemStr(toTextAux(tderef1(i), max));
      ++i;
      if (i != e) elemStr += ",";
      elemStrs.push_back(elemStr);
      len += elemStr.size();
   }
   size_t count = elemStrs.size();
   if (len > max)
   {
      for (size_t i = 0; i < count;)
      {
         result += indent(elemStrs[i], 1);
         ++i;
         if (i < count) result += "\n";
      }
   }
   else
   {
      for (size_t i = 0; i < count;)
      {
         result += elemStrs[i];
         ++i;
         if (i < count) result += " ";
      }
   }                    
   result += "]";
   return result;
}

template <class C>
TypeInfo* addContainer()
{
   typedef typename C::iterator I;
   massign     .add(tassign        <C>);
   massign     .add(tassign        <I>);
   iterBegin   .add(titerBegin     <C>);
   iterEnd     .add(titerEnd       <C>);

   fold        .add(tfold          <C>);

   minc        .add(iinc     <I>);
   mdec        .add(idec     <I>);
   mequal      .add(iequal   <I>);
   mnotEqual   .add(inotEqual<I>);

   mtoVec      .add(ttoVec  <C>);

   return getTypeAdd<C>();

   vector<int64_t> x;
   vector<int64_t>::iterator i = x.begin();
   i.operator++();

}

template <class C>
TypeInfo* addContainerVD()
{
   typedef typename C::iterator I;
   addContainer<C>();
   getElem     .add(tgetElem       <C>);
   insertElemX .add(tinsertElemIX  <C>);
   insertElemX .add(tinsertElemIntX<C>);
   eraseElemX  .add(teraseElemIX   <C>);
   eraseElemX  .add(teraseElemIntX <C>);
   setElemX    .add(tsetElemX      <C>);
   mderef      .add(tderef         <I>);
   append      .add(tappend        <C>);
   mmap        .add(tmap           <C>);
   mfromVec    .add(tfromVec       <C>);
   findCycles  .add(findCyclesCont<C>);
   return getTypeAdd<C>();
}

template <class C>
TypeInfo* addContainerVD1()
{
   typedef typename C::iterator I;
   addContainerVD<C>();
   toTextAux   .add(toTextCont<C>);
   return getTypeAdd<C>();
}

template <class C>
TypeInfo* addContainerVD2()
{
   typedef typename C::iterator I;
   addContainerVD<C>();
   toTextAux   .add(toTextCont1<C>);
   return getTypeAdd<C>();
}

template <class C>
TypeInfo* addContainerNR()
{
   typedef typename C::iterator I;
   addContainer<C>();
   getElem     .add(tgetElem1      <C>);
   mderef      .add(tderef1        <I>);
   toTextAux   .add(toTextCont1<C>);
   return getTypeAdd<C>();
}

template <class C>
TypeInfo* addContainer2()
{
   typedef typename C::value_type     T;
   typedef typename C::iterator       I;
   typedef typename C::const_iterator CI;
   typedef typename C::size_type      S;

   typedef vector<int64_t>::iterator (vector<int64_t>::*InsertElem)(vector<int64_t>::const_iterator, const int64_t&);
   //InsertElem f = &vector<int64_t>::insert;
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

Vec makeVec(Any* args, int64_t nargs)
{
   return Vec(args, args + nargs);
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

void addList()
{
   iterBegin   .add(iterBeginCons);
   iterEnd     .add(iterEndCons);
   mderef      .add(derefConsIter);
   minc        .add(incConsIter);
   mequal      .add(equalConsIter);
   mnotEqual   .add(notEqualConsIter);
   mmap        .add(mapCons);
}

bool equalSymbol(Symbol* a, Symbol* b)
{
	return a == b;
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
Any addGlobal(string name, Any a)
{
   globalFrame->vars.push_back(Var(name, a));
   Member* member = new Member(getSymbol(name));
   member->one = true;
   member->value = a;
   Struct::globalFrame.add(member, a);
   return a;
}

TypeInfo* addGlobal(string name, TypeInfo* a)
{
   a->setName(name);
   addGlobal(name, Any(a));
   return a;
}

template <class R>
void addGlobal(string name, Multimethod<R>& m)
{
   m.name = name;
   addGlobal(name, Any(&m));
}

bool isNum(Any a)
{
   return a.typeInfo == tiInt || a.typeInfo == tiInt64 || a.typeInfo == tiFloat || a.typeInfo == tiDouble;
}

Any List::closureDeleg(Any* lambdaP, Any* params, int64_t np)
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
#ifdef _WIN32
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

NumberRange<int64_t>* range(int64_t a, int64_t b)
{
   return new NumberRange<int64_t>(a, b);
}

#if 1
Compose* compose(Any f, Any g)
{
   return new Compose(f, g);
}

Any delegCompose(Any* _compose, Any* args, int64_t n)
{
   Compose* compose = *_compose;
   Any gResult(compose->g.call(args, n));
   return compose->f.call(&gResult, 1);
}
#else
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
#endif
ostream& operator<<(ostream& Stream, Any any)
{
   operator<<(Stream, toText(any));
   return Stream;
}

/*
Array concatArrays(Any inp, int64_t dim)
{
}
*/
void setup()
{
   globalFrame = new Frame;
   globalFrame->context = nullptr;
   globalFrame->visible = false;

   Struct::globalFrame.typeInfo = newTypeInterp("globalFrameType");

   tiAny           = addGlobal("Any"          , getTypeAdd<Any             >());
   tiPH            = addGlobal("PH"           , getTypeAdd<PH              >());
   tiMissingArg    = addGlobal("MissingArg"   , getTypeAdd<MissingArg      >());
   tiParseEnd      = addGlobal("ParseEnd"     , getTypeAdd<ParseEnd        >());
   tiDouble        = addGlobal("double"       , getTypeAdd<double          >());
   tiFloat         = addGlobal("float"        , getTypeAdd<float           >());
   tiInt64         = addGlobal("int64"        , getTypeAdd<int64_t         >());
   tiInt32         = addGlobal("int32"        , getTypeAdd<int32_t         >());
   tiInt16         = addGlobal("int16"        , getTypeAdd<int16_t         >());
   tiInt8          = addGlobal("int8"         , getTypeAdd<int8_t          >());
   tiUInt64        = addGlobal("uint64"       , getTypeAdd<uint64_t        >());
   tiUInt32        = addGlobal("uint32"       , getTypeAdd<uint32_t        >());
   tiUInt16        = addGlobal("uint16"       , getTypeAdd<uint16_t        >());
   tiUInt8         = addGlobal("uint8"        , getTypeAdd<uint8_t         >());
   tiChar          = addGlobal("char"         , getTypeAdd<char            >());
   tiSymbol        = addGlobal("symbol"       , getTypeAdd<Symbol         *>());
   tiCons          = addGlobal("Cons"         , getTypeAdd<Cons           *>());
   tiListClosure   = addGlobal("ListClosure"  , getTypeAdd<List::Closure  *>());
   tiStructClosure = addGlobal("StructClosure", getTypeAdd<Struct::Closure*>());
   tiString        = addGlobal("string"       , getTypeAdd<string          >());
   tiVarRef        = addGlobal("VarRef"       , getTypeAdd<VarRef         *>());
   tiVarFunc       = addGlobal("VarFunc"      , getTypeAdd<VarFunc         >());
   tiPartApply     = addGlobal("PartApply"    , getTypeAdd<PartApply      *>());
   tiBind          = addGlobal("Bind"         , getTypeAdd<Bind           *>());
   tiTypeInfo      = addGlobal("TypeInfo"     , getTypeAdd<TypeInfo       *>());
   tiInt  = tiInt64;
   tiUInt = tiUInt64;

   addMember(&Cons::car, "car");
   addMember(&Cons::cdr, "cdr");

   addMember(&VarRef::name  , "name");
   addMember(&VarRef::frameC, "frameC");
   addMember(&VarRef::varC  , "varC");

   addMember(&Frame::vars   , "vars");
   addMember(&Frame::context, "context");

   addMember(&TypeInfo::members    , "args");
   addMember(&TypeInfo::copyCon    , "copyCon");
   addMember(&TypeInfo::count      , "count");
   addMember(&TypeInfo::delegPtr   , "delegPtr");
   addMember(&TypeInfo::fullName   , "fullName");
   addMember(&TypeInfo::kind       , "kind");
   addMember(&TypeInfo::conv       , "conv");
   addMember(&TypeInfo::main       , "main");
   addMember(&TypeInfo::member     , "member");
   addMember(&TypeInfo::multimethod, "multimethod");
   addMember(&TypeInfo::name       , "name");
   addMember(&TypeInfo::nKind      , "nKind");
   addMember(&TypeInfo::number     , "number");
   addMember(&TypeInfo::of         , "of");
   addMember(&TypeInfo::size       , "size");

   addMember(&PH      ::n          , "n");

   addMember(&VarFunc ::func       , "func");

   addMember(&PartApply::fixed     , "fixed");
   addMember(&PartApply::args      , "args");

   addMember(&Bind     ::params    , "params");

   addMember(&MMBase   ::name      , "name");

   addMember(&Struct::Stack::frame , "frame");
   addMember(&Struct::Stack::parent, "parent");

   addMember(&Identifier::member   , "member");
   addMember(&Identifier::frameN   , "frameN");

   addMember(&Member::symbol       , "symbol");

   addMember(&Symbol::name         , "name");

   addMember(&Compose::f           , "f");
   addMember(&Compose::g           , "g");

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
   addTypeLink(tiSigned  , tiInt32   );
   addTypeLink(tiSigned  , tiInt64   );
   addTypeLink(tiUnsigned, tiUInt8   );
   addTypeLink(tiUnsigned, tiUInt16  );
   addTypeLink(tiUnsigned, tiUInt32  );
   addTypeLink(tiUnsigned, tiUInt64  );
   addTypeLink(tiFloating, tiFloat   );
   addTypeLink(tiFloating, tiDouble  );

   addTypeLinkConv(tiDouble, tiFloat );
   addTypeLinkConv(tiDouble, tiInt64 );

   addTypeLinkConv(tiInt64 , tiInt32 );
   addTypeLinkConv(tiInt32 , tiInt16 );
   addTypeLinkConv(tiInt16 , tiInt8  );

   addTypeLinkConv(tiUInt64, tiUInt32);
   addTypeLinkConv(tiUInt32, tiUInt16);
   addTypeLinkConv(tiUInt16, tiUInt8 );

   addTypeLinkConv(tiInt64 , tiUInt64);
   addTypeLinkConv(tiInt32 , tiUInt32);
   addTypeLinkConv(tiInt16 , tiUInt16);
   addTypeLinkConv(tiInt8  , tiUInt8 );
   addTypeLinkConv(tiInt8  , tiChar  );

   tiUInt8->doC3Lin(&TypeInfo::conv);

   tiInt8  ->nKind = kSigned;
   tiInt16 ->nKind = kSigned;
   tiInt32 ->nKind = kSigned;
   tiInt64 ->nKind = kSigned;
   tiUInt8 ->nKind = kUnsigned;
   tiUInt16->nKind = kUnsigned;
   tiUInt32->nKind = kUnsigned;
   tiUInt64->nKind = kUnsigned;
   tiFloat ->nKind = kFloat;
   tiDouble->nKind = kFloat;

   convert.useReturnT = true;
   convert.useTable   = true;
   convert.add(tconvert< int8_t ,  int8_t >);
   convert.add(tconvert< int8_t ,  int16_t>);
   convert.add(tconvert< int8_t ,  int32_t>);
   convert.add(tconvert< int8_t ,  int64_t>);
   convert.add(tconvert< int8_t , uint8_t >);
   convert.add(tconvert< int8_t , uint16_t>);
   convert.add(tconvert< int8_t , uint32_t>);
   convert.add(tconvert< int8_t , uint64_t>);
   convert.add(tconvert< int8_t , float   >);
   convert.add(tconvert< int8_t , double  >);

   convert.add(tconvert< int16_t ,  int8_t >);
   convert.add(tconvert< int16_t ,  int16_t>);
   convert.add(tconvert< int16_t ,  int32_t>);
   convert.add(tconvert< int16_t ,  int64_t>);
   convert.add(tconvert< int16_t , uint8_t >);
   convert.add(tconvert< int16_t , uint16_t>);
   convert.add(tconvert< int16_t , uint32_t>);
   convert.add(tconvert< int16_t , uint64_t>);
   convert.add(tconvert< int16_t , float   >);
   convert.add(tconvert< int16_t , double  >);

   convert.add(tconvert< int32_t ,  int8_t >);
   convert.add(tconvert< int32_t ,  int16_t>);
   convert.add(tconvert< int32_t ,  int32_t>);
   convert.add(tconvert< int32_t ,  int64_t>);
   convert.add(tconvert< int32_t , uint8_t >);
   convert.add(tconvert< int32_t , uint16_t>);
   convert.add(tconvert< int32_t , uint32_t>);
   convert.add(tconvert< int32_t , uint64_t>);
   convert.add(tconvert< int32_t , float   >);
   convert.add(tconvert< int32_t , double  >);

   convert.add(tconvert< int64_t ,  int8_t >);
   convert.add(tconvert< int64_t ,  int16_t>);
   convert.add(tconvert< int64_t ,  int32_t>);
   convert.add(tconvert< int64_t ,  int64_t>);
   convert.add(tconvert< int64_t , uint8_t >);
   convert.add(tconvert< int64_t , uint16_t>);
   convert.add(tconvert< int64_t , uint32_t>);
   convert.add(tconvert< int64_t , uint64_t>);
   convert.add(tconvert< int64_t , float   >);
   convert.add(tconvert< int64_t , double  >);

   convert.add(tconvert<uint8_t ,  int8_t >);
   convert.add(tconvert<uint8_t ,  int16_t>);
   convert.add(tconvert<uint8_t ,  int32_t>);
   convert.add(tconvert<uint8_t ,  int64_t>);
   convert.add(tconvert<uint8_t , uint8_t >);
   convert.add(tconvert<uint8_t , uint16_t>);
   convert.add(tconvert<uint8_t , uint32_t>);
   convert.add(tconvert<uint8_t , uint64_t>);
   convert.add(tconvert<uint8_t , float   >);
   convert.add(tconvert<uint8_t , double  >);

   convert.add(tconvert<uint16_t ,  int8_t >);
   convert.add(tconvert<uint16_t ,  int16_t>);
   convert.add(tconvert<uint16_t ,  int32_t>);
   convert.add(tconvert<uint16_t ,  int64_t>);
   convert.add(tconvert<uint16_t , uint8_t >);
   convert.add(tconvert<uint16_t , uint16_t>);
   convert.add(tconvert<uint16_t , uint32_t>);
   convert.add(tconvert<uint16_t , uint64_t>);
   convert.add(tconvert<uint16_t , float   >);
   convert.add(tconvert<uint16_t , double  >);

   convert.add(tconvert<uint32_t ,  int8_t >);
   convert.add(tconvert<uint32_t ,  int16_t>);
   convert.add(tconvert<uint32_t ,  int32_t>);
   convert.add(tconvert<uint32_t ,  int64_t>);
   convert.add(tconvert<uint32_t , uint8_t >);
   convert.add(tconvert<uint32_t , uint16_t>);
   convert.add(tconvert<uint32_t , uint32_t>);
   convert.add(tconvert<uint32_t , uint64_t>);
   convert.add(tconvert<uint32_t , float   >);
   convert.add(tconvert<uint32_t , double  >);

   convert.add(tconvert<uint64_t ,  int8_t >);
   convert.add(tconvert<uint64_t ,  int16_t>);
   convert.add(tconvert<uint64_t ,  int32_t>);
   convert.add(tconvert<uint64_t ,  int64_t>);
   convert.add(tconvert<uint64_t , uint8_t >);
   convert.add(tconvert<uint64_t , uint16_t>);
   convert.add(tconvert<uint64_t , uint32_t>);
   convert.add(tconvert<uint64_t , uint64_t>);
   convert.add(tconvert<uint64_t , float   >);
   convert.add(tconvert<uint64_t , double  >);

   convert.add(tconvert<float    ,  int8_t >);
   convert.add(tconvert<float    ,  int16_t>);
   convert.add(tconvert<float    ,  int32_t>);
   convert.add(tconvert<float    ,  int64_t>);
   convert.add(tconvert<float    , uint8_t >);
   convert.add(tconvert<float    , uint16_t>);
   convert.add(tconvert<float    , uint32_t>);
   convert.add(tconvert<float    , uint64_t>);
   convert.add(tconvert<float    , float   >);
   convert.add(tconvert<float    , double  >);

   convert.add(tconvert<double   ,  int8_t >);
   convert.add(tconvert<double   ,  int16_t>);
   convert.add(tconvert<double   ,  int32_t>);
   convert.add(tconvert<double   ,  int64_t>);
   convert.add(tconvert<double   , uint8_t >);
   convert.add(tconvert<double   , uint16_t>);
   convert.add(tconvert<double   , uint32_t>);
   convert.add(tconvert<double   , uint64_t>);
   convert.add(tconvert<double   , float   >);
   convert.add(tconvert<double   , double  >);

   toTextAux.add(toTextCons);
   toTextAux.add(toTextBool);
   toTextAux.add(toTextSym);
   toTextAux.add(toTextOp);
   toTextAux.add(toTextParseEnd);
   toTextAux.add(toTextChar);
   toTextAux.add(toTextString);
   toTextAux.add(toTextLambda);
   //toTextAux.add(toTextTypeInfo);
   toTextAux.add(toTextFrame);
   toTextAux.add(toTextArray);
   //toTextAux.add(toTextMultimethod);
   toTextAux.add(toTextVar);

   pushBack .add(pushBackCons);
   mequal   .add(equalSymbol);
   mnotEqual.add(notEqualSymbol);

           addContainerVD1<vector     <int64_t>>()->setName("vector<int64_t>" );
   tiVec = addContainerVD1<vector     <Any    >>()->setName("vector<Any>"     );
           addContainerVD1<vector     <Expr*  >>()->setName("vector<Expr*>"   );
           addContainerVD1<deque      <int64_t>>()->setName("deque<int64_t>"  );
           addContainerVD1<deque      <Var    >>()->setName("deque<Var>"      );
           addContainerVD2<string              >()->setName("string"          );
           addContainerNR <vector     <bool   >>()->setName("vector<bool>"    );
           addContainerNR <NumberRange<int64_t>>()->setName("NumberRange<Int>");

   addList();
   madd.useConversions = true;
   madd.useTable = true;
   mmultiply.useConversions = true;
   mmultiply.useTable = true;
   addMathOps< double>();
   addMathOps<  float>();
   addMathOps<int8_t>();
   addMathOps<int16_t>();
   addMathOps<int32_t>();
   addMathOps<int64_t>();
   addMathOps<uint8_t>();
   addMathOps<uint16_t>();
   addMathOps<uint32_t>();
   addMathOps<uint64_t>();
   addMathOps<   char>(false);
   addGlobal("convert" , convert   );
   addGlobal("="       , massign   );
   addGlobal("range"   , range     );
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
   addGlobal("list"    , VarFunc   (mylistN, 0, LLONG_MAX));
   addGlobal("toText"  , toText    );

   addGlobal("insertElemX" , insertElemX );
   addGlobal("eraseElemX"  , eraseElemX  );
   addGlobal("getElem"     , getElem     );
   addGlobal("setElemX"    , setElemX    );
   addGlobal("iterBegin"   , iterBegin   );
   addGlobal("iterEnd"     , iterEnd     );

   addGlobal("++"          , append      );
   addGlobal("map"         , mmap        );
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
   findCycles  .add(findCyclesAny);
   setupParser2();
   toTextAux   .add(toTextAny);
#ifdef TYPESET
   for (auto p : typeSet) p->doC3Lin(&TypeInfo::main);
   for (auto p : typeSet) p->doC3Lin(&TypeInfo::conv);
   for (auto p : typeSet) p->allocate();
#else
   for (auto p : typeMap) p.second->doC3Lin(&TypeInfo::main);
   for (auto p : typeMap) p.second->doC3Lin(&TypeInfo::conv);
   for (auto p : typeMap) p.second->allocate();
#endif
   convert.buildTable();
   //for (auto p : MMBase::mms)
   //   p->buildTable();
}

/*
int64_t func(int64_t a, int64_t b)
{
   return a + b;
}

print func(2, 3);

struct Func
{
   int64_t a;
   int64_t b;
   int64_t operator()()
   {
      return a + b;
   }
};

print Func(2, 3)();
*/

