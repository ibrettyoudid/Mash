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
   lVirtualSub, //virtual inheritance but attempt to do it without pointers
   lConv,
   lDelegate //isn't this the same as conversion? yes i think it is
};

enum Kind
{
   kNormal = 0,
   kPrimitive,
   kStruct,
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

void error(std::string message);

typedef std::vector<Any>        Vec;

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

struct AnyBase
{
   void* payload;
   TypeInfo* typeInfo;

   AnyBase() {}
   AnyBase(TypeInfo* typeInfo, void* payload);

   bool canderp();
   AnyBase derp();
   bool candeany();
   AnyBase deany();
};

struct AnyRef : public AnyBase
{
};

struct Any : public AnyBase
{
   typedef std::input_iterator_tag iterator_category;

   template <class Type> void construct(Type value);
   Any();
   template <class Type> Any(Type value);
   template <class Type> Any(Type& value, Dynamic);//detect type dynamically using C++ RTTI - takes a reference but creates a plain value
   template <class Type> Any(Type* value);//detect type dynamically using C++ RTTI - takes a pointer and creates a pointer
   template <class Type> Any(Type& value, Ref);    //creates a reference
   Any(const Any& rhs);
   Any(Any &&rhs);
   Any& operator= (const Any& rhs);
   Any& operator= (Any&& rhs);
   Any(TypeInfo* typeInfo, void* rhs);
   ~Any();
   bool empty();
   Any call(Any* args, int64_t n);
   Any operator()();
   Any operator()(Any arg0);
   Any operator()(Any arg0, Any arg1);
   Any operator()(Any arg0, Any arg1, Any arg2);
   Any operator()(Any arg0, Any arg1, Any arg2, Any arg3);
   Any operator()(Any arg0, Any arg1, Any arg2, Any arg3, Any arg4);
   //bool canderp();
   Any derp();
   //bool candeany();
   Any deany();
   Any* deany1();
   bool candeany2();
   Any* deany2();
   Any toRef();
   Any toPtr();
   Any toAny();
   //--------------------------------------------------------------------------- CONVERSION
   template <class To>   operator To&();
   template <class To>   To& as();
   template <class To>   To  as2();
   template <class To>   To* any_cast();
   operator MMBase& ();
   //---------------------------------------------------------------------------------
   std::string typeName    ();
   TypeInfo*   paramType   (int64_t n, bool useReturnT = false);
   bool        isPtrTo     (TypeInfo* other);
   bool        isRefTo     (TypeInfo* other);
   bool        isPRTF      (TypeInfo* other);
   int64_t     maxArgs     ();
   int64_t     minArgs     ();
   bool        callable    ();
   void        showhex     ();
   std::string hex         ();
   int64_t     rDepth      ();
   int64_t     pDepth      ();
   int64_t     prDepth     ();
   void        add         (Member* m, Any value);
   Any         getMember   (Member* m);
   Any         getMemberRef(Member* m);
   Any         getMemberPtr(Member* m);
   void        setMember   (Member* m, Any value);
   Any         getMemberRef(Symbol* symbol);
   Any         getMemberRef(std::string name);
   Any         getMemberRef(int64_t n);
   Member*     findMember  (Symbol* symbol);
   Member*     findMember  (std::string name);
   Member*     findMember  (int64_t n);
   void*       getMemberAddress(Member* m);
};

struct TypeLink
{
   TypeInfo* base;
   TypeInfo* derived;
   LinkKind  kind;
   int64_t   offset;
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

struct TypeConn
{
   TypeLinkList     bases;
   TypeLinkList     derived;
   TypeList         linear;
};

struct TypeInfo
{
   std::string      name;
   int64_t          size;
   Kind             kind;
   TypeInfo*        of = nullptr;

   MemberList       members;
   MemberList       allMembers;

   TypeConn         main;
   TypeConn         conv;
   TypePathList     layout;

   void             (*copyCon )(void* res, void* in, TypeInfo* ti) = nullptr;//set in getTypeBase
   void             (*destruct )(void* target, TypeInfo* ti)       = nullptr;
   Any              (*delegPtr)(Any* fp , Any* params, int64_t np) = nullptr;

   NKind            nKind;//Signed/unsigned/floating

   bool             cpp;
   const type_info* cType;

   bool             number      = false;
   bool             member      = false;//member function or member pointer (memptrs are callable)
   bool             multimethod = false;

   int64_t          count;
   std::string      fullName;
   TypeInfo*        ptr = nullptr;
   TypeInfo*        ref = nullptr;

   std::vector<std::vector<int64_t>> mmpindices;

   TypeInfo(std::string name = "") : name(name), kind(kNormal)
   {
   }


   void        doC3Lin   (TypeConn TypeInfo::*conn);
   void        allocate  ();
   void        allocate2 (TypeInfo* derived, int64_t baseOffset, TypeLinkList& links);
   TypeInfo*   argType   (int64_t n, bool useReturnT);
   TypeInfo*   unref     ();
   TypeInfo*   unptr     (int64_t n);
   TypeInfo*   toRef     ();
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
struct Destruct
{
   static void go(void* target, TypeInfo*)
   {
      T* t = static_cast<T*>(target);
      if (!t) throw "null pointer!";
      t->~T();
   }
};

template <class T>
struct Destruct<T&>
{
   static void go(void* target, TypeInfo*)
   {
      //T** t = static_cast<T**>(target);
      //(*t)->~T();
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

static void copynothing(void* res, void* in, TypeInfo* ti)
{
}

static void destructnothing(void*, TypeInfo*)
{
}

TypeInfo* newTypeInterp(std::string name);

template <class Type> TypeInfo* getTypeAdd();

template <class Type>
struct getType
{
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->delegPtr = nullptr;
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct  = Destruct<Type>::go;
      //getTypeAdd<Type*>();
      //getTypeAdd<Type&>();
      return typeInfo;
   }
};

template <> struct getType<void>
{
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp = true;
      typeInfo->cType = &typeid(void);
      typeInfo->name = "void";
      typeInfo->fullName = "void";
      typeInfo->size = 0;
      typeInfo->delegPtr = nullptr;
      typeInfo->copyCon = copynothing;
      typeInfo->destruct = destructnothing;
      return typeInfo;
   }
};

template <class To>
struct getType<To&>
{
   typedef To& Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(To*);
      typeInfo->delegPtr = nullptr;
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct = Destruct<Type>::go;
      typeInfo->kind     = kReference;
      typeInfo->of       = getTypeAdd<To>();
      typeInfo->of->ref  = typeInfo;
      return typeInfo;
   }
};

template <class To>
struct getType<To*>//Closure* and Multimethod* specialise this further
{
   typedef To* Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->delegPtr = nullptr;
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct = Destruct<Type>::go;
      typeInfo->kind     = kPointer;
      typeInfo->of       = getTypeAdd<To>();
      typeInfo->of->ptr  = typeInfo;
      return typeInfo;
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
   typedef R Type[n];
   static TypeInfo go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->delegPtr = nullptr;
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct  = Destruct<Type>::go;
      typeInfo->kind     = kArray;
      typeInfo->count    = n;
      typeInfo->of       = getTypeAdd<R>();
      return typeInfo;
   }
};

#define TYPESET

template <class Type>
struct getTypeAddStr
{
   static TypeInfo* go()
   {
#ifdef TYPESET
      auto i = typeSet.find(&typeid(Type));
      if (i == typeSet.end())
      {  
         TypeInfo* ti = getType<Type>::go();
         typeSet.insert(ti);
         return ti;
      }
      return *i;
#else
      TypeInfo*&          ti = typeMap[&typeid(Type)];
      if (ti == nullptr)  ti = getType<Type>::go();
      return ti;
#endif
   }
};

template <class Type>
struct getTypeAddStr<Type&>
{
   static TypeInfo* go()
   {
#ifdef TYPESET
      auto i = typeSetR.find(&typeid(Type));
      if (i == typeSetR.end())
      {  
         TypeInfo* ti = getType<Type&>::go();
         typeSetR.insert(ti);
         return ti;
      }
      return *i;
#else
      TypeInfo*&          ti = typeMapR[&typeid(Type)];
      if (ti == nullptr)  ti = getType<Type&>::go();
      return ti;
#endif
   }
};

//we don't want to have to pass around a Type so we want a function that doesn't take it as an argument,
//so we have to pass on the call to a struct template because we can't use function overloading to specialise without an argument
template <class Type>
TypeInfo* getTypeAdd()
{
   return getTypeAddStr<Type>::go();
}

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
void addTypeLinkConv(TypeInfo* base, TypeInfo* derived);

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
   base.main->derived.push_back(tl);
   derived.main->bases.push_back(tl);
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
   Any       (*getMemberPtr)(Member* mem, Any& ca);
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
   payload = new char[typeInfo->size];
   typeInfo->copyCon(payload, &value, typeInfo);
}
template <class Type> Any::Any(Type value)
{
   typeInfo = getTypeAdd<Type>();
   payload = new char[typeInfo->size];
   typeInfo->copyCon(payload, &value, typeInfo);
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
#ifdef TYPESET
   typeInfo = *typeSet.find(&typeid(value));
#else
   typeInfo = typeMap[&typeid(value)];
#endif
   payload = new char[typeInfo->size];
   typeInfo->copyCon(payload, &value, typeInfo);
}
template <class Type> Any::Any(Type* value)//detect type dynamically using C++ RTTI - takes a pointer and creates a pointer
{
#ifdef TYPESET
   auto i = typeSet.find(&typeid(*value));
   if (i != typeSet.end())
      typeInfo = *i;
   else
      typeInfo = nullptr;
#else
   if (typeMap.contains(&typeid(*value)))
      typeInfo = typeMap[&typeid(*value)];
   else
      typeInfo = nullptr;
#endif
   if (typeInfo)
      typeInfo = typeInfo->ptr;
   if (!typeInfo)
   {
      typeInfo = getTypeAdd<Type*>();
      /*
      if (typeMap[&typeid(*value)] != typeInfo->of)
      {
         std::ostringstream o;
         o << typeid(*value).name() << " should be " << typeInfo->of->cType->name();
         //error(o.str());
      }
      */
   }

   payload = new char[typeInfo->size];
   typeInfo->copyCon(payload, &value, typeInfo);
}
template <class Type> Any::Any(Type& value, Ref)//creates a reference
{
   typeInfo = getTypeAdd<Type&>();
   payload = new char[typeInfo->size];
   Type* temp = &value;
   typeInfo->copyCon(payload, &temp, typeInfo);
}
#if 1
template <class To>
To& Any::as()
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
      std::ostringstream o;
      o << "types unrelated: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
      std::cout << o.str() << std::endl;
      throw std::runtime_error(o.str());
   }
   int64_t indirs = toPL - fromRL - fromPL;
   if (indirs > 1)
   {
      std::ostringstream o;
      o << "cannot add more than 1 indirection: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
      std::cout << o.str() << std::endl;
      throw std::runtime_error(o.str());
   }
   if (indirs < -3)
   {
      std::ostringstream o;
      o << "cannot remove more than 3 indirections: attempt to cast from " << typeInfo->getName() << " to " << toType1->getName();
      std::cout << o.str() << std::endl;
      throw std::runtime_error(o.str());
   }
   //below, the number of asterisks in the < > must equal the number of dereferences between return and cast (in order for the types to be correct)
   switch (indirs)
   {
      //case 2:
      //   return double_cast<To>(&ptr);
      case 1:
         return *double_cast<To*>(&payload);

       //in cases below, type in < > is the from type, both To and From may be pointer/pointer-to-pointer/etc. types
      case 0:
         return *static_cast<To*>(payload);
      case -1:
         return **static_cast<To**>(payload);
      case -2:
         return ***static_cast<To***>(payload);
      case -3:
         return ****static_cast<To****>(payload);
   }
   throw "HUH?";
}

bool canCast(TypeInfo* toType, TypeInfo* fromType);
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
Any::operator To&()
{
   return as<To>();
}
template <class To>
To* Any::any_cast()
{
   TypeInfo* typeInfoTo = getTypeAdd<To>();
   //if (typeInfoTo == tiAny) return reinterpret_cast<To*>(this);
   if (typeInfo->isRTF(typeInfoTo)) return static_cast<To*>(payload);
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
Any getMemberPtr(Member* mem, Any &ca)
{
   R C::* ptr = mem->ptrToMember.as<R C::*>();
   C*     c   = ca.as<C*>();
   return Any(&(c->*ptr));
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
   pm->getMemberPtr = getMemberPtr<C, R>;
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
   typedef R C::* Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->delegPtr = delegPtrMem<R, C>;
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct  = Destruct<Type>::go;
      typeInfo->kind     = kPtrMem;
      typeInfo->member   = true;
      typeInfo->of       = getTypeAdd<R>();
      typeInfo->members.add(new Member(getSymbol("class"), getTypeAdd<C>()));
      return typeInfo;
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
   typedef void (*Type)();
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoid0;
      return fti;
   }
};

template <class A0>
Any delegVoid1(Any* fp1, Any* params, int64_t n)
{
   if (n != 1) throw "wrong number of params";
   void (*fp)(A0) = *fp1;
   fp(params[0]);
   return Any();
}
template <class A0>
struct getType<void (*)(A0)>
{
   typedef void (*Type)(A0);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoid1<A0>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      return fti;
   }
};

template <class A0, class A1>
Any delegVoid2(Any* fp1, Any* params, int64_t n)
{
   if (n != 2) throw "wrong number of params";
   void (*fp)(A0, A1) = *fp1;
   fp(params[0], params[1]);
   return Any();
}
template <class A0, class A1>
struct getType<void (*)(A0,A1)>
{
   typedef void (*Type)(A0, A1);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoid2<A0, A1>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      return fti;
   }
};

template <class A0, class A1, class A2>
Any delegVoid3(Any* fp1, Any* params, int64_t n)
{
   if (n != 3) throw "wrong number of params";
   void (*fp)(A0, A1, A2) = *fp1;
   fp(params[0], params[1], params[2]);
   return Any();
}
template <class A0, class A1, class A2>
struct getType<void (*)(A0,A1,A2)>
{
   typedef void (*Type)(A0, A1, A2);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoid3<A0, A1, A2>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      return fti;
   }
};

template <class A0, class A1, class A2, class A3>
Any delegVoid4(Any* fp1, Any* params, int64_t n)
{
   if (n != 4) throw "wrong number of params";
   void (*fp)(A0, A1, A2, A3) = *fp1;
   fp(params[0], params[1], params[2], params[3]);
   return Any();
}
template <class A0, class A1, class A2, class A3>
struct getType<void (*)(A0,A1,A2,A3)>
{
   typedef void (*Type)(A0, A1, A2, A3);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoid4<A0, A1, A2, A3>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      return fti;
   }
};

template <class A0, class A1, class A2, class A3, class A4>
Any delegVoid5(Any* fp1, Any* params, int64_t n)
{
   if (n != 5) throw "wrong number of params";
   void (*fp)(A0, A1, A2, A3, A4) = *fp1;
   fp(params[0], params[1], params[2], params[3], params[4]);
   return Any();
}
template <class A0, class A1, class A2, class A3, class A4>
struct getType<void (*)(A0,A1,A2,A3,A4)>
{
   typedef void (*Type)(A0, A1, A2, A3, A4);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoid5<A0, A1, A2, A3, A4>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->members.add(new Member(getTypeAdd<A4>()));
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// FUNCTIONS
template <class R>
Any delegRes0(Any* fp1, Any* params, int64_t n)
{
   if (n != 0) throw "wrong number of params";
   R (*fp)() = *fp1;
   return fp();
}
template <class R>
struct getType<R (*)()>
{
   typedef R (*Type)();
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegRes0<R>;
      return fti;
   }
};

template <class R, class A0>
Any delegRes1(Any* fp1, Any* params, int64_t n)
{
   if (n != 1) throw "wrong number of params";
   R (*fp)(A0) = *fp1;
   return fp(params[0]);
}
template <class R, class A0>
struct getType<R (*)(A0)>
{
   typedef R (*Type)(A0);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegRes1<R, A0>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      return fti;
   }
};

template <class R, class A0, class A1>
Any delegRes2(Any* fp1, Any* params, int64_t n)
{
   if (n != 2) throw "wrong number of params";
   R (*fp)(A0, A1) = *fp1;
   return fp(params[0], params[1]);
}
template <class R, class A0, class A1>
struct getType<R (*)(A0,A1)>
{
   typedef R (*Type)(A0, A1);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegRes2<R, A0, A1>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      return fti;
   }
};

template <class R, class A0, class A1, class A2>
Any delegRes3(Any* fp1, Any* params, int64_t n)
{
   if (n != 3) throw "wrong number of params";
   R (*fp)(A0, A1, A2) = *fp1;
   return fp(params[0], params[1], params[2]);
}
template <class R, class A0, class A1, class A2>
struct getType<R (*)(A0,A1,A2)>
{
   typedef R (*Type)(A0, A1, A2);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegRes3<R, A0, A1, A2>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      return fti;
   }
};

template <class R, class A0, class A1, class A2, class A3>
Any delegRes4(Any* fp1, Any* params, int64_t n)
{
   if (n != 4) throw "wrong number of params";
   R (*fp)(A0, A1, A2, A3) = *fp1;
   return fp(params[0], params[1], params[2], params[3]);
}
template <class R, class A0, class A1, class A2, class A3>
struct getType<R (*)(A0,A1,A2,A3)>
{
   typedef R (*Type)(A0, A1, A2, A3);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegRes4<R, A0, A1, A2, A3>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      return fti;
   }
};

template <class R, class A0, class A1, class A2, class A3, class A4>
Any delegRes5(Any* fp1, Any* params, int64_t n)
{
   if (n != 5) throw "wrong number of params";
   R (*fp)(A0, A1, A2, A3, A4) = *fp1;
   return fp(params[0], params[1], params[2], params[3], params[4]);
}
template <class R, class A0, class A1, class A2, class A3, class A4>
struct getType<R (*)(A0,A1,A2,A3,A4)>
{
   typedef R (*Type)(A0, A1, A2, A3, A4);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegRes5<R, A0, A1, A2, A3, A4>;
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->members.add(new Member(getTypeAdd<A4>()));
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// MEMBER FUNCTIONS RETURNING VOID
template <class C>
Any delegVoidClass0(Any* fp1, Any* params, int64_t n)
{
   if (n != 1) throw "wrong number of params";
   void (C::*fp)() = *fp1;
   (params[0].as<C>().*fp)();
   return Any();
}
template <class C>
struct getType<void (C::*)()>
{
   typedef void (C::*Type)();
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidClass0<C>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0>
Any delegVoidClass1(Any* fp1, Any* params, int64_t n)
{
   if (n != 2) throw "wrong number of params";
   void (C::*fp)(A0) = *fp1;
   (params[0].as<C>().*fp)(params[1]);
   return Any();
}
template <class C, class A0>
struct getType<void (C::*)(A0)>
{
   typedef void (C::*Type)(A0);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidClass1<C, A0>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0, class A1>
Any delegVoidClass2(Any* fp1, Any* params, int64_t n)
{
   if (n != 3) throw "wrong number of params";
   void (C::*fp)(A0, A1) = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2]);
   return Any();
}
template <class C, class A0, class A1>
struct getType<void (C::*)(A0,A1)>
{
   typedef void (C::*Type)(A0, A1);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidClass2<C, A0, A1>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2>
Any delegVoidClass3(Any* fp1, Any* params, int64_t n)
{
   if (n != 4) throw "wrong number of params";
   void (C::*fp)(A0, A1, A2) = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3]);
   return Any();
}
template <class C, class A0, class A1, class A2>
struct getType<void (C::*)(A0,A1,A2)>
{
   typedef void (C::*Type)(A0, A1, A2);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidClass3<C, A0, A1, A2>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2, class A3>
Any delegVoidClass4(Any* fp1, Any* params, int64_t n)
{
   if (n != 5) throw "wrong number of params";
   void (C::*fp)(A0, A1, A2, A3) = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4]);
   return Any();
}
template <class C, class A0, class A1, class A2, class A3>
struct getType<void (C::*)(A0,A1,A2,A3)>
{
   typedef void (C::*Type)(A0, A1, A2, A3);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidClass4<C, A0, A1, A2, A3>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2, class A3, class A4>
Any delegVoidClass5(Any* fp1, Any* params, int64_t n)
{
   if (n != 6) throw "wrong number of params";
   void (C::*fp)(A0, A1, A2, A3, A4) = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4], params[5]);
   return Any();
}
template <class C, class A0, class A1, class A2, class A3, class A4>
struct getType<void (C::*)(A0,A1,A2,A3,A4)>
{
   typedef void (C::*Type)(A0, A1, A2, A3, A4);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidClass5<C, A0, A1, A2, A3, A4>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->members.add(new Member(getTypeAdd<A4>()));
      fti->member = true;
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// MEMBER FUNCTIONS
template <class R, class C>
Any delegResClass0(Any* fp1, Any* params, int64_t n)
{
   if (n != 1) throw "wrong number of params";
   R (C::*fp)() = *fp1;
   return (params[0].as<C>().*fp)();
}
template <class R, class C>
struct getType<R (C::*)()>
{
   typedef R (C::*Type)();
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResClass0<R, C>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0>
Any delegResClass1(Any* fp1, Any* params, int64_t n)
{
   if (n != 2) throw "wrong number of params";
   R (C::*fp)(A0) = *fp1;
   return (params[0].as<C>().*fp)(params[1]);
}
template <class R, class C, class A0>
struct getType<R (C::*)(A0)>
{
   typedef R (C::*Type)(A0);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResClass1<R, C, A0>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1>
Any delegResClass2(Any* fp1, Any* params, int64_t n)
{
   if (n != 3) throw "wrong number of params";
   R (C::*fp)(A0, A1) = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2]);
}
template <class R, class C, class A0, class A1>
struct getType<R (C::*)(A0,A1)>
{
   typedef R (C::*Type)(A0, A1);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResClass2<R, C, A0, A1>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2>
Any delegResClass3(Any* fp1, Any* params, int64_t n)
{
   if (n != 4) throw "wrong number of params";
   R (C::*fp)(A0, A1, A2) = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3]);
}
template <class R, class C, class A0, class A1, class A2>
struct getType<R (C::*)(A0,A1,A2)>
{
   typedef R (C::*Type)(A0, A1, A2);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResClass3<R, C, A0, A1, A2>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2, class A3>
Any delegResClass4(Any* fp1, Any* params, int64_t n)
{
   if (n != 5) throw "wrong number of params";
   R (C::*fp)(A0, A1, A2, A3) = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4]);
}
template <class R, class C, class A0, class A1, class A2, class A3>
struct getType<R (C::*)(A0,A1,A2,A3)>
{
   typedef R (C::*Type)(A0, A1, A2, A3);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResClass4<R, C, A0, A1, A2, A3>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2, class A3, class A4>
Any delegResClass5(Any* fp1, Any* params, int64_t n)
{
   if (n != 6) throw "wrong number of params";
   R (C::*fp)(A0, A1, A2, A3, A4) = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4], params[5]);
}
template <class R, class C, class A0, class A1, class A2, class A3, class A4>
struct getType<R (C::*)(A0,A1,A2,A3,A4)>
{
   typedef R (C::*Type)(A0, A1, A2, A3, A4);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResClass5<R, C, A0, A1, A2, A3, A4>;
      fti->members.add(new Member(getTypeAdd<C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->members.add(new Member(getTypeAdd<A4>()));
      fti->member = true;
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// CONST MEMBER FUNCTIONS RETURNING VOID
template <class C>
Any delegVoidConstClass0(Any* fp1, Any* params, int64_t n)
{
   if (n != 1) throw "wrong number of params";
   void (C::*fp)() const = *fp1;
   (params[0].as<C>().*fp)();
   return Any();
}
template <class C>
struct getType<void (C::*)() const>
{
   typedef void (C::*Type)();
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidConstClass0<C>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0>
Any delegVoidConstClass1(Any* fp1, Any* params, int64_t n)
{
   if (n != 2) throw "wrong number of params";
   void (C::*fp)(A0) const = *fp1;
   (params[0].as<C>().*fp)(params[1]);
   return Any();
}
template <class C, class A0>
struct getType<void (C::*)(A0) const>
{
   typedef void (C::*Type)(A0);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidConstClass1<C, A0>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0, class A1>
Any delegVoidConstClass2(Any* fp1, Any* params, int64_t n)
{
   if (n != 3) throw "wrong number of params";
   void (C::*fp)(A0, A1) const = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2]);
   return Any();
}
template <class C, class A0, class A1>
struct getType<void (C::*)(A0,A1) const>
{
   typedef void (C::*Type)(A0, A1);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidConstClass2<C, A0, A1>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2>
Any delegVoidConstClass3(Any* fp1, Any* params, int64_t n)
{
   if (n != 4) throw "wrong number of params";
   void (C::*fp)(A0, A1, A2) const = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3]);
   return Any();
}
template <class C, class A0, class A1, class A2>
struct getType<void (C::*)(A0,A1,A2) const>
{
   typedef void (C::*Type)(A0, A1, A2);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidConstClass3<C, A0, A1, A2>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2, class A3>
Any delegVoidConstClass4(Any* fp1, Any* params, int64_t n)
{
   if (n != 5) throw "wrong number of params";
   void (C::*fp)(A0, A1, A2, A3) const = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4]);
   return Any();
}
template <class C, class A0, class A1, class A2, class A3>
struct getType<void (C::*)(A0,A1,A2,A3) const>
{
   typedef void (C::*Type)(A0, A1, A2, A3);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidConstClass4<C, A0, A1, A2, A3>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->member = true;
      return fti;
   }
};

template <class C, class A0, class A1, class A2, class A3, class A4>
Any delegVoidConstClass5(Any* fp1, Any* params, int64_t n)
{
   if (n != 6) throw "wrong number of params";
   void (C::*fp)(A0, A1, A2, A3, A4) const = *fp1;
   (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4], params[5]);
   return Any();
}
template <class C, class A0, class A1, class A2, class A3, class A4>
struct getType<void (C::*)(A0,A1,A2,A3,A4) const>
{
   typedef void (C::*Type)(A0, A1, A2, A3, A4);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<void>();
      fti->delegPtr = delegVoidConstClass5<C, A0, A1, A2, A3, A4>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->members.add(new Member(getTypeAdd<A4>()));
      fti->member = true;
      return fti;
   }
};

//////////////////////////////////////////////////////////////////////////////// CONST MEMBER FUNCTIONS
template <class R, class C>
Any delegResConstClass0(Any* fp1, Any* params, int64_t n)
{
   if (n != 1) throw "wrong number of params";
   R (C::*fp)() const = *fp1;
   return (params[0].as<C>().*fp)();
}
template <class R, class C>
struct getType<R (C::*)() const>
{
   typedef R (C::*Type)();
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResConstClass0<R, C>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0>
Any delegResConstClass1(Any* fp1, Any* params, int64_t n)
{
   if (n != 2) throw "wrong number of params";
   R (C::*fp)(A0) const = *fp1;
   return (params[0].as<C>().*fp)(params[1]);
}
template <class R, class C, class A0>
struct getType<R (C::*)(A0) const>
{
   typedef R (C::*Type)(A0);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResConstClass1<R, C, A0>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1>
Any delegResConstClass2(Any* fp1, Any* params, int64_t n)
{
   if (n != 3) throw "wrong number of params";
   R (C::*fp)(A0, A1) const = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2]);
}
template <class R, class C, class A0, class A1>
struct getType<R (C::*)(A0,A1) const>
{
   typedef R (C::*Type)(A0, A1);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResConstClass2<R, C, A0, A1>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2>
Any delegResConstClass3(Any* fp1, Any* params, int64_t n)
{
   if (n != 4) throw "wrong number of params";
   R (C::*fp)(A0, A1, A2) const = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3]);
}
template <class R, class C, class A0, class A1, class A2>
struct getType<R (C::*)(A0,A1,A2) const>
{
   typedef R (C::*Type)(A0, A1, A2);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResConstClass3<R, C, A0, A1, A2>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2, class A3>
Any delegResConstClass4(Any* fp1, Any* params, int64_t n)
{
   if (n != 5) throw "wrong number of params";
   R (C::*fp)(A0, A1, A2, A3) const = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4]);
}
template <class R, class C, class A0, class A1, class A2, class A3>
struct getType<R (C::*)(A0,A1,A2,A3) const>
{
   typedef R (C::*Type)(A0, A1, A2, A3);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResConstClass4<R, C, A0, A1, A2, A3>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->member = true;
      return fti;
   }
};

template <class R, class C, class A0, class A1, class A2, class A3, class A4>
Any delegResConstClass5(Any* fp1, Any* params, int64_t n)
{
   if (n != 6) throw "wrong number of params";
   R (C::*fp)(A0, A1, A2, A3, A4) const = *fp1;
   return (params[0].as<C>().*fp)(params[1], params[2], params[3], params[4], params[5]);
}
template <class R, class C, class A0, class A1, class A2, class A3, class A4>
struct getType<R (C::*)(A0,A1,A2,A3,A4) const>
{
   typedef R (C::*Type)(A0, A1, A2, A3, A4);
   static TypeInfo* go()
   {
      TypeInfo* fti = new TypeInfo;
      fti->cpp      = true;
      fti->cType    = &typeid(Type);
      fti->name     = typeid(Type).name();
      fti->fullName = typeid(Type).name();
      fti->size     = sizeof(Type);
      fti->copyCon  = CopyCon<Type>::go;
      fti->destruct = destructnothing;
      fti->kind     = kFunction;
      fti->of       = getTypeAdd<R>();
      fti->delegPtr = delegResConstClass5<R, C, A0, A1, A2, A3, A4>;
      fti->members.add(new Member(getTypeAdd<const C>()));
      fti->members.add(new Member(getTypeAdd<A0>()));
      fti->members.add(new Member(getTypeAdd<A1>()));
      fti->members.add(new Member(getTypeAdd<A2>()));
      fti->members.add(new Member(getTypeAdd<A3>()));
      fti->members.add(new Member(getTypeAdd<A4>()));
      fti->member = true;
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
   T& operator[](const std::vector<int64_t>& p)
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
   std::vector<int64_t> getTableIndices(int64_t index)
   {
      std::vector<int64_t> res;
      for (size_t d = 0; d < dimensions; ++d)
      {
         res.push_back(index % sizes[d]);
         index /= sizes[d];
      }
      return res;
   }
   void clear()
   {
      delete[] mem;
      mem = nullptr;
   }
   T* begin()
   {
      return mem;
   }
   T* end()
   {
      return mem + totalSize;
   }
};

struct MMParamType
{
   TypeInfo* type;
   std::vector<std::pair<int64_t, TypeInfo*>> conversions;
};

typedef std::set<TypeInfo*> TypeSet2;

struct MMBase
{
   std::string      name;
   int64_t          minArgs = 0;
   int64_t          maxArgs = 0;
   int64_t          maxCost = 0;
   bool             useReturnT = false;
   bool             useTable   = false;
   bool             useConversions = false;
   bool             needsBuilding = false;
   std::vector<Any> methods;
   Array<Any>       table;
   Array<int64_t>   costs;
   std::vector<TypeSet2> paramTypeSets;
   std::vector<TypeSet2> paramTypeSets2;
   std::vector<TypeList> paramTypeLists;
   std::vector<std::vector<MMParamType>> paramConversions;
   std::vector<std::vector<MMParamType>> paramConversions2;
   int64_t          mmindex;
   static int64_t   mmcount;
   static std::set<MMBase*> mms;

   MMBase(std::string _name = "") : name(_name), mmindex(mmcount++)
   {
      mms.insert(this);
   }
   ~MMBase()
   {
      mms.erase(this);
   }
   void add(Any method);
   int64_t  compareMethods(Any& lhs, Any& rhs, TypeList &at);
   void buildTable();
   void findMostDerived(int64_t paramIndex, TypeInfo* type);
   void fillParamTypesInOrder(int64_t paramIndex, TypeInfo* type);
   void fillParamType(bool first, int64_t pi, int64_t ti, TypeInfo* t);
   void simulate(std::vector<Any*> &validMethods, std::vector<TypeInfo*> &at, int64_t ai);
   Any* getMethod(Any* args, int64_t nargs);
   Any* getMethodFromTable(Any* args, int64_t nargs);
};

template <class R>
struct Multimethod : public MMBase
{
   Multimethod(std::string _name = "") : MMBase(_name)
   {
   }
   R call(Any* args, int64_t nargs)
   {
      Any* addr = getMethod(args, nargs);
      if (useReturnT)
         return addr->call(args+1, nargs-1);
      else
         return addr->call(args, nargs);
   }
   R operator()(Any arg0)
   {
      return call(&arg0, 1);
   }
   R operator()(Any arg0, Any arg1)
   {
      Any args[2] = { arg0, arg1 };
      return call(args, 2);
   }      
   R operator()(Any arg0, Any arg1, Any arg2)
   {
      Any args[3] = { arg0, arg1, arg2 };
      return call(args, 3);
   }
};

template <>
struct Multimethod<void> : public MMBase
{
   Multimethod(std::string _name = "") : MMBase(_name)
   {
   }
   void call(Any* args, int64_t nargs)
   {
      Any* addr = getMethod(args, nargs);
      if (useReturnT)
         addr->call(args+1, nargs-1);
      else
         addr->call(args, nargs);
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

typedef Multimethod<Any> MultimethodAny;
#if 0
template <>
struct MultimethodVoid : public Multimethod<Any>
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

template <>
struct getType<MMBase>
{
   typedef MMBase Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->delegPtr = nullptr;
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct  = Destruct<Type>::go;
      typeInfo->multimethod = true;
      return typeInfo;
   }
};

template <class R>
Any delegMM(Any* mm, Any* args, int64_t n)
{
   return mm->as<Multimethod<R>>().call(args, n);
}

Any delegMMV(Any* mm, Any* args, int64_t n);

template <class R>
struct getType<Multimethod<R>>
{
   typedef Multimethod<R> Type;
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
      typeInfo->multimethod = true;
      typeInfo->of       = getTypeAdd<R>();
      typeInfo->delegPtr = delegMM<R>;
      addTypeLink(getTypeAdd<MMBase>(), typeInfo);
      return typeInfo;
   }
};

template <>
struct getType<Multimethod<void>>
{
   typedef Multimethod<void> Type;
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
      typeInfo->multimethod = true;
      typeInfo->of       = getTypeAdd<void>();
      typeInfo->delegPtr = delegMMV;
      addTypeLink(getTypeAdd<MMBase>(), typeInfo);
      return typeInfo;
   }
};

template <class R>
Any delegMMP(Any* mm, Any* args, int64_t n)
{
   return mm->as<Multimethod<R>*>()->call(args, n);
}

Any delegMMPV(Any* mm, Any* args, int64_t n);

template <class R>
struct getType<Multimethod<R>*>
{
   typedef Multimethod<R>* Type;
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
      typeInfo->multimethod = true;
      typeInfo->of       = getTypeAdd<Multimethod<R>>();
      typeInfo->delegPtr = delegMMP<R>;
      typeInfo->kind     = kPointer;
      return typeInfo;
   }
};

template <>
struct getType<Multimethod<void>*>
{
   typedef Multimethod<void>* Type;
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
      typeInfo->multimethod = true;
      typeInfo->of       = getTypeAdd<Multimethod<void>>();
      typeInfo->delegPtr = delegMMPV;
      typeInfo->kind     = kPointer;
      return typeInfo;
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
   std::vector<bool> fixed;
   Vec          args;
};

PartApply* partApply(MissingArg, Any a0);
PartApply* partApply(Any f, Any a0);
PartApply* partApply(MissingArg, MissingArg, Any a1);
PartApply* partApply(Any fn, MissingArg, Any a1);
PartApply* partApply(MissingArg, Any a0, Any a1);
PartApply* partApply(Any fn, Any a0, Any a1);

Any delegPartApply(Any* cl, Any* args, int64_t n);

template <typename F>
Any functor(F f)
{
   return partApply(&F::operator(), f);
}

template <>
struct getType<PartApply*>
{
   typedef PartApply* Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct = Destruct<Type>::go;
      typeInfo->of       = getTypeAdd<PartApply>();
      typeInfo->delegPtr = delegPartApply;
      typeInfo->kind     = kPointer;
      return typeInfo;
   }
};

struct PH
{
   int64_t n;
   PH(int64_t n) : n(n) {}
};

extern TypeInfo* tiPH;
extern TypeInfo* tiMissingArg;
extern TypeInfo* tiPartApply;
extern TypeInfo* tiBind;

struct Bind
{
   Vec params;
       Bind(Vec params) : params(params) {}
};

Any delegBind(Any* cl, Any* args, int64_t n);

Any bind(Any* args, int64_t nargs);

template <>
struct getType<Bind*>
{
   typedef Bind* Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct = Destruct<Type>::go;
      typeInfo->of       = getTypeAdd<Bind>();
      typeInfo->delegPtr = delegBind;
      typeInfo->kind     = kPointer;
      return typeInfo;
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
   typedef VarFunc Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct = Destruct<Type>::go;
      typeInfo->of       = getTypeAdd<Any>();
      typeInfo->delegPtr = delegVF;
      return typeInfo;
   }
};

struct Var
{
   std::string   name;
   Any           value;
   int64_t       offset;
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
      Var              rest;
      Any              body;
      Frame*           context;
      int64_t          minArgs;
      int64_t          maxArgs;

      Closure() : context(nullptr), minArgs(0), maxArgs(0) {}
      Closure(Any params, Any body, Frame* context = nullptr);
   };

   Any closureDeleg(Any* closure, Any* params, int64_t np);
}

namespace Struct
{
   struct Stack
   {
      Stack* parent;
      Any    frame;
   };

   struct Closure
   {
      Lambda* lambda;
      Stack*  context;

      Closure(Lambda* lambda, Stack* context) : lambda(lambda), context(context) {}
   };

   Any applyAnyClosure(Any* closure, Any* args, int64_t nargs);

   extern Stack* stack;
}

struct ExprIterator;

struct Expr
{
   virtual      ~Expr () {}
   virtual void accept(ExprIterator* iterator) {};
};

struct Lambda : public Expr
{
   TypeInfo* type;
   Expr*     body;
   Lambda(TypeInfo* type, Expr* body) : type(type), body(body) { }
   virtual void accept(ExprIterator* iterator);
};

template <>
struct getType<List::Closure*>
{
   typedef List::Closure* Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct = Destruct<Type>::go;
      typeInfo->of       = getTypeAdd<List::Closure>();
      typeInfo->delegPtr = List::closureDeleg;
      typeInfo->kind     = kPointer;
      return typeInfo;
   }
};

template <>
struct getType<Struct::Closure*>
{
   typedef Struct::Closure* Type;
   static TypeInfo* go()
   {
      TypeInfo* typeInfo = new TypeInfo;
      typeInfo->cpp      = true;
      typeInfo->cType    = &typeid(Type);
      typeInfo->name     = typeid(Type).name();
      typeInfo->fullName = typeid(Type).name();
      typeInfo->size     = sizeof(Type);
      typeInfo->copyCon  = CopyCon<Type>::go;
      typeInfo->destruct = Destruct<Type>::go;
      typeInfo->of       = getTypeAdd<Struct::Closure>();
      typeInfo->delegPtr = Struct::applyAnyClosure;
      typeInfo->kind     = kPointer;
      return typeInfo;
   }
};

extern Multimethod<Any> convert;

template <class To>
To Any::as2()
{
   TypeInfo* toType = getTypeAdd<To>();
   TypeInfo* toType1 = toType;
   int64_t toRL = 0;//to type ref levels: toRL can only be 0 or 1 and it makes no difference to the code required, except that adding more than one level of indirection will be bad
   int64_t toPL = 0;//to type pointer levels
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

   //an Any is basically just a pointer to T
   //can convert an Any containing T to a pointer to the T
   //can convert a pointer to Any containing T to a pointer to a pointer to the T

   //if you can convert the Any to a pointer to Any then
   //can convert an Any containing T to a pointer to a pointer to the T

   //can convert an (Any containing Any containing T) to a (pointer to a pointer to the T)

   //uses for pointers/references
   //cheaper than copies for some types
   //can change original remotely (but why?)
   //   to sneakily return results
   //   for returning from container getItem functions so you don't need a separate setItem function
   //   for passing to assignment functions
   //   for passing to anything that modifies the original and you want to keep the changes, such as passing this to member functions for containers

   //can make a linked list of types-to-contained-types by extracting values from pointers/refs/nested Anys
   //do we need that? or just to drill down?
   //if we're converting to pointer(s) to Any we don't know the target nested type so there's nothing to compare our linked list to
   //we need to drill down if we want to eventually get down to a type that is not a pointer(s)/ref(s) to Any
   //so there's two cases, pointer(s) to Any and pointer(s) to T???

   //if pointer(s) to Any
   if (toType == tiAny)
   {
   }
   else
   {
   }

   int64_t fromRL = 0;
   int64_t fromPL = 0;
   AnyBase from(*this);
   while (true)
   {
      if (from.canderp())
      {
         if (from.typeInfo->kind == kReference)
            ++fromRL;
         else
            ++fromPL;
         from = from.derp();
      }
      else if (from.candeany())
      {
         ++fromRL;
         from = from.deany();
      }
      else
         break;
   }
   TypeInfo* fromType = from.typeInfo;
   if (toPL + toRL == 0)
   {
      return convert(toType, Any(from.typeInfo, from.payload));//uh-oh, cannot return a reference to this
   }
   //include toRL in the calculation?
   //maybe not because conversion between references and values is automatic on the C++ side (but not on the interpreted side)
   //doing conversions when toPL is not equal to fromPL is probably not great but leave it in for now
   int64_t indirChange = toPL - fromRL - fromPL;

   //maybe put something in to disallow slicing
   //only allow conversions from derived to base 
   //(maybe restrict it to a single layer of indirection too unless const, 
   //but DEFINITELY if different offsets due to multiple inheritance)
   if (!hasLink(toType, fromType))
   {
      std::ostringstream o;
      o << "types unrelated: attempt to cast from " << fromType->getName() << " to " << toType1->getName();
      std::cout << o.str() << std::endl;
      throw std::runtime_error(o.str());
   }
   if (indirChange > 1)
   {
      std::ostringstream o;
      o << "cannot add more than 1 indirection: attempt to cast from " << fromType->getName() << " to " << toType1->getName();
      std::cout << o.str() << std::endl;
      throw std::runtime_error(o.str());
   }
   if (indirChange == 1)
   {
      return *static_cast<To*>((void*) & payload);
   }
   if (indirChange == 0)
   {
      return *static_cast<To*>(payload);
   }
   from = *this;
   while (indirChange < 0)
   {
      if (from.canderp())
      {
         ++indirChange;
         from = from.derp();
      }
      else if (from.candeany())
      {
         ++indirChange;
         from = from.deany();
      }
      else
         throw "We've lost some layers of indirection since we checked earlier in the function!";
   }
   return *static_cast<To*>(from.payload);
}


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              