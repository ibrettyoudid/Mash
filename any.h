// Copyright (c) 2023 Brett Curtis
#pragma once
#include <map>
#include <vector>
#include <deque>
#include <set>
#include <iostream>
#include <typeinfo>

struct Any;
struct TypeInfo;

enum LinkKind
{
   lSub = 0,
   lVirtual,
   lVirtualSub, //virtual inheritance but attempt to keep it in same place
   lConv,
   lDelegate //isn't this the same as conversion? yes i think it is
};

enum Kind
{
   kNormal = 0,
   kFunction,
   kPointer,
   kReference,
   kPtrMem,
   kRefMem,
   kArray
};

enum NKind
{
   kSigned,
   kUnsigned,
   kFloat
};

struct Member;

struct Symbol;

struct MMBase;

struct LessMemberSymbol
{
   typedef void is_transparent;
   bool operator()(const Member* lhs, const Member* rhs) const;
   bool operator()(const Member* lhs, Symbol* rhs) const;
   bool operator()(Symbol* lhs, const Member* rhs) const;
};

struct LessMemberName
{
   typedef void is_transparent;
   bool operator()(const Member* lhs, const Member* rhs) const;
   bool operator()(const Member* lhs, const std::string& rhs) const;
   bool operator()(const std::string& lhs, const Member* rhs) const;
};

struct MemberList
{
   std::set<Member*> byPtr;
   std::multiset<Member*, LessMemberSymbol> bySymbol;
   std::multiset<Member*, LessMemberName  > byName;
   std::deque<Member*> byOffset;

   Member* add(Member* v);
   Member* operator[](Member* v);
   Member* operator[](Symbol* symbol);
   Member* operator[](std::string name);
   Member* operator[](int64_t n);
   int64_t     size();
   void    clear();
};

struct TypeLink;

extern TypeInfo* tiChar;

struct Dynamic
{
};

struct Ref
{
};

struct Any
{
   TypeInfo* typeInfo;
   void* ptr;

   template <class Type> void construct(Type value);
   Any();
   template <class Type> Any(Type value);
   template <class Type> Any(Type& value, Dynamic);//detect type dynamically using C++ RTTI - takes a reference but creates a plain value
   template <class Type> Any(Type& value, Ref);    //creates a reference
   Any(const Any& rhs);
   Any(Any &&rhs);
   Any& operator= (const Any& rhs);
   Any& operator= (Any&& rhs);
   Any(TypeInfo* typeInfo, void* rhs);
   ~Any();
   Any call(Any* args, int64_t n);
   Any operator()();
   Any operator()(Any arg0);
   Any operator()(Any arg0, Any arg1);
   Any operator()(Any arg0, Any arg1, Any arg2);
   Any operator()(Any arg0, Any arg1, Any arg2, Any arg3);
   Any operator()(Any arg0, Any arg1, Any arg2, Any arg3, Any arg4);
   Any derp();
   //--------------------------------------------------------------------------- CONVERSION
   template <class To>   operator To&();
   template <class To>   To& as();
   template <class To>   To* any_cast();
   operator MMBase& ();
   //---------------------------------------------------------------------------------
   std::string typeName  ();
   TypeInfo*   paramType (int64_t n, bool useReturnT = false);
   bool        isPtrTo   (TypeInfo* other);
   bool        isRefTo   (TypeInfo* other);
   bool        isPRTF    (TypeInfo* other);
   int64_t         maxArgs   ();
   int64_t         minArgs   ();
   bool        callable  ();
   void        showhex   ();
   std::string hex       ();
   int64_t         rDepth    ();
   int64_t         pDepth    ();
   int64_t         prDepth   ();
   Any         add       (Member* m, Any value);
   Any         member    (Member* m);
   Any         member    (Symbol* symbol);
   Any         member    (std::string name);
   Any         member    (int64_t n);
};

struct TypeLink
{
   TypeInfo* base;
   TypeInfo* derived;
   LinkKind  kind;
   int64_t       offset;
   void*     (*staticUp   )(void*);
   void*     (*staticDown )(void*);
   void*     (*dynamicUp  )(void*);
   void*     (*dynamicDown)(void*);

   TypeLink(TypeInfo* base, TypeInfo* derived, LinkKind kind, int64_t offset)
      : base(base), derived(derived), kind(kind), offset(offset) {}
   /* 
   for normal inheritance, offset is the position in the derived object of the base object, 
   under single inheritance it's always 0

   for virtual inheritance,
   need a link in the derived type to the base
   need a link in the base to the derived type
   */
};

typedef std::vector<TypeInfo*> TypeList;
typedef std::vector<TypeLink*> TypeLinkList;

struct TypePath
{
   TypeInfo* base;
   TypeInfo* derived;
   LinkKind  kind;
   int64_t   offsetAll;
   int64_t   offsetInterp;
   TypeLinkList  links;
   TypePath(TypeInfo* base, TypeInfo* derived, LinkKind kind, int64_t offsetAll, int64_t offsetInterp, TypeLinkList links) : base(base), derived(derived), kind(kind), offsetAll(offsetAll), offsetInterp(offsetInterp), links(links) {}
};

typedef std::multiset<TypePath*> TypePathList;

std::string hex(unsigned char* b, unsigned char* e);
void showhex(unsigned char* b, unsigned char* e);

struct TypeInfo
{
   std::string      name;
   int64_t          size;
   Kind             kind;
   TypeInfo*        of;

   MemberList       members;
   MemberList       allMembers;

   TypeLinkList     bases;
   TypeLinkList     derived;
   TypeList         linear;
   TypePathList     layout;

   void     (*copyCon )(void* res, void* in, TypeInfo* ti);//set in getTypeBase
   void     (*destroy )(void* target);
   Any      (*delegPtr)(Any* fp , Any* params, int64_t np);

   NKind    nKind;//Signed/unsigned/floating

   bool             cpp;
   const type_info* cType;

   bool     number;
   bool     member;//member function or member pointer (memptrs are callable)
   bool     multimethod;

   int64_t      count;
   std::string      fullName;

   std::vector<std::vector<int64_t>> mmpindices;

   TypeInfo(std::string name = "") : name(name), member(false), multimethod(false), of(nullptr), copyCon(nullptr), kind(kNormal)
   {
   }


   void        doC3Lin   ();
   void        allocate  ();
   void        allocate2 (TypeInfo* derived, int64_t baseOffset, TypeLinkList& links);
   TypeInfo*   argType   (int64_t n, bool useReturnT);
   TypeInfo*   unref     ();
   TypeInfo*   unptr     (int64_t n);
   TypeInfo*   refTo     ();
   bool        isPtrTo   (TypeInfo* other);
   bool        isRefTo   (TypeInfo* other);
   bool        isPRTF    (TypeInfo* other);
   bool        isRTF     (TypeInfo* other);
   std::string getCName  (std::string n = "");
   std::string getName   ();
   TypeInfo*   setName   (std::string s);
   Member*     add       (Member* m);
   Member*     operator[](Symbol* s);
   Member*     operator[](std::string n);
   Member*     operator[](int64_t n);
};

bool hasLink(TypeInfo* base, TypeInfo* derived);
//void addMember(Any member, std::string name);

extern TypeInfo* tiAny;
/*
struct ArgInfo
{
   TypeInfo* typeInfo;
   std::string   name;
   int64_t       offset;
   Any       defaultVal;
};
*/
template <class T>
T* toObject(T value)
{
   return new T(value);
}

struct TILess
{
   bool operator()(const type_info* lhs, const type_info* rhs) const
   {
      return lhs->before(*rhs) != 0;
   }
};



typedef std::map<const type_info*, TypeInfo*, TILess> TypeMap;
extern TypeMap typeMap;
extern TypeMap typeMapR;

struct TILess2
{
   typedef void is_transparent;
   bool operator()(const TypeInfo* lhs, const TypeInfo* rhs) const
   {
      return lhs->cType < rhs->cType;
   }
   bool operator()(const type_info* lhs, const TypeInfo* rhs) const
   {
      return lhs < rhs->cType;
   }
   bool operator()(const TypeInfo* lhs, const type_info* rhs) const
   {
      return lhs->cType < rhs;
   }
};

typedef std::set<TypeInfo*, TILess2> TypeSet;
extern TypeSet typeSet;
extern TypeSet typeSetR;


template <class T>
struct CopyCon
{
   static void go(void* res, void* in, TypeInfo*)//doesn't work on references
   {
      new((T*)res) T(*(T*)in);
   }
};

template <class T>
struct CopyCon<T&>
{
   static void go(void* res, void* in, TypeInfo* ti)
   {
      CopyCon<T*>::go(res, in, ti);
   }
};

template <class T>
struct CopyCon<const T&>
{
   static void go(void* res, void* in, TypeInfo* ti)
   {
      CopyCon<T*>::go(res, in, ti);
   }
};

template <class T>
struct CopyCon<const T>
{
   static void go(void* res, void* in, TypeInfo* ti)
   {
      CopyCon<T*>::go(res, in, ti);
   }
};

template <class T, int64_t n>
struct CopyCon<T[n]>
{
   static void go(void* res, void* in, TypeInfo* ti)
   {
      T* resT = static_cast<T*>(res);
      T* inT  = static_cast<T*>(in );
      for (size_t i = 0; i < n; ++i)
         CopyCon<T>::go(resT + i, inT + i, ti);
   }
};

template <class T>
struct Destroy
{
   static void go(void* target)
   {
      T* t = static_cast<T*>(target);
      t->~T();
   }
};

template <class T>
struct Destroy<T&>
{
   static void go(void* target)
   {
      T** t = static_cast<T**>(target);
      (*t)->~T();
   }
};


/*
getTypeAdd -> getTypeAddStr -> getType -> getTypeBase

getTypeAddStr is specialised for
   references since typeid gives the same answer for refs as non refs

getType is specialised for 
   references     { kind = kReference; of; add & to name; }
   pointers       { kind = kPointer; of }
   pointers to member
   function types { of; call; kind = kFunction; members; }
   Multimethod    { of; call; }
   Multimethod*
   PartApply*
   VarFunc
   Closure*


*/

template <class Type>
TypeInfo getTypeBase()
{
   TypeInfo typeInfo;
   typeInfo.cpp      = true;
   typeInfo.cType    = &typeid(Type);
   typeInfo.name     = typeid(Type).name();
   typeInfo.fullName = typeid(Type).name();
   typeInfo.size     = sizeof(Type);
   typeInfo.delegPtr = nullptr;
   typeInfo.copyCon  = CopyCon<Type>::go;
   typeInfo.destroy  = Destroy<Type>::go;
   return typeInfo;
}

static void copynothing(void* res, void* in, TypeInfo* ti)
{
}

static void destroynothing(void*)
{
}

template <class Type>
struct getType
{
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<Type>());
      return fti;
   }
};

template < >
struct getType<void>
{
   static TypeInfo info()
   {
      TypeInfo typeInfo;
      typeInfo.cpp = true;
      typeInfo.name = "void";
      typeInfo.fullName = "void";
      typeInfo.size = 0;
      typeInfo.delegPtr = nullptr;
      typeInfo.copyCon = copynothing;
      typeInfo.destroy = destroynothing;
      return typeInfo;
   }
};

template <class Type>
struct getTypeAddStr
{
   static TypeInfo* go()
   {
      TypeInfo*&          ti = typeMap[&typeid(Type)];
      if (ti == nullptr)  ti = toObject(getType<Type>::info());
      return ti;
      //return new TypeInfo(getType<X>::info());
   }
};

template <class Type>
struct getTypeAddStr<Type&>
{
   static TypeInfo* go()
   {
      /*
      auto i = typeSetR.find(&typeid(Type));
      if (i == typeSetR.end())
      {  
         TypeInfo* ti = toObject(getType<Type>::info());
         typeSetR.insert(ti);
         return ti;
      }
      return *i;
      */
      TypeInfo*&          ti = typeMapR[&typeid(Type)];
      if (ti == nullptr)  ti = toObject(getType<Type&>::info());
      return ti;
      //return new TypeInfo(getType<X>::info());
   }
};

template <class Type>
TypeInfo* getTypeAdd()
{
   return getTypeAddStr<Type>::go();
}

template <class To>
struct getType<To&>
{
   typedef To& T;
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.size      = sizeof(To*);
      fti.name     += "&";
      fti.fullName += "&";
      fti.kind  = kReference;
      fti.of = getTypeAdd<To>();
      return fti;
   }
};

template <class R>
struct getType<R*>//Closure* and Multimethod* specialise this further
{
   typedef R* T;
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind = kPointer;
      fti.of = getTypeAdd<R>();
      return fti;
   }
};
/*
template <class R>
struct getType<R[]>
{
   typedef R T[];
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());//illegal sizeof operand, makes a kind of sense i suppose being 0 bytes
      fti.kind = kArray;
      fti.of = getTypeAdd<R>();
      return fti;
   }
};
*/
template <class R, int64_t n>
struct getType<R[n]>
{
   typedef R T[n];
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind  = kArray;
      fti.count = n;
      fti.of = getTypeAdd<R>();
      return fti;
   }
};

template <class To, class From>
To double_cast(From F)
{
   return *(To*)(void*)&F;
}

template <class To, class From>
To union_cast(From f)//no good if one has a copy constructor
{
   union
   {
      From f;
      To   t;
   } u;

   u.f = f;
   return u.t;
}

template <class From, class To>
struct StaticCast
{
   static void* go(void* from)
   {
      return static_cast<To*>(static_cast<From*>(from));
   }
};

template <class From, class To>
struct DynamicCast
{
   static void* go(void* from)
   {
      return dynamic_cast<To*>(static_cast<From*>(from));
   }
};

void addTypeLink(TypeInfo* base, TypeInfo* derived, LinkKind kind = lSub, int64_t offset = 0);

template <class Base, class Derived>
void addTypeLinkC(LinkKind kind = lSub, int64_t offset = 0)
{
   TypeInfo* base    = getTypeAdd<Base   >();
   TypeInfo* derived = getTypeAdd<Derived>();
   TypeLink* tl = new TypeLink(base, derived, kind, offset);
   tl.staticUp    = StaticCast <Derived, Base>::go;
   tl.staticDown  = StaticCast <Base, Derived>::go;
   tl.dynamicUp   = DynamicCast<Derived, Base>::go;
   tl.dynamicDown = DynamicCast<Base, Derived>::go;
   base->derived.push_back(tl);
   derived->bases.push_back(tl);
}

struct Symbol
{
   std::string    name;
   int64_t       group;
   int64_t       id;

   Symbol(std::string name) : name(name) {}
   Symbol(std::string name, int64_t group, int64_t id) : name(name), group(group), id(id) {}
};

struct SymbolCompare
{
   bool operator()(const Symbol* LHS, const Symbol* RHS) const
   {
      return LHS->name < RHS->name;
   }
};

typedef std::set<Symbol*, SymbolCompare> Symbols;

extern Symbols symbols;

Symbol* getSymbol(std::string name, int64_t group = 0, int64_t id = 0);

struct Member
{
   bool      one;
   bool      cpp;
   TypeInfo* typeInfo;
   Symbol*   symbol;
   Any       value;
   int64_t       offset;
   Any           ptrToMember;
   TypeLinkList  links;
#if MEMBERFUNCTIONSEXACT
   Any       getMemberRef;
   Any       getMember;
   Any       setMember;
#else
   Any       (*getMember   )(Member* mem, Any& ca);
   Any       (*getMemberRef)(Member* mem, Any& ca);
   void      (*setMember   )(Member* mem, Any& ca, Any& ra);
#endif
   Member(Symbol* symbol, TypeInfo * typeInfo = nullptr, int64_t offset = 0, Any ptrToMember = 0) : symbol(symbol), typeInfo(typeInfo), offset(offset), ptrToMember(ptrToMember), one(false) {}
   Member(TypeInfo* typeInfo) : symbol(nullptr), typeInfo(typeInfo), offset(0), ptrToMember(0), one(false) {}
};

//required by microsoft vector
#ifdef _MSC_VER
template<> inline
   Any* std::addressof(Any& _Val)
   {	// return address of _Val
   return &_Val;
   }
#endif

template <class Type>
void Any::construct(Type value)
{
   typeInfo = getTypeAdd<Type>();
   ptr = new char[typeInfo->size];
   typeInfo->copyCon(ptr, &value, typeInfo);
}
template <class Type> Any::Any(Type value)
{
   typeInfo = getTypeAdd<Type>();
   ptr = new char[typeInfo->size];
   typeInfo->copyCon(ptr, &value, typeInfo);
}
/*
template <class Type> Any(Type* value)
{
   typeInfo = getTypeAdd(*value);
   ptr      = value;
}
*/
template <class Type> Any::Any(Type& value, Dynamic)//detect type dynamically using C++ RTTI - takes a reference but creates a plain value
{
   //typeInfo = *typeSet.find(&typeid(value));
   typeInfo = typeMap[&typeid(value)];
   ptr = new char[typeInfo->size];
   typeInfo->copyCon(ptr, &value, typeInfo);
}
template <class Type> Any::Any(Type& value, Ref)//creates a reference
{
   typeInfo = getTypeAdd<Type&>();
   ptr = new char[typeInfo->size];
   Type* temp = &value;
   typeInfo->copyCon(ptr, &temp, typeInfo);
}
#if 1
template <class To>
Any::operator To&()
{
   TypeInfo* toType = getTypeAdd<To>();
   TypeInfo* toType1 = toType;
   int64_t toRL = 1;//toRL can only be 0 or 1 and it makes no difference to the code required, except that adding more than one level of indirection will be bad
   int64_t toPL = 0;
   while (true)
   {
      if (toType->kind == kReference)
      {
         toType = toType->of;
         ++toRL;
      }
      else if (toType->kind == kPointer)
      {
         toType = toType->of;
         ++toPL;
      }
      else
         break;
   }
   TypeInfo* fromType = typeInfo;
   int64_t fromRL = 0;
   int64_t fromPL = 0;
   while (true)
   {
      if (fromType->kind == kReference)
      {
         fromType = fromType->of;
         ++fromRL;
      }
      else if (fromType->kind == kPointer)
      {
         fromType = fromType->of;
         ++fromPL;
      }
      else
         break;
   }
   //maybe put something in to disallow slicing
   if (!hasLink(toType, fromType))//only allow conversions from derived to base
   {
      std::cout << "types unrelated: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName() << std::endl;
      std::ostringstream o;
      o << "types unrelated: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
      throw std::runtime_error(o.str());
   }
   int64_t indirs = fromRL + fromPL - toPL;
   if (indirs < -1)
   {
      std::cout << "cannot add more than 1 indirection: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName() << std::endl;
      std::ostringstream o;
      o << "cannot add more than 1 indirection: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
      throw std::runtime_error(o.str());
   }
   if (indirs > 3)
   {
      std::cout << "cannot remove more than 3 indirections: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName() << std::endl;
      std::ostringstream o;
      o << "cannot remove more than 3 indirections: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
      throw std::runtime_error(o.str());
   }
   //below, the number of asterisks in the < > must equal the number of dereferences between return and static_cast (in order for the types to be correct)
   switch (indirs)
   {
      //case -2:
      //   return double_cast<To>(&ptr);
      case -1:
         return *double_cast<To*>(&ptr);

       //in cases below, type in < > is the from type, both To and From may be pointer/pointer-to-pointer/etc. types
      case 0:
         return *static_cast<To*>(ptr);
      case 1:
         return **static_cast<To**>(ptr);
      case 2:
         return ***static_cast<To***>(ptr);
      case 3:
         return ****static_cast<To****>(ptr);
   }
   throw "HUH?";
}
#else
template <class To>
Any::operator To ()
{
   TypeInfo* toType = getTypeAdd<To>();
   TypeInfo* toType1 = toType;
   int64_t toRL = 1;//toRL can only be 0 or 1 and it makes no difference to the code required, except that adding more than one level of indirection will be bad
   int64_t toPL = 0;
   while (true)
   {
      if (toType->kind == kReference)
      {
         toType = toType->of;
         ++toRL;
      }
      else if (toType->kind == kPointer)
      {
         toType = toType->of;
         ++toPL;
      }
      else
         break;
   }
   TypeInfo* fromType = typeInfo;
   int64_t fromRL = 0;
   int64_t fromPL = 0;
   while (true)
   {
      if (fromType->kind == kReference)
      {
         fromType = fromType->of;
         ++fromRL;
      }
      else if (fromType->kind == kPointer)
      {
         fromType = fromType->of;
         ++fromPL;
      }
      else
         break;
   }
   //maybe put something in to disallow slicing
   if (!hasLink(toType, fromType))//only allow conversions from derived to base
   {
      std::cout << "types unrelated: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName() << std::endl;
      std::ostringstream o;
      o << "types unrelated: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
      throw std::runtime_error(o.str());
   }
   int64_t indirs = fromRL + fromPL - toPL;
   if (indirs < -1)
   {
      std::cout << "cannot add more than 1 indirection: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName() << std::endl;
      std::ostringstream o;
      o << "cannot add more than 1 indirection: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
      throw std::runtime_error(o.str());
   }
   //below, the number of asterisks in the < > must equal the number of dereferences between return and static_cast (in order for the types to be correct)
   switch (indirs)
   {
      //case -2:
      //   return double_cast<To>(&ptr);
      //in cases below, type in < > is the from type, both To and From may be pointer/pointer-to-pointer/etc. types
   case -1:
      return *double_cast<To*>(&ptr);
   case 0:
      return *static_cast<To*>(ptr);
   case 1:
      return **static_cast<To**>(ptr);
   case 2:
      return ***static_cast<To***>(ptr);
   case 3:
      return ****static_cast<To****>(ptr);
   }
   std::cout << "cannot remove more than 3 indirections: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName() << std::endl;
   std::ostringstream o;
   o << "cannot remove more than 3 indirections: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
   throw std::runtime_error(o.str());
}
#endif
/*
template <class To1>
operator To1*()
{
   typedef To1* To;
   TypeInfo* typeInfoTo = getTypeAdd<To>();
   if (typeInfo == typeInfoTo) return *static_cast<To*>(ptr);
   //if (typeInfo->kind == kPointer && typeInfo->of == typeInfoTo) return **static_cast<To**>(ptr);
   if (typeInfoTo->kind == kPointer && typeInfoTo->of == typeInfo) return static_cast<To>(ptr);
   throw "type mismatch";
}
*/
template <class To>
To& Any::as()
{
   TypeInfo* typeInfoTo = getTypeAdd<To>();
   if (typeInfoTo == tiAny) return *reinterpret_cast<To*>(this);
   if (typeInfo->isRTF(typeInfoTo)) return *static_cast<To*>(ptr);
   if (typeInfo->isPtrTo(typeInfoTo)) return **static_cast<To**>(ptr);
   if (typeInfoTo->isPtrTo(typeInfo)) return *reinterpret_cast<To*>(&ptr);
   //if (typeInfoTo->kind == kPointer && typeInfoTo->of == typeInfo) return double_cast<To>(ptr);
   std::cout << TS + "type mismatch: casting " + typeInfo->name + " to " + typeInfoTo->name << std::endl;
   throw "type mismatch";
}
template <class To>
To* Any::any_cast()
{
   TypeInfo* typeInfoTo = getTypeAdd<To>();
   //if (typeInfoTo == tiAny) return reinterpret_cast<To*>(this);
   if (typeInfo->isRTF(typeInfoTo)) return static_cast<To*>(ptr);
   //if (typeInfo->isPtrTo(typeInfoTo)) return *static_cast<To**>(ptr);
   //if (typeInfoTo->isPtrTo(typeInfo)) return reinterpret_cast<To*>(&ptr);
   return nullptr;
}
/*
template <class To>
To any_cast()
{
   TypeInfo* typeInfoTo = getTypeAdd<To>();
   if (typeInfoTo->isPtrTo(typeInfo)) return reinterpret_cast<To>(&ptr);
   return nullptr;
}
*/
#if MEMBERFUNCTIONSEXACT
template <class C, class R>
R getMember(Member* mem, C* c)
{
   R C::*ptr = mem->ptrToMember.as<R C::*>();
   return c->*ptr;
}

template <class C, class R>
R& getMemberRef(Member* mem, C* c)
{
   R C::* ptr = mem->ptrToMember.as<R C::*>();
   return c->*ptr;
}

template <class C, class R>
void setMember(Member* mem, C* c, R& r)
{
   R C::* ptr = mem->ptrToMember.as<R C::*>();
   c->*ptr = r;
}
#else
template <class C, class R>
Any getMember(Member* mem, Any &ca)
{
   R C::* ptr = mem->ptrToMember.as<R C::*>();
   C*     c   = ca.as<C*>();
   return c->*ptr;
}

template <class C, class R>
Any getMemberRef(Member* mem, Any &ca)
{
   R C::* ptr = mem->ptrToMember.as<R C::*>();
   C*     c   = ca.as<C*>();
   return Any(c->*ptr, Ref());
}

template <class C, class R>
void setMember(Member* mem, Any& ca, Any& ra)
{
   R C::* ptr = mem->ptrToMember.as<R C::*>();
   C* c = ca.as<C*>();
   R& r = ra.as<R>();
   c->*ptr = r;
}
#endif

template <class C, class R>
void addMember(R C::*ptr, std::string name)
{
   TypeInfo* cti = getTypeAdd<C>();
   TypeInfo* mti = getTypeAdd<R>();
   C* c = nullptr;
   int64_t offset = reinterpret_cast<int64_t>(&(c->*ptr));
   Member* pm = new Member(getSymbol(name), mti, offset, ptr);
   cti->members.add(pm);
   pm->getMember    = getMember<C, R>;
   pm->getMemberRef = getMemberRef<C, R>;
   pm->setMember    = setMember<C, R>;
}

template <class R, class C>
Any delegPtrMem(Any* fp1, Any* params, int64_t n)
{
   R C::*pm(*fp1);
   std::cout << typeid(pm).name() << std::endl;
   C* c(params[0]);
   std::cout << "c type=" << typeid(c).name() << " value=" << c << std::endl;
   R& r(c->*pm);
   std::cout << "r type=" << typeid(r).name() << " address=" << &r << " value=" << r << std::endl;

   Any a(r, Ref());//return a REFERENCE to the member
   a.showhex();
   return a;
}

template <class R, class C>
struct getType<R C::*>
{
   typedef R* T;
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kPtrMem;
      fti.member = true;
      fti.of  = getTypeAdd<R>();
      fti.members.add(new Member(getSymbol("class"), getTypeAdd<C>()));
      fti.delegPtr = delegPtrMem<R, C>;
      return fti;
   }
};

void addMemberInterp(TypeInfo* _class, std::string name, TypeInfo* type);
//--------------------------------------------------------------------------- void func()
Any delegVoid0(Any* fp1, Any* params, int64_t n);

//begin AnySpecs
///////////////////////////////////////////////////////////////////////////////// FUNCTIONS RETURNING VOID
template <>
struct getType<void (*)()>
{
   typedef void (*T)();
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind = kFunction;
      fti.delegPtr = delegVoid0;
      return fti;
   }
};

template <class A0>
Any delegVoid1(Any* fp1, Any* params, int64_t n)
{
   void (*fp)(A0) = *fp1;
   fp(params[0]);
   return Any(0);
}
template <class A0>
struct getType<void (*)(A0)>
{
   typedef void (*T)(A0);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind = kFunction;
      fti.delegPtr = delegVoid1<A0>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      return fti;
   }
};

template <class A0, class A1>
Any delegVoid2(Any* fp1, Any* params, int64_t n)
{
   void (*fp)(A0, A1) = *fp1;
   fp(params[0], params[1]);
   return Any(0);
}
template <class A0, class A1>
struct getType<void (*)(A0,A1)>
{
   typedef void (*T)(A0, A1);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind = kFunction;
      fti.delegPtr = delegVoid2<A0, A1>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      return fti;
   }
};

template <class A0, class A1, class A2>
Any delegVoid3(Any* fp1, Any* params, int64_t n)
{
   void (*fp)(A0, A1, A2) = *fp1;
   fp(params[0], params[1], params[2]);
   return Any(0);
}
template <class A0, class A1, class A2>
struct getType<void (*)(A0,A1,A2)>
{
   typedef void (*T)(A0, A1, A2);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind = kFunction;
      fti.delegPtr = delegVoid3<A0, A1, A2>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      return fti;
   }
};

template <class A0, class A1, class A2, class A3>
Any delegVoid4(Any* fp1, Any* params, int64_t n)
{
   void (*fp)(A0, A1, A2, A3) = *fp1;
   fp(params[0], params[1], params[2], params[3]);
   return Any(0);
}
template <class A0, class A1, class A2, class A3>
struct getType<void (*)(A0,A1,A2,A3)>
{
   typedef void (*T)(A0, A1, A2, A3);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind = kFunction;
      fti.delegPtr = delegVoid4<A0, A1, A2, A3>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      return fti;
   }
};

template <class A0, class A1, class A2, class A3, class A4>
Any delegVoid5(Any* fp1, Any* params, int64_t n)
{
   void (*fp)(A0, A1, A2, A3, A4) = *fp1;
   fp(params[0], params[1], params[2], params[3], params[4]);
   return Any(0);
}
template <class A0, class A1, class A2, class A3, class A4>
struct getType<void (*)(A0,A1,A2,A3,A4)>
{
   typedef void (*T)(A0, A1, A2, A3, A4);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind = kFunction;
      fti.delegPtr = delegVoid5<A0, A1, A2, A3, A4>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.members.add(new Member(getTypeAdd<A4>()));
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// FUNCTIONS
template <class R>
Any delegRes0(Any* fp1, Any* params, int64_t n)
{
   R (*fp)() = *fp1;
   return fp();
}
template <class R>
struct getType<R (*)()>
{
   typedef R (*T)();
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegRes0<R>;
      return fti;
   }
};

template <class R, class A0>
Any delegRes1(Any* fp1, Any* params, int64_t n)
{
   R (*fp)(A0) = *fp1;
   return fp(params[0]);
}
template <class R, class A0>
struct getType<R (*)(A0)>
{
   typedef R (*T)(A0);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegRes1<R, A0>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      return fti;
   }
};

template <class R, class A0, class A1>
Any delegRes2(Any* fp1, Any* params, int64_t n)
{
   R (*fp)(A0, A1) = *fp1;
   return fp(params[0], params[1]);
}
template <class R, class A0, class A1>
struct getType<R (*)(A0,A1)>
{
   typedef R (*T)(A0, A1);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegRes2<R, A0, A1>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      return fti;
   }
};

template <class R, class A0, class A1, class A2>
Any delegRes3(Any* fp1, Any* params, int64_t n)
{
   R (*fp)(A0, A1, A2) = *fp1;
   return fp(params[0], params[1], params[2]);
}
template <class R, class A0, class A1, class A2>
struct getType<R (*)(A0,A1,A2)>
{
   typedef R (*T)(A0, A1, A2);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegRes3<R, A0, A1, A2>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      return fti;
   }
};

template <class R, class A0, class A1, class A2, class A3>
Any delegRes4(Any* fp1, Any* params, int64_t n)
{
   R (*fp)(A0, A1, A2, A3) = *fp1;
   return fp(params[0], params[1], params[2], params[3]);
}
template <class R, class A0, class A1, class A2, class A3>
struct getType<R (*)(A0,A1,A2,A3)>
{
   typedef R (*T)(A0, A1, A2, A3);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegRes4<R, A0, A1, A2, A3>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      return fti;
   }
};

template <class R, class A0, class A1, class A2, class A3, class A4>
Any delegRes5(Any* fp1, Any* params, int64_t n)
{
   R (*fp)(A0, A1, A2, A3, A4) = *fp1;
   return fp(params[0], params[1], params[2], params[3], params[4]);
}
template <class R, class A0, class A1, class A2, class A3, class A4>
struct getType<R (*)(A0,A1,A2,A3,A4)>
{
   typedef R (*T)(A0, A1, A2, A3, A4);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegRes5<R, A0, A1, A2, A3, A4>;
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.members.add(new Member(getTypeAdd<A4>()));
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// MEMBER FUNCTIONS RETURNING VOID
template <class C>
Any delegVoidClass0(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)() = *fp1;
   (params[0].as<C>().*fp)();
   return Any(0);
}
template <class C>
struct getType<void (C::*)()>
{
   typedef void (C::*T)();
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidClass0<C>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0>
Any delegVoidClass1(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0) = *fp1;
   (params[0].as<C>().*fp)(params[1]);
   return Any(0);
}
template <class C, class A0>
struct getType<void (C::*)(A0)>
{
   typedef void (C::*T)(A0);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidClass1<C, A0>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0, class A1>
Any delegVoidClass2(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0, A1) = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2]);
   return Any(0);
}
template <class C, class A0, class A1>
struct getType<void (C::*)(A0,A1)>
{
   typedef void (C::*T)(A0, A1);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidClass2<C, A0, A1>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2>
Any delegVoidClass3(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0, A1, A2) = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3]);
   return Any(0);
}
template <class C, class A0, class A1, class A2>
struct getType<void (C::*)(A0,A1,A2)>
{
   typedef void (C::*T)(A0, A1, A2);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidClass3<C, A0, A1, A2>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2, class A3>
Any delegVoidClass4(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0, A1, A2, A3) = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4]);
   return Any(0);
}
template <class C, class A0, class A1, class A2, class A3>
struct getType<void (C::*)(A0,A1,A2,A3)>
{
   typedef void (C::*T)(A0, A1, A2, A3);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidClass4<C, A0, A1, A2, A3>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2, class A3, class A4>
Any delegVoidClass5(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0, A1, A2, A3, A4) = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4], params[5]);
   return Any(0);
}
template <class C, class A0, class A1, class A2, class A3, class A4>
struct getType<void (C::*)(A0,A1,A2,A3,A4)>
{
   typedef void (C::*T)(A0, A1, A2, A3, A4);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind     = kFunction;
      fti.delegPtr = delegVoidClass5<C, A0, A1, A2, A3, A4>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.members.add(new Member(getTypeAdd<A4>()));
      fti.member = true;
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// MEMBER FUNCTIONS
template <class R, class C>
Any delegResClass0(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)() = *fp1;
   return (params[0].as<C>().*fp)();
}
template <class R, class C>
struct getType<R (C::*)()>
{
   typedef R (C::*T)();
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind     = kFunction;
      fti.of       = getTypeAdd<R>();
      fti.delegPtr = delegResClass0<R, C>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.member   = true;
      return fti;
   }
};

template <class R, class C, class A0>
Any delegResClass1(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0) = *fp1;
   return (params[0].as<C>().*fp)(params[1]);
}
template <class R, class C, class A0>
struct getType<R (C::*)(A0)>
{
   typedef R (C::*T)(A0);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResClass1<R, C, A0>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1>
Any delegResClass2(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0, A1) = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2]);
}
template <class R, class C, class A0, class A1>
struct getType<R (C::*)(A0,A1)>
{
   typedef R (C::*T)(A0, A1);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResClass2<R, C, A0, A1>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2>
Any delegResClass3(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0, A1, A2) = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3]);
}
template <class R, class C, class A0, class A1, class A2>
struct getType<R (C::*)(A0,A1,A2)>
{
   typedef R (C::*T)(A0, A1, A2);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResClass3<R, C, A0, A1, A2>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2, class A3>
Any delegResClass4(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0, A1, A2, A3) = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4]);
}
template <class R, class C, class A0, class A1, class A2, class A3>
struct getType<R (C::*)(A0,A1,A2,A3)>
{
   typedef R (C::*T)(A0, A1, A2, A3);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResClass4<R, C, A0, A1, A2, A3>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2, class A3, class A4>
Any delegResClass5(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0, A1, A2, A3, A4) = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4], params[5]);
}
template <class R, class C, class A0, class A1, class A2, class A3, class A4>
struct getType<R (C::*)(A0,A1,A2,A3,A4)>
{
   typedef R (C::*T)(A0, A1, A2, A3, A4);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResClass5<R, C, A0, A1, A2, A3, A4>;
      fti.members.add(new Member(getTypeAdd<C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.members.add(new Member(getTypeAdd<A4>()));
      fti.member = true;
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// CONST MEMBER FUNCTIONS RETURNING VOID
template <class C>
Any delegVoidConstClass0(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)() const = *fp1;
   (params[0].as<C>().*fp)();
   return Any(0);
}
template <class C>
struct getType<void (C::*)() const>
{
   typedef void (C::*T)();
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidConstClass0<C>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0>
Any delegVoidConstClass1(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0) const = *fp1;
   (params[0].as<C>().*fp)(params[1]);
   return Any(0);
}
template <class C, class A0>
struct getType<void (C::*)(A0) const>
{
   typedef void (C::*T)(A0);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidConstClass1<C, A0>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0, class A1>
Any delegVoidConstClass2(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0, A1) const = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2]);
   return Any(0);
}
template <class C, class A0, class A1>
struct getType<void (C::*)(A0,A1) const>
{
   typedef void (C::*T)(A0, A1);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidConstClass2<C, A0, A1>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2>
Any delegVoidConstClass3(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0, A1, A2) const = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3]);
   return Any(0);
}
template <class C, class A0, class A1, class A2>
struct getType<void (C::*)(A0,A1,A2) const>
{
   typedef void (C::*T)(A0, A1, A2);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidConstClass3<C, A0, A1, A2>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2, class A3>
Any delegVoidConstClass4(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0, A1, A2, A3) const = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4]);
   return Any(0);
}
template <class C, class A0, class A1, class A2, class A3>
struct getType<void (C::*)(A0,A1,A2,A3) const>
{
   typedef void (C::*T)(A0, A1, A2, A3);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidConstClass4<C, A0, A1, A2, A3>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2, class A3, class A4>
Any delegVoidConstClass5(Any* fp1, Any* params, int64_t n)
{
   void (C::*fp)(A0, A1, A2, A3, A4) const = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4], params[5]);
   return Any(0);
}
template <class C, class A0, class A1, class A2, class A3, class A4>
struct getType<void (C::*)(A0,A1,A2,A3,A4) const>
{
   typedef void (C::*T)(A0, A1, A2, A3, A4);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.delegPtr   = delegVoidConstClass5<C, A0, A1, A2, A3, A4>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.members.add(new Member(getTypeAdd<A4>()));
      fti.member = true;
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// CONST MEMBER FUNCTIONS
template <class R, class C>
Any delegResConstClass0(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)() const = *fp1;
   return (params[0].as<C>().*fp)();
}
template <class R, class C>
struct getType<R (C::*)() const>
{
   typedef R (C::*T)();
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResConstClass0<R, C>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0>
Any delegResConstClass1(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0) const = *fp1;
   return (params[0].as<C>().*fp)(params[1]);
}
template <class R, class C, class A0>
struct getType<R (C::*)(A0) const>
{
   typedef R (C::*T)(A0);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResConstClass1<R, C, A0>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1>
Any delegResConstClass2(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0, A1) const = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2]);
}
template <class R, class C, class A0, class A1>
struct getType<R (C::*)(A0,A1) const>
{
   typedef R (C::*T)(A0, A1);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResConstClass2<R, C, A0, A1>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2>
Any delegResConstClass3(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0, A1, A2) const = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3]);
}
template <class R, class C, class A0, class A1, class A2>
struct getType<R (C::*)(A0,A1,A2) const>
{
   typedef R (C::*T)(A0, A1, A2);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResConstClass3<R, C, A0, A1, A2>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2, class A3>
Any delegResConstClass4(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0, A1, A2, A3) const = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4]);
}
template <class R, class C, class A0, class A1, class A2, class A3>
struct getType<R (C::*)(A0,A1,A2,A3) const>
{
   typedef R (C::*T)(A0, A1, A2, A3);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResConstClass4<R, C, A0, A1, A2, A3>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2, class A3, class A4>
Any delegResConstClass5(Any* fp1, Any* params, int64_t n)
{
   R (C::*fp)(A0, A1, A2, A3, A4) const = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4], params[5]);
}
template <class R, class C, class A0, class A1, class A2, class A3, class A4>
struct getType<R (C::*)(A0,A1,A2,A3,A4) const>
{
   typedef R (C::*T)(A0, A1, A2, A3, A4);
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind   = kFunction;
      fti.of = getTypeAdd<R>();
      fti.delegPtr   = delegResConstClass5<R, C, A0, A1, A2, A3, A4>;
      fti.members.add(new Member(getTypeAdd<const C>()));
      fti.members.add(new Member(getTypeAdd<A0>()));
      fti.members.add(new Member(getTypeAdd<A1>()));
      fti.members.add(new Member(getTypeAdd<A2>()));
      fti.members.add(new Member(getTypeAdd<A3>()));
      fti.members.add(new Member(getTypeAdd<A4>()));
      fti.member = true;
      return fti;
   }
};

//end AnySpecs

template <class T>
struct Array
{
   int64_t         dimensions;
   std::vector<int64_t> sizes;
   T*          mem;
   int64_t         totalSize;
   Array() : mem(nullptr)
   {
   }
   Array(int64_t dim) : dimensions(dim), sizes(dim), mem(nullptr)
   {
   }
   void alloc()
   {
      clear();
      totalSize = 1;
      dimensions = sizes.size();
      for (size_t d = 0; d < dimensions; ++d)
         totalSize *= sizes[d];
      mem = new T[totalSize];
   }
   Array(std::vector<int64_t> sizes) : sizes(sizes), mem(nullptr)
   {
      alloc();
   }
   T& operator[](std::vector<int64_t> p)
   {
      T* addr = mem;
      int64_t mult = 1;
      for (size_t d = 0; d < dimensions; ++d)
      {
         addr += p[d] * mult;
         mult *= sizes[d];
      }
      return *addr;
   }
   void clear()
   {
      delete[] mem;
      mem = nullptr;
   }
};

struct MMBase
{
   int64_t          minArgs;
   int64_t          maxArgs;
   bool             useReturnT;
   bool             useTable;
   std::vector<Any> methods;
   std::string      name;
   Array<Any>       table;
   std::vector<std::set<TypeInfo*>> paramTypes;
   int64_t          mmindex;
   static int64_t   mmcount;
   static std::set<MMBase*> mms;

   MMBase(std::string _name = "") : name(_name), useReturnT(false), useTable(false), minArgs(0), maxArgs(0), mmindex(mmcount++)
   {
      mms.insert(this);
   }
   ~MMBase()
   {
      mms.erase(this);
   }
};

template <class Result>
struct Multimethod : public MMBase
{
   Multimethod(std::string _name = "") : MMBase(_name)
   {
   }
   void add(Any method)
   {
      TypeInfo* ti = method.typeInfo;
      int64_t methodMaxArgs = method.maxArgs() + (useReturnT ? 1 : 0);
      int64_t methodMinArgs = method.minArgs() + (useReturnT ? 1 : 0);
      if (methods.size() == 0 || methodMaxArgs > maxArgs) maxArgs = methodMaxArgs;
      if (methods.size() == 0 || methodMinArgs < minArgs) minArgs = methodMinArgs;
      methods.push_back(method);
   }
   Result call(Any* args, int64_t nargs)
   {
      if (useTable)
         return callT(args, nargs);
      else
      {
#define DEBUGMM
#ifdef DEBUGMM
         std::cerr << "MULTIMETHOD " << name << " CALLED WITH: ";
         for (size_t ai = 0; ai < nargs; ++ai)
            std::cerr << args[ai].typeName() << "   ";
         std::cerr << std::endl;
#endif   
         for (size_t mi = 0; mi < methods.size(); ++mi)
         {
#ifdef DEBUGMM
            std::cerr << "trying: ";
            for (size_t ai = 0; ai < nargs; ++ai)
               std::cerr << methods[mi].paramType(ai)->getName() << "   ";
            std::cerr << std::endl;
#endif
            for (size_t ai = 0; ai < nargs; ++ai)
            {
               TypeInfo* fromt;
               TypeInfo* tot;
               if (useReturnT)
               {
                  if (ai == 0) tot = args[ai];
                  else         tot = args[ai].typeInfo;
                  fromt = methods[mi].paramType(ai - 1);
               }
               else
               {
                  fromt = args[ai].typeInfo;
                  tot = methods[mi].paramType(ai);
               }
               #ifdef DEBUGMM
               std::cerr << tot->name << "   ";
               #endif
               if (!(tot == tiAny || fromt->isPRTF(tot)))
               {
                  #ifdef DEBUGMM
                  std::cerr << " FAIL arg = " << fromt->getName() << std::endl;
                  #endif
                  goto nextMethod2;
               }
            }
            #ifdef DEBUGMM
            std::cerr << std::endl << " *** MATCH ***" << std::endl;
            #endif
            if (useReturnT)
               return methods[mi].call(args+1, nargs-1).as<Result>();
            else
               return methods[mi].call(args, nargs).as<Result>();
   nextMethod2:;
         }
         throw "multimethod not applicable to type(s)";
      }
   }
   Result operator()(Any arg0)
   {
      return call(&arg0, 1);
   }
   Result operator()(Any arg0, Any arg1)
   {
      Any args[2] = { arg0, arg1 };
      return call(args, 2);
   }      
   Result operator()(Any arg0, Any arg1, Any arg2)
   {
      Any args[3] = { arg0, arg1, arg2 };
      return call(args, 3);
   }
   Result callT(Any* args, int64_t nargs)
   {
      std::vector<int64_t> indices(maxArgs);
      if (useReturnT)
      {
         indices[0] = args[0].as<TypeInfo*>()->mmpindices[mmindex][0];
         for (int64_t ai = 1; ai < nargs; ++ai)
            indices[ai] = args[ai].typeInfo->mmpindices[mmindex][ai];
         return table[indices].call(args+1, nargs-1);
      }
      else
      {
         for (int64_t ai = 0; ai < nargs; ++ai)
            indices[ai] = args[ai].typeInfo->mmpindices[mmindex][ai];
         return table[indices].call(args, nargs);
      }
   }
   void insertParamType(int64_t pi, TypeInfo* t)
   {
      if (paramTypes[pi].insert(t).second)
      { 
         if (t->mmpindices.size() <= mmindex)
            t->mmpindices.resize(mmindex + 1);
         if (t->mmpindices[mmindex].size() <= pi)
            t->mmpindices[mmindex].resize(pi + 1);
         t->mmpindices[mmindex][pi] = paramTypes[pi].size() - 1;
         //for (auto d : t->derived)
         //   insertParamType(pi, d->derived);
      }
   }
   void buildTable()
   {
      table.clear();
      paramTypes.clear();
      paramTypes.resize(maxArgs);
      for (auto &m : methods)
         for (size_t pi = 0; pi < m.typeInfo->members.size() + useReturnT; ++pi)
            insertParamType(pi, m.paramType(pi, useReturnT));

      table.sizes.resize(maxArgs);
      for (size_t pi = 0; pi < maxArgs; ++ pi)
         table.sizes[pi] = paramTypes[pi].size();
      table.alloc();
      std::vector<Any*> validMethods;
      for (auto &m : methods) validMethods.push_back(&m);//make a vector of pointers so we can easily test for equality
      std::vector<TypeInfo*> at(maxArgs);
      simulate(validMethods, at, 0);
   }
   void simulate(std::vector<Any*> &validMethods, std::vector<TypeInfo*> &at, int64_t ai)
   {
#if 1
      //step 1
      if (ai < maxArgs)
      {
         for (auto at1 : paramTypes[ai])
         {
            std::vector<Any*> validMethodsNew;
            at[ai] = at1;
            for (auto m : validMethods)
               if (hasLink(m->paramType(ai, useReturnT), at1))
                  validMethodsNew.push_back(m);
            simulate(validMethodsNew, at, ai + 1);
         }
      }
      //step 2
      else
      {
         //at this point, validMethods is a list of the valid methods for the argument types in 'at'
         std::vector<Any*> bestMethods(validMethods);
         std::vector<std::set<TypeInfo*>> validParamTypes(maxArgs);
         //get all the param types of the VALID methods
         for (size_t ai2 = 0; ai2 < maxArgs; ++ai2)
            for (auto m : validMethods)
               validParamTypes[ai2].insert(m->paramType(ai2, useReturnT));
         for (size_t ai2 = 0; ai2 < maxArgs; ++ai2)
         {
            auto atlb = at[ai2]->linear.begin();//atl = argument type linearisation
            auto atle = at[ai2]->linear.end();
            auto atlmin = atle;
            TypeInfo* ptmin = nullptr;
            for (auto pt : validParamTypes[ai2])
            {
               auto atl = std::find(atlb, atle, pt);
               if (atl < atlmin)
               {
                  atlmin = atl;
                  ptmin = pt;
               }
            }
            if (!ptmin) throw std::runtime_error("Algorithm failure in multimethod::simulate - parameter type found in step 1 has seemingly disappeared in step 2");
            std::vector<Any*> bestMethodsNew;
            for (auto m : bestMethods)
               if (m->paramType(ai2, useReturnT) == ptmin)
                  bestMethodsNew.push_back(m);
            bestMethods = std::move(bestMethodsNew);
         }
         if (bestMethods.size() == 1)
         {
            std::vector<int64_t> ati(maxArgs);
            for (int64_t ai2 = 0; ai2 < maxArgs; ++ai2)
               ati[ai2] = at[ai2]->mmpindices[mmindex][ai2];
            table[ati] = *bestMethods.front();
         }
         else if (bestMethods.size() == 0)
         {
            std::cout << "warning: no best method in multimethod " << name << " for argument types ";
            for (size_t ai2 = 0; ai2 < maxArgs; ++ai2)
               std::cout << at[ai2]->getName() << "  ";
            std::cout << std::endl;
         }
         else
         {
            std::cout << "very strange indeed!" << std::endl;
         }
      }
      return;
#endif
   }
};
/*
* remember that actual arguments are copied to formal parameters
* so arguments must be convertible to parameter types
* so parameters must be a base of the arguments
* 
* thoughts on a faster algorithm
* 
* an argument list that is the same is an error, so ignore those
* an argument that is not derived from the parameter is invalid
* an argument list that is all the same or worse is replaced
* an argument list that is all the same or better replaces
* so that only leaves: a blocking argument list must be better on at least one argument and worse on at least one
*                 and: a list with an argument that multiply inherits from two different parameter types (but could decide by linearisation)
*
* sort the methods by the type of each parameter
* 
* for each method
*   mark the exact parameters in the table
*   depth first search through derived classes of parameters (don't forget this is multi-dimensional due to multiple parameters)
*     when we hit a type with a method with a parameter that is derived:
*       stop and check the other parameters to see if it blocks or replaces
*     what happens when we meet up with the search from another method with a parameter of a different base due to multiple inheritance?
*        use linearisation to decide?
*        how do we detect this?
*
* can initialise a const int64_t with an int64_t
* cannot assign to a const int64_t with anything
* 
*                           can assign this
*  to this         |int64_t *|const int64_t *|int64_t * const|const int64_t * const|
*       int64_t *      |ia   |           |ia         |
* const int64_t *      |
*       int64_t * const|
* const int64_t * const|
*/
#if 1
template <>
struct Multimethod<void> : public Multimethod<Any>
{
   Multimethod(std::string _name = "") : Multimethod<Any>(_name)
   {
   }
   void call(Any* args, int64_t nargs)
   {
      Multimethod<Any>::call(args, nargs);
   }
   void operator()(Any arg0)
   {
      call(&arg0, 1);
   }
   void operator()(Any arg0, Any arg1)
   {
      Any args[2] = { arg0, arg1 };
      call(args, 2);
   }      
   void operator()(Any arg0, Any arg1, Any arg2)
   {
      Any args[3] = { arg0, arg1, arg2 };
      call(args, 3);
   }      
};
#endif
typedef Multimethod<Any> MultimethodAny;

template <class R>
Any delegMM(Any* mm, Any* args, int64_t n)
{
   return mm->as<Multimethod<R> >().call(args, n);
}

template <class R>
struct getType<Multimethod<R> >
{
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<Multimethod<R> >());
      fti.multimethod = true;
      fti.of = getTypeAdd<R>();
      //for (size_t i = 0; i < mm.nargs; ++i)
      //   fti.members.add(getTypeAdd<Any>());
      fti.delegPtr = delegMM<R>;
      return fti;
   }
};

template <class R>
Any delegMMP(Any* mm, Any* args, int64_t n)
{
   return mm->as<Multimethod<R>*>()->call(args, n);
}

template <class R>
struct getType<Multimethod<R>*>
{
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<Multimethod<R>*>());
      fti.multimethod = true;
      fti.of = getTypeAdd<R>();
      //for (size_t i = 0; i < mm.nargs; ++i)
      //   fti.members.add(getTypeAdd<Any>());
      fti.delegPtr = delegMMP<R>;
      fti.kind = kPointer;
      fti.of = getTypeAdd<Multimethod<R> >();
      return fti;
   }
};

void doC3Lin(TypeInfo* type);

struct MissingArg
{
};

extern MissingArg missingArg;
extern MissingArg mA;

struct PartApply
{
   int64_t pArgsFixed;
   std::vector<Any> argsFixed;
   Any      (*delegPtr)(Any* fp , Any* params, int64_t np);
};

PartApply* makePartApply(Any f, Any a0);
PartApply* makePartApply(Any f, Any a0, Any a1);

Any delegPartApply(Any* cl, Any* args, int64_t n);

template <>
struct getType<PartApply*>
{
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<PartApply*>());
      //fti.of = getTypeAdd<R>();
      //for (size_t i = 0; i < mm.nargs; ++i)
      //   fti.members.add(getTypeAdd<Any>());
      fti.delegPtr = delegPartApply;
      fti.kind = kPointer;
      fti.of = getTypeAdd<PartApply>();
      return fti;
   }
};

typedef Any (*VFAux)(Any*, int64_t);

struct VarFunc
{
   int64_t         minArgs;
   int64_t         maxArgs;
   Any         func;
   VarFunc(Any func, int64_t minArgs, int64_t maxArgs) : func(func), minArgs(minArgs), maxArgs(maxArgs)
   {
   }
};

Any delegVF(Any* fp, Any* args, int64_t n);

template <>
struct getType<VarFunc>
{
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<VarFunc>());
      fti.of = getTypeAdd<Any>();
      fti.delegPtr = delegVF;
      return fti;
   }
};
                
struct Var
{
   std::string   name;
   Any      value;
   int64_t      offset;
   Var(                           ) :                           offset(0) {}
   Var(std::string name           ) : name(name),               offset(0) {}
   Var(std::string name, Any value) : name(name), value(value), offset(0) {}
};

struct Frame;
struct Lambda;

namespace List
{

struct Closure
{
   std::deque<Var>  params;
   Var         rest;
   Any         body;
   Frame*      context;
   int64_t         minArgs;
   int64_t         maxArgs;

   Closure() : context(nullptr), minArgs(0), maxArgs(0) {}
   Closure(Any params, Any body, Frame* context = nullptr);
};

Any closureDeleg(Any* closure, Any* params, int64_t np);

}

namespace Structure
{
   struct Stack
   {
      Stack* context;
   };

   struct Closure
   {
      Lambda* lambda;
      Stack*  context;
   };

}

template <>
struct getType<List::Closure*>
{
   typedef List::Closure* T;
   static TypeInfo info()
   {
      TypeInfo fti(getTypeBase<T>());
      fti.kind = kPointer;
      fti.of = getTypeAdd<List::Closure>();
      fti.delegPtr = List::closureDeleg;
      return fti;
   }
};


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
