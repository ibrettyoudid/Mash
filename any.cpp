// Copyright (c) 2023 Brett Curtis
#include "stdafx.h"
#include "any.h"
#include "classes.h"
#include <deque>

using namespace std;

TypeMap typeMap;
TypeMap typeMapR;
TypeSet typeSet;
TypeSet typeSetR;

int64_t MMBase::mmcount = 0;
set<MMBase*> MMBase::mms;

void error(std::string message)
{
   cout << message << endl;
   throw std::runtime_error(message);
}

void copyConInterp(void* dest, void* src, TypeInfo* typeInfo)
{
   for (auto m : typeInfo->members.byOffset)
      if (!m->one)
         m->typeInfo->copyCon((char*)dest + m->offset, (char*)src + m->offset, m->typeInfo);
}

void destructInterp(void *ptr, TypeInfo* typeInfo)
{
   for (auto m : typeInfo->members.byOffset)
      if (!m->one)
         m->typeInfo->destruct((char*)ptr + m->offset, m->typeInfo);
}

TypeInfo* typeInfoInterp(std::string name)
{
   TypeInfo* typeInfo = new TypeInfo;
   typeInfo->cpp = false;
   typeInfo->cType = nullptr;
   typeInfo->name = name;
   typeInfo->fullName = name;
   typeInfo->size = 0;
   typeInfo->delegPtr = nullptr;
   typeInfo->copyCon = copyConInterp;
   typeInfo->destruct = destructInterp;
   typeSet.insert(typeInfo);
   return typeInfo;
}

void addTypeLink(TypeInfo* base, TypeInfo* derived, LinkKind kind, int64_t offset)
{
   TypeLink* tl = new TypeLink(base, derived, kind, offset);
   base->main.derived.push_back(tl);
   derived->main.bases.push_back(tl);
}

void addTypeLinkConv(TypeInfo* base, TypeInfo* derived)
{
   TypeLink* tl = new TypeLink(base, derived, lConv, 0);
   base->conv.derived.push_back(tl);
   derived->conv.bases.push_back(tl);
}

Any delegVoid0(Any* fp1, Any* params, int64_t n)
{
   void (*fp)() = *fp1;
   fp();
   return Any(0);
}

Any delegMMPV(Any* mm, Any* args, int64_t n)
{
   mm->as<Multimethod<void>*>()->call(args, n);
   return 0;
}

Any delegMMV(Any* mm, Any* args, int64_t n)
{
   mm->as<Multimethod<void>>().call(args, n);
   return 0;
}

TypeLink* joinLinks(TypeLink* btod, TypeLink* dtodd)
{
   if (btod->kind == lSub && dtodd->kind == lSub)
   {
      return new TypeLink(btod->base, dtodd->derived, lSub, btod->offset + dtodd->offset);
   }
   throw "can't join";
}

struct NoGoodHeads{};

bool LessMemberSymbol::operator()(const Member* lhs, const Member* rhs) const
{
   return lhs->symbol < rhs->symbol;
}

bool LessMemberSymbol::operator()(const Member* lhs, Symbol* rhs) const
{
   return lhs->symbol < rhs;
}

bool LessMemberSymbol::operator()(Symbol* lhs, const Member* rhs) const
{
   return lhs < rhs->symbol;
}

bool LessMemberName::operator()(const Member* lhs, const Member* rhs) const
{
   if (lhs->symbol)
      if (rhs->symbol)
         return lhs->symbol->name < rhs->symbol->name;
      else
         return false;
   else
      return rhs->symbol;
}

bool LessMemberName::operator()(const Member* lhs, const std::string& rhs) const
{
   if (lhs->symbol)
      return lhs->symbol->name < rhs;
   else
      return true;
}

bool LessMemberName::operator()(const std::string& lhs, const Member* rhs) const
{
   if (rhs->symbol)
      return lhs < rhs->symbol->name;
   else
      return false;
}

Member* MemberList::add(Member* m)
{
   auto i = byPtr.find(m);
   if (i == byPtr.end())
   {
      bySymbol.insert(m);
      byName.insert(m);
      byPtr.insert(m);
      byOffset.push_back(m);
      return m;
   }
   throw "member already exists!";
   return *i;
}
Member* MemberList::operator[](Member* m)
{
   auto i = byPtr.find(m);
   if (i != byPtr.end())
   {
      return *i;
   }
   return nullptr;
   //throw "member not found!";
}
Member* MemberList::operator[](Symbol* symbol)
{
   auto i = bySymbol.find(symbol);
   if (i != bySymbol.end())
   {
      return *i;
   }
   return nullptr;
   //throw "member not found!";
}
Member* MemberList::operator[](std::string name)
{
   auto i = byName.find(name);
   if (i != byName.end())
   {
      return *i;
   }
   return nullptr;
   //throw "member not found!";
}

Member* MemberList::operator[](int64_t n)
{
   return byOffset[n];
}

int64_t MemberList::size()
{
   return byPtr.size();
}

void MemberList::clear()
{
   byPtr.clear();
   bySymbol.clear();
   byName.clear();
   byOffset.clear();
}

Any::Any() : typeInfo(nullptr), payload(nullptr)
{
   //construct(0);
}

Any::Any(const Any& rhs)
{
   if (rhs.payload)
   {
      typeInfo = rhs.typeInfo;
      payload = new char[typeInfo->size];
      typeInfo->copyCon(payload, rhs.payload, typeInfo);
   }
   else
   {
      typeInfo = nullptr;
      payload  = nullptr;
   }
}

Any::Any(Any&& rhs) : typeInfo(rhs.typeInfo), payload(rhs.payload)
{
   rhs.payload = nullptr;
}  

Any& Any::operator=(const Any& rhs)
{
   if (payload)
   {
      typeInfo->destruct(payload, typeInfo);
      delete[] static_cast<char*>(payload);
   }
   typeInfo = rhs.typeInfo;
   payload = new char[typeInfo->size];
   typeInfo->copyCon(payload, rhs.payload, typeInfo);
   return *this;
}
Any& Any::operator=(Any&& rhs)
{
   if (payload)
   {
      typeInfo->destruct(payload, typeInfo);
      delete[] static_cast<char*>(payload);
   }
   typeInfo = rhs.typeInfo;
   payload = rhs.payload;
   rhs.payload = nullptr;
   return *this;
}

Any::Any(TypeInfo* typeInfo, void* rhs) : typeInfo(typeInfo)
{
   payload = new char[typeInfo->size];//creates a copy on construction
   typeInfo->copyCon(payload, rhs, typeInfo);
}
Any::~Any()
{
   if (payload)
   { 
      typeInfo->destruct(payload, typeInfo);
      delete[] static_cast<char*>(payload);
   }
}
bool Any::empty()
{
   return !payload;
}
Any Any::call(Any* args, int64_t n)
{
   return typeInfo->delegPtr(this, args, n);
}
Any Any::operator()()
{
   return typeInfo->delegPtr(this, nullptr, 0);
}
Any Any::operator()(Any arg0)
{
   return typeInfo->delegPtr(this, &arg0, 1);
}
Any Any::operator()(Any arg0, Any arg1)
{
   Any args[2] = { arg0, arg1 };
   return typeInfo->delegPtr(this, args, 2);
}
Any Any::operator()(Any arg0, Any arg1, Any arg2)
{
   Any args[3] = { arg0, arg1, arg2 };
   return typeInfo->delegPtr(this, args, 3);
}
Any Any::operator()(Any arg0, Any arg1, Any arg2, Any arg3)
{
   Any args[4] = { arg0, arg1, arg2, arg3 };
   return typeInfo->delegPtr(this, args, 4);
}
Any Any::operator()(Any arg0, Any arg1, Any arg2, Any arg3, Any arg4)
{
   Any args[5] = { arg0, arg1, arg2, arg3, arg4 };
   return typeInfo->delegPtr(this, args, 5);
}
//--------------------------------------------------------------------------- CONVERSION
Any Any::derp()
{
   return Any(typeInfo->of, *(void**)payload);
}
Any Any::deany()//denest a nested any
{
   if (typeInfo == tiAny)
      return Any(((Any*)payload)->typeInfo, ((Any*)payload)->payload);
   else
      throw "not an Any!";
}
Any Any::ptrTo()
{
   return Any(typeInfo->ptr, &payload);
}
Any Any::refTo()
{
   return Any(typeInfo->ref, &payload);
}
Any::operator MMBase& ()
{
   if (typeInfo->kind == kPointer && typeInfo->of->multimethod) return **static_cast<MMBase**>(payload);
   if (typeInfo->multimethod) return *static_cast<MMBase*>(payload);
   std::cout << TS + "type mismatch: casting " + typeInfo->getName() + " to MMBase&" << std::endl;
   throw "type mismatch";
}
//---------------------------------------------------------------------------
std::string Any::typeName()
{
   return typeInfo->getName();
   if (typeInfo->name.substr(0, 7) == "struct ")
      return typeInfo->name.substr(7);
   else if (typeInfo->name.substr(0, 6) == "class ")
      return typeInfo->name.substr(6);
   else
      return typeInfo->name;
}
TypeInfo* Any::paramType(int64_t n, bool useReturnT)
{
   return typeInfo->argType(n, useReturnT);
}
bool Any::isPtrTo(TypeInfo* other)
{
   return typeInfo->isPtrTo(other);
}
bool Any::isRefTo(TypeInfo* other)
{
   return typeInfo->isRefTo(other);
}
bool Any::isPRTF(TypeInfo* other)
{
   return typeInfo->isPRTF(other);
}
bool Any::callable()
{
   return payload && typeInfo->delegPtr;
   //return typeInfo == tiListClosure || typeInfo == tiStructClosure || typeInfo == tiVarFunc || typeInfo->multimethod || typeInfo->kind == kFunction;
}

int64_t Any::maxArgs()
{
   if (typeInfo == tiListClosure)
      return as<List::Closure*>()->maxArgs;
   else if (typeInfo == tiVarFunc)
      return as<VarFunc>().maxArgs;
   else if (typeInfo->multimethod)
      return ((MMBase&)*this).minArgs;
   else if (typeInfo->kind == kFunction)
      return typeInfo->members.size();
   else
   {
      cout << "maxArgs called for non-function" << endl;
      cout << typeInfo->fullName << endl;
      throw "maxArgs called for non-function";
   }
}

int64_t Any::minArgs()
{
   if (typeInfo == tiListClosure)
      return as<List::Closure*>()->minArgs;
   else if (typeInfo == tiVarFunc)
      return as<VarFunc>().minArgs;
   else if (typeInfo->multimethod)
      return ((MMBase&)*this).minArgs;
   else if (typeInfo->kind == kFunction)
      return typeInfo->members.size();
   else
   {
      cout << "minArgs called for non-function" << endl;
      cout << typeInfo->fullName << endl;
      cout.flush();
      throw "minArgs called for non-function";
   }
}

string hex(unsigned char* b, unsigned char* e)
{
   ostringstream o;
   int64_t step = 8;
   for (unsigned char* c = b; c < e; c += step)
   {
      if (c > b) o << endl;
      o << (int64_t*)c << " ";
      for (unsigned char* c1 = c; c1 < c + step && c1 < e; ++c1)
         o << "0123456789ABCDEF"[(*c1 >> 4) & 0xF] << "0123456789ABCDEF"[*c1 & 0xF] << " ";
      for (unsigned char* c1 = c; c1 < c + step && c1 < e; ++c1)
         if (*c1 >= 32) o << *c1; else o << ".";
   }
   return o.str();
}

void showhex(unsigned char* b, unsigned char* e)
{
   cout << hex(b, e);
}

string Any::hex()
{
   unsigned char* b = (unsigned char*)payload;
   unsigned char* e = b + typeInfo->size;
   return ::hex(b, e);
}

void Any::showhex()
{
   cout << hex();
}

int64_t Any::pDepth()
{
   TypeInfo* ti = typeInfo;
   int64_t count = 0;
   while (true)
   {
      if (ti->kind == kPointer || ti->kind == kReference)
      {
         if (ti->kind == kPointer) ++count;
         ti = ti->of;
      }
      else break;
   }
   return count;
}

int64_t Any::rDepth()
{
   TypeInfo* ti = typeInfo;
   int64_t count = 0;
   while (true)
   {
      if (ti->kind == kPointer || ti->kind == kReference)
      {
         if (ti->kind == kReference) ++count;
         ti = ti->of;
      }
      else break;
   }
   return count;
}

int64_t Any::prDepth()
{
   TypeInfo* ti = typeInfo;
   int64_t count = 0;
   while (true)
   {
      if (ti->kind == kPointer || ti->kind == kReference)
      {
         ++count;
         ti = ti->of;
      }
      else break;
   }
   return count;
}

void Any::add(Member* m, Any value)
{
   if (m->one)
   {
      //m->typeInfo = m->value.typeInfo;
      typeInfo->add(m);
   }
   else
   {
      TypeInfo* typeInfoNew = new TypeInfo(*typeInfo);
      typeInfoNew->add(m);
      void* ptrNew = new char[typeInfoNew->size];
      typeInfo->copyCon(ptrNew, payload, typeInfo);
      delete[] payload;
      payload = ptrNew;
      typeInfo = typeInfoNew;
      m->setMember(m, *this, value);
   }
}

Any Any::getMember(Member* m)
{
   if (m->one)
      return m->value;
   else
      return (*m->getMember)(m, *this);
}

Any Any::getMemberRef(Member* m)
{
#if 0
   TypeInfo* tiNew = new TypeInfo;
   tiNew->kind = kReference;
   tiNew->of = typeInfo;
   tiNew->copyCon = CopyCon<void*>::go;
   tiNew->destruct= destructnothing;
   tiNew->delegPtr = nullptr;
   tiNew->size = sizeof(void*);
   //TypeInfo* typeIndex = *typeSetR.find(typeInfo->cType);
   char* ptrNew = reinterpret_cast<char*>(ptr) + m->offset;
   return Any(tiNew, reinterpret_cast<void*>(&ptrNew));
#else
   if (m->one)
      return m->value;
   else if (typeInfo->cpp)
      return (*m->getMemberRef)(m, *this);
   else
   {
      void *addr = getMemberAddress(m);
      return Any(m->typeInfo->ref, &addr);
   }
#endif
}

Any Any::getMemberPtr(Member* m)
{
   if (m->one)
      return m->value;
   else
      return (*m->getMember)(m, *this);
}

void Any::setMember(Member* m, Any value)
{
   if (m->one)
      m->value = value;
   else
      (*m->setMember)(m, *this, value);
}

Any Any::getMemberRef(Symbol* symbol)
{
   return getMemberRef(findMember(symbol));
   //return (*m->getMemberRef)(m, *this);
}

Any Any::getMemberRef(std::string name)
{
   return getMemberRef(findMember(name));
   //return (*m->getMemberRef)(m, *this);
}

Any Any::getMemberRef(int64_t n)
{
   return getMemberRef(findMember(n));
   //return (*m->getMemberRef)(m, *this);
}

Member* Any::findMember(Symbol* symbol)
{
   return (*typeInfo)[symbol];
}

Member* Any::findMember(std::string name)
{
   return (*typeInfo)[name];
}

Member* Any::findMember(int64_t n)
{
   return (*typeInfo)[n];
}

void* Any::getMemberAddress(Member* m)
{
   if (m->one)
      return m->value.payload;
   else
      return (char*)payload + m->offset;
}

void TypeInfo::doC3Lin(TypeConn TypeInfo::* conn)
{
   typedef deque<TypeList> TypeLists;
   TypeLists baseLins;
   TypeList  result;
   result.push_back(this);

   if (!(this->*conn).bases.empty())
   {
      int64_t blCount = (this->*conn).bases.size();
      for (auto baseLink : (this->*conn).bases)
      {
         if ((baseLink->base->*conn).linear.empty()) baseLink->base->doC3Lin(conn);
         baseLins.push_back((baseLink->base->*conn).linear);
      }
      while (blCount)
      {
         //loop through all the base linearisations, looking at the heads to see if any are good
         for (auto &baseLin : baseLins)
         {
            if (baseLin.empty()) continue;
            TypeInfo* head = baseLin[0];

            //loop through the tails of the base linearisations, making sure this head does not occur, if it does it is BAD
            for (auto &tailCheck : baseLins)
            {
               for (size_t i = 1; i < tailCheck.size(); ++i)
               {
                  if (head == tailCheck[i]) goto bad;
               }
            }
            result.push_back(head);

            //now remove this good head from ALL the base linearisations
            for (auto &decapitate : baseLins)
            {
               if (decapitate[0] == head)//only need to check the first since we know it's not in the tail
               {
                  decapitate.erase(decapitate.begin());
                  if (decapitate.empty()) --blCount;
               }
            }
            goto good;
bad:;
            //try next findGood
         }
         //if we fall through then there are no good heads
         throw NoGoodHeads();
good:;
         //success: keep going while baseLins is not empty
      }
   }
   (this->*conn).linear = result;
}

void TypeInfo::allocate()
{
   if (kind == kReference || kind == kPointer)
   {
      size = sizeof(nullptr);
      return;
   }
   
   if (!cpp)
   {
      size = 0;
      for (auto p : main.bases)
      { 
         p->base->allocate();
         p->offset = size;
         size += p->base->size;
      }

      for (auto m : members.byOffset)
      {
         if (!m->one)
         {
            m->offset = size;
            m->typeInfo->allocate();
            size += m->typeInfo->size;
         }
      }
   }
   layout.clear();
   TypeLinkList tll;
   allocate2(this, 0, tll);
}

void TypeInfo::allocate2(TypeInfo* derived, int64_t baseOffset, TypeLinkList& links)
{
   for (auto p : main.bases)
   {
      TypeLinkList linksNew(links);
      linksNew.push_back(p);
      p->base->allocate2(derived, baseOffset + p->offset, linksNew);

      TypePath* tp;
      if (cpp)
         tp = new TypePath(p->base, derived, p->kind, baseOffset + p->offset, baseOffset, linksNew);
      else
         tp = new TypePath(p->base, derived, p->kind, baseOffset + p->offset, baseOffset + p->offset, linksNew);
      derived->layout.insert(tp);
   }

   for (auto m : members.byOffset)
   {
#if 0
      Member* newM = new Member(m->symbol, m->typeInfo, m->one ? 0 : (baseOffset + m->offset), m->ptrToMember);
      newM->links = links;
      newM->getMember = m->getMember;
      newM->getMemberRef = m->getMemberRef;
      newM->setMember = m->setMember;
      newM->ptrToMember = m->ptrToMember;
      derived->allMembers.add(newM);
#else
      if (m->one)
         derived->allMembers.add(m);
      else
      {
         Member* newM = new Member(*m);
         newM->offset = baseOffset + m->offset;
         newM->links  = links;
         derived->allMembers.add(newM);
      }
#endif
   }
}

TypeInfo* findLink(TypeInfo* base, TypeInfo* derived)
{
   for (TypeList::iterator l = derived->main.linear.begin(); l != derived->main.linear.end(); ++l)
   {
      if (*l == base) return *l;
   }
   return nullptr;
}

bool hasLink(TypeInfo* base, TypeInfo* derived)
{
   if (base == derived) return true;
   for (TypeList::iterator l = derived->main.linear.begin(); l != derived->main.linear.end(); ++l)
   {
      if (*l == base) return true;
   }
   return false;
}

bool hasLinkBi(TypeInfo* a, TypeInfo* b)
{
   return hasLink(a, b) || hasLink(b, a);
}

int64_t indexLink(TypeInfo* base, TypeInfo* derived)
{
   for (size_t i = 0; i < derived->main.linear.size(); ++i)
   {
      if (derived->main.linear[i] == base) return i;
   }
   return -1;
}

bool canCast(TypeInfo* toType, TypeInfo* fromType)
{
   TypeInfo* toType1 = toType;
   TypeInfo* fromType1 = fromType;
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
   if (!hasLink(toType, fromType) && toType != tiAny)//only allow conversions from derived to base
   {
      //std::ostringstream o;
      //o << "types unrelated: attempt to cast from " << fromType1->getName() << " to " << toType1->getName();
      //std::cout << o.str() << std::endl;
      return false;
   }
   int64_t indirs = fromRL + fromPL - toPL;
   if (indirs < -1)
   {
      //std::ostringstream o;
      //o << "cannot add more than 1 indirection: attempt to cast from " << fromType1->getName() << " to " << toType1->getName();
      //std::cout << o.str() << std::endl;
      return false;
   }
   if (indirs > 3)
   {
      //std::ostringstream o;
      //o << "cannot remove more than 3 indirections: attempt to cast from " << fromType1->getName() << " to " << toType1->getName();
      //std::cout << o.str() << std::endl;
      return false;
   }
   return true;
}
/*
void addMember(Any member, string name)
{
   TypeInfo* typeIndex = member.typeInfo;
   if (!typeIndex->member)
      cout << "not a data member" << endl;
}
*/

TypeInfo* TypeInfo::argType(int64_t n, bool useReturnT)
{
   n -= useReturnT;
   if (n < 0 || n >= members.size())
      return of;
   else
      return members[n]->typeInfo;
}
TypeInfo* TypeInfo::unref()
{
   if (kind == kReference) return of->unref(); else return this;
}
TypeInfo* TypeInfo::unptr(int64_t n)
{
   if      (kind == kReference       ) return of->unptr(n  );
   else if (kind == kPointer && n > 0) return of->unptr(n-1);
   else return this;
}
TypeInfo* TypeInfo::refTo()
{
   if (this == nullptr) return nullptr;
   if (kind == kReference) return of; else return nullptr;
}
bool TypeInfo::isPtrTo(TypeInfo* other)
{
   if (this == other) return true;
   TypeInfo* unrefThis  = this ->unref();
   TypeInfo* unrefOther = other->unref();
   if (unrefThis == unrefOther) return true;
   if (unrefThis->kind == kPointer) unrefThis = unrefThis->of->unref();
   //return unrefThis == unrefOther;
   return hasLinkBi(unrefThis, unrefOther);
}
bool TypeInfo::isRefTo(TypeInfo* other)
{
   if (this == other) return true;
   TypeInfo* unrefThis  = this ->unref();
   TypeInfo* unrefOther = other->unref();
   //return unrefThis == unrefOther;
   return hasLinkBi(unrefThis, unrefOther);
}
//lhs is rhs, or pointer to rhs, or reference to rhs, or rhs is pointer to lhs or reference to lhs
//is pointer/ref to/from
bool TypeInfo::isPRTF(TypeInfo* other)
{
   if (this == other) return true;
   TypeInfo* unrefThis  = this ->unref();
   TypeInfo* unrefOther = other->unref();
   if (unrefThis == unrefOther) return true;
   if (unrefThis->kind == kPointer) unrefThis = unrefThis->of->unref();
   if (unrefOther->kind == kPointer) unrefOther = unrefOther->of->unref();
   //return unrefThis == unrefOther;
   return hasLinkBi(unrefThis, unrefOther);
}

bool TypeInfo::isRTF(TypeInfo* other)
{
   //return unref() == other->unref();
   return hasLinkBi(unref(), other->unref());
}

std::string TypeInfo::getCName(std::string var)
{
   ostringstream o;
   switch (kind)
   { 
      case kNormal:
         o << this->name;
         if (var.size()) o << " " << var;
         return o.str();
      case kReference:
         o << "&" << var;
         return of->getCName(o.str());
      case kPointer:
         o << "*" << var;
         return of->getCName(o.str());
      case kPtrMem:
         o << members.byOffset[0]->typeInfo->name << "::* " << var;
         return of->getCName(o.str());
      case kArray:
         o << var << "[" << count << "]";
         return of->getCName(o.str());
      case kFunction:
         o << "(*" << var << ")(";
         for (size_t i = 0; i < members.byOffset.size(); ++i)
         {
            auto a = members.byOffset[i];
            if (i > 0) o << ", ";
            if (a->symbol)
               o << a->typeInfo->getCName(a->symbol->name);
            else
               o << a->typeInfo->getCName();
         }
         o << ")";
         return of->getCName(o.str());
   }
}

std::string TypeInfo::getName()
{
   ostringstream o;
   if (this == nullptr)
   {
      o << "NULLPTR!!";
      return o.str();
   }
   switch (kind)
   {
   case kNormal:
      o << this->name;
      break;
   case kReference:
      o << "& " << of->getName();
      break;
   case kPointer:
      o << "* " << of->getName();
      break;
   case kPtrMem:
      o << members.byOffset[0]->typeInfo->name << "::* " << of->getName();
      break;
   case kArray:
      o << "[" << count << "] " << of->getName();
      break;
   case kFunction:
      o << "*function(";
      for (size_t i = 0; i < members.byOffset.size(); ++i)
      {
         auto a = members.byOffset[i];
         if (i > 0) o << ", ";
         o << a->typeInfo->getName();
         if (a->symbol)
            o << " " << a->symbol->name;
      }
      o << ") -> " << of->getName();
      break;
   }
   return o.str();
}
TypeInfo* TypeInfo::setName(string s)
{
   name = s;
   return this;
}
Member* TypeInfo::add(Member* m)
{
   if (!m->one)
   {
      m->offset = size;
      size += m->typeInfo->size;
   }
   return members.add(m);
}
Member* TypeInfo::operator[](Symbol* s)
{
   return allMembers[s];
}
Member* TypeInfo::operator[](std::string name)
{
   return allMembers[name];
}
Member* TypeInfo::operator[](int64_t n)
{
   return allMembers[n];
}

MissingArg missingArg;
MissingArg _;

Any delegPartApply(Any* cl, Any* argsVar, int64_t n)
{
   PartApply* pa = *cl;
   size_t nArgsAll = pa->argsFixed.size() + n;
   Any* argsAll = new Any[nArgsAll];
   size_t afi = 0;
   size_t avi = 0;
   for (size_t aai = 0; aai < nArgsAll; ++aai)
   {
      if (pa->pArgsFixed >> aai & 1LL)
         argsAll[aai] = pa->argsFixed[afi++];
      else
         argsAll[aai] = argsVar[avi++];
   }
   //Any res = clos->call(argsAll[0], &argsAll[1], nArgsAll - 1);
   Any res = argsAll[0].call(argsAll + 1, nArgsAll - 1);
   delete[] argsAll;
   return res;
}
//pArgsFixed is binary defining which args are fixed
//bit 0 is the function itself and bits 1..n are arguments 1..n
PartApply* partApply(MissingArg, Any a0)
{
   PartApply* res = new PartApply;
   res->pArgsFixed = 2;
   res->argsFixed.push_back(a0);
   return res;
}

PartApply* partApply(Any fn, Any a0)
{
   PartApply* res = new PartApply;
   res->pArgsFixed = 3;//binary - means members 0 and 1 are fixed
   res->argsFixed.push_back(fn);
   res->argsFixed.push_back(a0);
   return res;
}

PartApply* partApply(MissingArg, MissingArg, Any a1)
{
   PartApply* res = new PartApply;
   res->pArgsFixed = 4;
   res->argsFixed.push_back(a1);
   return res;
}

PartApply* partApply(Any fn, MissingArg, Any a1)
{
   PartApply* res = new PartApply;
   res->pArgsFixed = 5;
   res->argsFixed.push_back(fn);
   res->argsFixed.push_back(a1);
   return res;
}

PartApply* partApply(MissingArg, Any a0, Any a1)
{
   PartApply* res = new PartApply;
   res->pArgsFixed = 6;//binary - means members 0, 1 & 2 are fixed
   res->argsFixed.push_back(a0);
   res->argsFixed.push_back(a1);
   return res;
}

PartApply* partApply(Any fn, Any a0, Any a1)
{
   PartApply* res = new PartApply;
   res->pArgsFixed = 7;//binary - means members 0, 1 & 2 are fixed
   res->argsFixed.push_back(fn);
   res->argsFixed.push_back(a0);
   res->argsFixed.push_back(a1);
   return res;
}

Any delegBind(Any* cl, Any* args, int64_t nargs)
{
   Bind* bi = *cl;
   Vec argsOut;
   for (auto& param : bi->params)
   {
      if (param.typeInfo == tiPH)
      {
         int64_t n = param.as<PH>().n;
         if (n >= nargs)
            throw "not enough arguments passed";
         argsOut.push_back(args[n]);
      }
      else
         argsOut.push_back(param);
   }
   return argsOut[0].call(&argsOut[1], bi->params.size() - 1);
}

Any bind(Any* args, int64_t nargs)
{
   return new Bind(Vec(args, args + nargs));
}

extern TypeInfo* tiListClosure;
extern TypeInfo* tiStructClosure;
extern TypeInfo* tiVarFunc;

template <typename F>
Any functor(F f)
{
   return partApply(&F::operator(), f);
}

Any delegVF(Any* fp, Any* args, int64_t n)
{
   return fp->as<VarFunc>().func(args, n);
}

void addMemberInterp(TypeInfo* _class, std::string name, TypeInfo* type)
{
   _class->members.add(new Member(getSymbol(name), type, _class->size));
}

void MMBase::add(Any method)
{
   TypeInfo* ti = method.typeInfo;
   int64_t methodMaxArgs = method.maxArgs() + (useReturnT ? 1 : 0);
   int64_t methodMinArgs = method.minArgs() + (useReturnT ? 1 : 0);
   if (methods.size() == 0 || methodMaxArgs > maxArgs) maxArgs = methodMaxArgs;
   if (methods.size() == 0 || methodMinArgs < minArgs) minArgs = methodMinArgs;
   methods.push_back(method);
   needsBuilding = true;
}
int64_t MMBase::compareMethods(Any& lhs, Any& rhs, TypeList &at)
{
   if (useReturnT)
   {
      throw "not implemented yet";
   }
   else
   {
      bool lhsBetter = false;
      bool rhsBetter = false;
      for (size_t ai = 0; ai < maxArgs; ++ai)
      {
         auto atlb = at[ai]->conv.linear.begin();//atl = argument type linearisation
         auto atle = at[ai]->conv.linear.end();

         auto lhsAtl = find(atlb, atle, lhs.paramType(ai, useReturnT));
         auto rhsAtl = find(atlb, atle, rhs.paramType(ai, useReturnT));
         if (lhsAtl < rhsAtl)
            lhsBetter = true;
         else if (rhsAtl < lhsAtl)
            rhsBetter = true;
      }
      if (lhsBetter)
         if (rhsBetter)
            return 0;//each has arguments/parameters it is better on
         else
            return -1;//lhs is better, the best ones get sorted to the front
      else
         if (rhsBetter)
            return 1;//rhs is better
         else
            return 1;
            //throw "methods have the same parameters";
   }
}
void MMBase::findMostDerived(int64_t paramIndex, TypeInfo* type)
{
   if (type->conv.derived.size())
   {
      for (auto derLink : type->conv.derived)
         findMostDerived(paramIndex, derLink->derived);
   }
   else
      fillParamTypesInOrder(paramIndex, type);
}
void MMBase::fillParamTypesInOrder(int64_t paramIndex, TypeInfo* type)
{
   if (paramTypeSets2[paramIndex].contains(type))
      return;
   for (auto baseLink : type->conv.bases)
      fillParamTypesInOrder(paramIndex, baseLink->base);
   if (paramTypeSets[paramIndex].contains(type))
   {
      paramTypeSets2[paramIndex].insert(type);
      paramTypeLists[paramIndex].push_back(type);
   }
}
void MMBase::buildTable()
{
   table.clear();
   paramTypeSets.clear();
   paramTypeSets.resize(maxArgs);
   for (auto &m : methods)
      for (size_t pi = 0; pi < maxArgs; ++pi)
         paramTypeSets[pi].insert(m.paramType(pi, useReturnT));

   paramTypeSets2.clear();
   paramTypeLists.clear();
   paramTypeSets2.resize(maxArgs);
   paramTypeLists.resize(maxArgs);
   for (size_t paramIndex = 0; paramIndex < maxArgs; ++paramIndex)
   {
      for (auto type : paramTypeSets[paramIndex])
         findMostDerived(paramIndex, type);
   }

   for (size_t pi = 0; pi < maxArgs; ++pi)
   {
      auto &param = paramTypeLists[pi];
      for (size_t ti = 0; ti < param.size(); ++ti)
         fillParamType(true, pi, ti, param[ti]);
   }

   table.sizes.resize(maxArgs);
   for (size_t pi = 0; pi < maxArgs; ++ pi)
      table.sizes[pi] = paramTypeLists[pi].size();
   table.alloc();
   std::vector<TypeInfo*> at(maxArgs);
#if 1
   std::vector<int64_t> ati(maxArgs);
   for (auto &m : methods)
   {
      for (size_t pi = 0; pi < maxArgs; ++pi)
         ati[pi] = m.paramType(pi, useReturnT)->mmpindices[mmindex][pi];
      table[ati] = m;
   }
   if (useConversions)
   {
      bool ambiguities = false;
      bool blankCells  = false;
      for (int64_t tableIndex = 0; tableIndex < table.totalSize; ++tableIndex)
      {
         int64_t tableIndex1 = tableIndex;
         for (size_t pi = 0; pi < maxArgs; ++pi)
         {
            ati[pi] = tableIndex1 % table.sizes[pi];
            at[pi] = paramTypeLists[pi][ati[pi]];
            tableIndex1 /= table.sizes[pi];
         }
         if (!table[ati].callable())
         {
            vector<TypeInfo*> at2(at);
            std::vector<int64_t> ati2(ati);
            Vec bestMethods;
            for (size_t pi = 0; pi < maxArgs; ++pi)
            {
               for (auto &cb : at[pi]->conv.bases)
               {
                  if (paramTypeSets[pi].contains(cb->base))
                  {
                     at2[pi] = cb->base;
                     ati2[pi] = cb->base->mmpindices[mmindex][pi];
                     Any prevCell = table[ati2];
                     Vec newMethods;
                     if (!prevCell.empty())
                     {
                        if (prevCell.typeInfo->kind == kFunction)
                           newMethods.push_back(prevCell);
                        else if (prevCell.typeInfo == tiVec)
                           newMethods = Vec(prevCell);
                     }
                     if (newMethods.size() > 0) 
                     {
                        for (auto &newMethod : newMethods)
                        {
                           bool add = true;
                           for (int64_t bmi = bestMethods.size() - 1; bmi >= 0; --bmi)
                           {
                              int64_t c = compareMethods(newMethod, bestMethods[bmi], at);
                              if (c < 0)
                                 bestMethods.erase(bestMethods.begin() + bmi);
                              else if (c > 0)
                                 add = false;
                           }
                           if (add)
                              bestMethods.push_back(newMethod);
                        }
                     }
                  }
               }
               at2 [pi] = at [pi];
               ati2[pi] = ati[pi];
            }
            if (bestMethods.size() == 1)
            {
               table[ati] = bestMethods[0];
            }
            else if (bestMethods.size() > 1)
            {
               table[ati] = bestMethods;
               ambiguities = true;
            }
            else
            {
               blankCells = true;
            }
         }
      }
      if (blankCells || ambiguities)
      {
         if (blankCells ) cout << "warning: multimethod " << name << " has blank cells" << endl;
         if (ambiguities) cout << "warning: multimethod " << name << " has ambiguities" << endl;
         cout << toText(*this, 120);
      }
   }
#else
   std::vector<Any*> validMethods;
   for (auto &m : methods) validMethods.push_back(&m);//make a vector of pointers so we can easily test for equality
   simulate(validMethods, at, 0);
#endif
   needsBuilding = false;
}
void MMBase::fillParamType(bool first, int64_t paramIndex, int64_t typeIndex, TypeInfo* type)
{
   if (!first && paramTypeSets[paramIndex].contains(type)) return;
   if (type->mmpindices.size() <= mmindex)
      type->mmpindices.resize(mmindex + 1);
   if (type->mmpindices[mmindex].size() <= paramIndex)
      type->mmpindices[mmindex].resize(paramIndex + 1, -1);
   type->mmpindices[mmindex][paramIndex] = typeIndex;
   if (type->ref)
      fillParamType(first, paramIndex, typeIndex, type->ref);//ref to type goes to same method
   if (type->ptr)
      fillParamType(first, paramIndex, typeIndex, type->ptr);//ptr to type goes to same method
   for (auto d : type->conv.derived)
      fillParamType(false, paramIndex, typeIndex, d->derived);
}
void MMBase::simulate(std::vector<Any*> &validMethods, std::vector<TypeInfo*> &at, int64_t ai)
{
   //step 1
   if (ai < maxArgs)
   {
      for (auto at1 : paramTypeLists[ai])
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
      //now find the best method by a similar process
      //the best method is definitely among those that have the best (valid) param type for the argument types currently in 'at'
      for (size_t ai2 = 0; ai2 < maxArgs; ++ai2)
      {
         auto atlb = at[ai2]->conv.linear.begin();//atl = argument type linearisation
         auto atle = at[ai2]->conv.linear.end();
         auto atlmin = atle;
         TypeInfo* ptmin = nullptr;
         //find the best valid param type (not just the best one still in bestMethods)
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
}

Any* MMBase::getMethod(Any* args, int64_t nargs)
{
   if (useTable)
   {
      return getMethodFromTable(args, nargs);
   }
   else
   {
      for (auto &method : methods)
      {
         for (size_t ai = 0; ai < nargs; ++ai)
         {
            TypeInfo* fromt;
            TypeInfo* tot;
            if (useReturnT && ai == 0) 
            {
               tot = args[ai];
               fromt = method.paramType(ai, useReturnT);
            }
            else
            {
               fromt = args[ai].typeInfo;
               tot = method.paramType(ai, useReturnT);
            }
            if (!canCast(tot, fromt))
            {
               goto nextMethod2;
            }
         }
         return &method;
      nextMethod2:;
      }
      std::cerr << "multimethod not applicable to type(s)" << endl;
      std::cerr << "MULTIMETHOD " << name << " CALLED WITH: ";
      for (size_t ai = 0; ai < nargs; ++ai)
         std::cerr << args[ai].typeName() << "   ";
      std::cerr << std::endl;
      std::cerr << "multimethod contains these methods:" << std::endl;
      for (size_t mi = 0; mi < methods.size(); ++mi)
         std::cerr << toText(methods[mi], LLONG_MAX) << std::endl;
      throw "multimethod not applicable to type(s)";
   }
}


Any* MMBase::getMethodFromTable(Any* args, int64_t nargs)
{
   if (needsBuilding)
      buildTable();
   Any* address = table.mem;
   int64_t mult = 1;
   if (useReturnT)
   {
      address += args[0].as<TypeInfo*>()->mmpindices[mmindex][0] * mult;
      mult *= table.sizes[0];
      for (int64_t ai = 1; ai < nargs; ++ai)
      {
         address += args[ai].typeInfo->mmpindices[mmindex][ai] * mult;
         mult *= table.sizes[ai];
      }
   }
   else
   {
      for (int64_t ai = 0; ai < nargs; ++ai)
      {
         //TypeInfo* typeInfo = args[ai].typeInfo;
         while (true)
         {
            vector<vector<int64_t>>& mmpindices(args[ai].typeInfo->mmpindices);
            if (mmpindices.size() > mmindex && mmpindices[mmindex].size() > ai)
            {
               address += mmpindices[mmindex][ai] * mult;
               break;
            }
            else if (args[ai].typeInfo->of == tiAny)
               args[ai] = args[ai].derp();
            else if (args[ai].typeInfo == tiAny)
               args[ai] = args[ai].deany();
            else
               throw "MULTIMETHOD NOT APPLICABLE TO TYPE";
            /*
            if (typeInfo->of == tiAny)
               typeInfo = typeInfo->of;
            else if (typeInfo == tiAny)
               typeInfo = 
            */
         }
         mult *= table.sizes[ai];
      }
   }
   return address;
}
/*
* remember that actual arguments are copied to formal parameters
* so arguments must be convertible to parameter types
* so parameters must be a base of the arguments
* 
* thoughts on a faster algorithm
* 
* an argument list that is the same is an error, so ignore those
* an argument that is not convertible to the parameter is invalid
* an argument list that is all (the same or worse) is replaced
* an argument list that is all (the same or better) replaces
* so that only leaves: a blocking argument list must be better on at least one argument and worse on at least one, ie. the argument lists are unordered
*                 and: a list with an argument that multiply inherits from two different parameter types (but could decide by linearisation)
*
* clear the multidimensional table
* mark the available methods in the table
* loop through the table from least specific to most specific, normal row by row, column by column will work 
* at each cell:
*    if it's already filled (by an exact method), leave it
*    look at the cells previous to it in each dimension and see how many DIFFERENT methods there are
*       if there is only a single method, fill the current one with the same
*       if there is more than one method, we have an ambiguity
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
* types written the sane way instead of the c inside out way
* 
*                           can assign this
*  to this         |* int|* const int|const * int|const * const int|
*       *       int|ia   |           |ia         |
*       * const int|
* const *       int|
* const * const int|
* 
* cant set a * int to point to a const int
* a * const int can point to an int or a const int
* 
* if you set a * * const int to point to a * const int, that's fine
* if you set a * * const int to point to a * int, like a * const int points to an int, 
*    THAT is when the trouble starts because you could then, via the pointer to pointer, 
*    reassign that * int to point to a const int, because it thinks it's a * const int according to the type signature
* 

int = int
int = const int
const int -- can't assign at all, only initialise

*int = *int
*int = *const int  // not allowed
*const int = *const int
*const int = *int

**int = **int
**const int = **const int
**int = **const int //obviously not allowed
**const int = **int //not allowed but less obvious
*const*const int = **int //ok


const int a = 1
* int pa;
* * const int ppa = &pa; // we have to disallow this
* const * const int ppa2 = &pa // but this is ok because the extra const disallows *ppa2 = &a
*ppa = &a;
*pa = 2;

* 
* cost of numeric conversions
* 
* things to be lost bad to not so bad
* 
* giant amounts of range as in double -> int8
* the whole negative sign as in int -> uint
* fractional part
* significant figures
*
* 1-3
* perfect conversions (but they are a waste of memory)
* float -> double
* int8 -> int16 -> int32 -> int64
* uint8 -> uint16 -> uint32 -> uint64
* uint8 -> int16/int32/int64
* uint16 -> int32/int64
* uint32 -> int64
* 
* 10
* losing 1 bit of range
* uintx -> intx
* 
* int64 uint64
* |    \   |
* int32 uint32
* |    \   |
* int16 uint16
* |    \   |
* int8  uint8
* 
* int64       
*   |  \    
* int32 uint64
*   |  \   |
* int16 uint32
*   |  \   |
*  int8 uint16
*      \   |
*       uint8
*                                    from
*  to     double float int64 uint64 int32 uint32 int16 uint16 int8 uint8
* double       0     1                                                    
* float              0                                                            
* int64                    0     10     1      1     2      2    3     3
* uint64                          0            1            2          3
* int32                                 0     10     1      1    2     2
* uint32                                       0            1          2
* int16                                              0     10    1     1
* uint16                                                    0          1
* int8                                                           0    10
* uint8                                                                0
*
* question: what is the equivalent of the base class-derived class relation in terms of conversions?
* is it (a) the base has a cheaper conversion to the parameter type than the derived
*       (b) the base is the type that the derived converts to cheapest
*       (c) the derived converts to the base cheaper than the base converts to the param
* 
* leaning towards (b)
* 
* think relation needs to be transitive for the new algorithm to work
* 
* what we want is for the cheapest convert from a derived 
* 
* what we want is for the predecessor cells to contain the best method for the area they cover, i.e. the area they cover contains no method with a cheaper parameter conversion
* 
* we need to search the previous cells from cheapest conversion to most expensive stopping when we find something
* 
* we're in cell 3, we search cell 2. say cell 2's only previous cell was 1
* 
* 
class O
class A extends O             A O
class B extends O             B O
class C extends O             C O
class D extends O             D O
class E extends O             E O
class K1 extends C, B, A      C B A O
class K2 extends A, D         A D O
class K3 extends B, D, E      B D E O
class Z extends K1, K2, K3    Z K1 C K2 K3 B A D E O

In K3, K3 precedes A, but in Z it is reversed

I think C3 linearisation is what I need. Just check the local bases for methods AND THEN choose the one closest to the head of the linearisation. That would certainly make it easier 
*/
#if 0
paramConversions.clear();
paramConversions.resize(maxArgs);
//due to the fact that the only thing that matters is which type an argument converts to with the lowest cost,
//all arguments that convert to that type cheapest can have the same index
for (size_t pi = 0; pi < maxArgs; ++pi)
{ 
   for (int64_t fromi = 0; fromi < convert.paramTypeLists[1].size(); ++fromi)
   {
      vector<int64_t> costi(2);
      costi[1] = fromi;
      int64_t minCost = maxCost;
      int64_t minIndex = -1;
      TypeInfo* minType = nullptr;
      for (int64_t toi = 0; toi < convert.paramTypeLists[0].size(); ++toi)
      {
         if (paramTypeSets[pi].contains(convert.paramTypeLists[0][toi]))
         {
            costi[0] = toi;
            int64_t cost = convert.costs[costi];
            if (cost < minCost)
            {
               minCost = cost;
               minIndex = toi;
               minType = convert.paramTypeLists[0][toi];
            }
         }
      }
      if (minType)//if this is not null then an argument of type convert.paramTypeLists[1][fromi] is valid
      {
         int64_t newIndex = find(paramTypeLists[pi].begin(), paramTypeLists[pi].end(), minType) - paramTypeLists[pi].begin();
         convert.paramTypeLists[1][fromi]->mmpindices[mmindex][pi] = newIndex;
      }


      for (auto type : paramTypeLists[pi])
      {
         costi[0] = type->mmpindices[convert.mmindex][pi];
         int64_t cost = convert.costs[costi];

         MMParamType paramType;
         paramType.type = type;
         paramConversions[pi][fromi].conversions.push_back(make_pair(convert.costs[costi], convert.paramConversions[1][fromi].type));
      }
      sort(paramType.conversions.begin(), paramType.conversions.end());
      paramConversions[pi].push_back(paramType);
   }
}
#endif
#if 0
for (size_t pi = 0; pi < maxArgs; ++pi)
{ 
   for (auto type : paramTypeSets[pi])
   {
      MMParamType paramType;
      paramType.type = type;
      for (size_t fromi = 0; fromi < convert.paramConversions[1].size(); ++fromi)
      {
         vector<int64_t> costi(2);
         costi[0] = type->mmpindices[mmindex][pi];
         costi[1] = fromi;
         paramType.conversions.push_back(make_pair(convert.costs[costi], convert.paramConversions[1][fromi].type));
      }
      sort(paramType.conversions.begin(), paramType.conversions.end());
      paramConversions[pi].push_back(paramType);
   }
}
#endif
