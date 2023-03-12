// Copyright (c) 2023 Brett Curtis
#pragma once
#include <vector>
#include <set>
#include <deque>

#include "any.h"
#include <algorithm>

struct Cons
{
   Any car;
   Any cdr;
   Cons() {}
   Cons(Any car, Any cdr) : car(car), cdr(cdr) {}
};

struct Gen
{
};

struct Type;

typedef double Num;
typedef Vec::iterator           VecI;
typedef Vec::const_iterator     VecCI;
Vec makeVec(Any* args, int64_t nargs);

typedef std::deque<Var>         DequeVar;
typedef std::deque<Var*>        DequePVar;

struct LessVarName
{
   bool operator()(const Var* lhs, const Var* rhs) const
   {
      return lhs->name < rhs->name;
   }
};

struct VarList
{
   std::set  <Var*, LessVarName> forwardTo;
   std::deque<Var*> byOffset;

   Var* add(Var& v)
   {
      auto i = forwardTo.find(&v);
      if (i == forwardTo.end())
      {
         Var* vp = new Var(v);
         forwardTo.insert(vp);
         byOffset.push_back(vp);
         return vp;
      }
      throw "var already exists!";
      return nullptr;
   }
};

struct Frame
{
   Any         returnProg;
   Frame*      returnFrame;
   Frame*      context;
   Any         prog;
   std::deque<Var>  vars;
   bool        visible;
   Frame() : visible(true) {}
   Frame(Any prog, Frame* context) : prog(prog), context(context), visible(true) {}
};

struct SymbolData
{
   std::string  name;
   int64_t      id;
            SymbolData(std::string name, int64_t id = -1) : name(name), id(id) {}
};

//MEANT TO BE A FUCKING WARNING NOT AN ERROR
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

template <class T>
struct NumberRange
{
   typedef T value_type;
   struct const_iterator
   {
      typedef std::random_access_iterator_tag iterator_category;
      typedef T   value_type;
      typedef int difference_type;
      typedef T*  pointer;
      typedef T&  reference;
      T i;
      T step;

      const_iterator(T i, T step = 1) : i(i), step(step) { }
      value_type&       operator* (     ) { return i; }
      value_type        operator[](int64_t n) { return i + n * step; }
      const_iterator&   operator++(     ) { i += step; return *this; }
      const_iterator&   operator--(     ) { i -= step; return *this; }
      const_iterator    operator++(int  ) { const_iterator n(*this); i += step; return n; }
      const_iterator    operator--(int  ) { const_iterator n(*this); i -= step; return n; }
      const_iterator    operator+ (int64_t n) { return const_iterator(i + step * n, step); }
      const_iterator    operator- (int64_t n) { return const_iterator(i - step * n, step); }
      const_iterator&   operator+=(int64_t n) { i += step * n; return *this; }
      const_iterator&   operator-=(int64_t n) { i -= step * n; return *this; }
      int64_t               operator- (const_iterator rhs) { return (i - rhs.i) / step; }
      bool              operator==(const_iterator rhs) { return i == rhs.i; }
      bool              operator!=(const_iterator rhs) { return i != rhs.i; }
      bool              operator<=(const_iterator rhs) { return i <= rhs.i; }
      bool              operator>=(const_iterator rhs) { return i >= rhs.i; }
      bool              operator< (const_iterator rhs) { return i <  rhs.i; }
      bool              operator> (const_iterator rhs) { return i >  rhs.i; }
   };
   typedef const_iterator iterator;
   T b;
   T e;
   T step;

   NumberRange(T _b, T _e, T _step = 1) : b(_b), e(_e), step(_step) { }
   const_iterator begin () { return const_iterator(b, step); }
   const_iterator end   () { return const_iterator(e, step); }
   const_iterator cbegin() { return const_iterator(b, step); }
   const_iterator cend  () { return const_iterator(e, step); }
   T operator[](T i) { return b + i * step; }
};

struct Compose
{
   Any f;
   Any g;

   Compose(Any f, Any g) : f(f), g(g) {}
};

Any delegCompose(Any* _compose, Any* args, int64_t n);

template <>
struct getType<Compose*>
{
   typedef Compose* Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct  = Destruct<Type>::go;
      typeInfo->of       = getTypeAdd<Compose>();
      typeInfo->delegPtr = delegCompose;
      typeInfo->kind     = kPointer;
      return typeInfo;
   }
};

Compose* compose(Any f, Any g);
/*
   Symbol* temp = getSymbol(name);
   auto res = symbols.insert(temp);

   if (res.second)
   {
      (**res.first).group = group;
      (**res.first).id    = id   ;
   }
   return *res.first;
*/

struct VarRef
{
   std::string  name;
   int64_t      frameC;
   int64_t      varC;
            VarRef(std::string name, int64_t frameC, int64_t varC) : name(name), frameC(frameC), varC(varC) {}
};

extern Symbol *symBlankArg;
extern Cons   *nil;

extern Multimethod<Any>    convert    ;
extern Multimethod<Any>    pushBack   ;
extern Multimethod<void>   insertElemX;
extern Multimethod<std::string> toTextAux  ;

extern TypeInfo* tiAny     ;
extern TypeInfo* tiSymbol  ;
extern TypeInfo* tiCons    ;
extern TypeInfo* tiInt     ;
extern TypeInfo* tiInt64   ;
extern TypeInfo* tiFloat   ;
extern TypeInfo* tiDouble  ;
extern TypeInfo* tiListClosure  ;
extern TypeInfo* tiStructClosure;
extern TypeInfo* tiParseEnd;
extern TypeInfo* tiString  ;
extern TypeInfo* tiVarRef  ;
extern TypeInfo* tiVarFunc ;
extern TypeInfo* tiVec     ;

extern Frame*    globalFrame;

namespace Struct
{
   extern Any globalFrame;
}


std::string toText        (Any a       , int64_t max = LLONG_MAX);
std::string toTextInt     (int64_t n   , int64_t max = LLONG_MAX);
std::string toTextIntR    (int64_t&    , int64_t max = LLONG_MAX);
std::string toTextTypeInfo(TypeInfo* ti, int64_t max = LLONG_MAX);
std::string toTextAny     (Any a       , int64_t max = LLONG_MAX);

void        setup       ();
Cons*       mylist      ();
Cons*       mylist      (Any i0);
Cons*       mylist      (Any i0, Any i1);
Cons*       mylist      (Any i0, Any i1, Any i2);
Cons*       mylist      (Any i0, Any i1, Any i2, Any i3);
int64_t     lengthCons  (Cons* c);
Any&        car         (Cons* c);
Any&        cdr         (Cons* c);
Any&        first       (Cons* c);
Any&        second      (Cons* c);
Any&        third       (Cons* c);
Any&        fourth      (Cons* c);
Any&        drop1       (Cons* c);
Any&        drop2       (Cons* c);
Any&        drop3       (Cons* c);
Any&        drop4       (Cons* c);
std::string symbolText  (Symbol* symbol);
Cons*       cons        (Any a, Cons* b);
bool        isNil       (Any a);
bool        isBlank     (Any a);
Cons*       reverse     (Cons* c);
bool        isNum       (Any a);
Cons*       mapCons     (Any f, Cons* c);
bool        operator==  (Any a, Any b);
bool        operator!=  (Any a, Any b);
Any         operator++  (Any a);
Any         operator*   (Any a);

std::ostream& operator<<(std::ostream& Stream, Any any);
