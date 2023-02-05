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

void addTypeLink(TypeInfo* base, TypeInfo* derived, LinkKind kind, int64_t offset)
{
   TypeLink* tl = new TypeLink(base, derived, kind, offset);
   base->derived.push_back(tl);
   derived->bases.push_back(tl);
}

Any delegVoid0(Any* fp1, Any* params, int64_t n)
{
   void (*fp)() = *fp1;
   fp();
   return Any(0);
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
   throw "member not found!";
}
Member* MemberList::operator[](std::string name)
{
   auto i = byName.find(name);
   if (i != byName.end())
   {
      return *i;
   }
   throw "member not found!";
}

Member* MemberList::operator[](int64_t n)
{
   return byOffset[n];
}

int64_t MemberList::size()
{
   return byOffset.size();
}

void MemberList::clear()
{
   byPtr.clear();
   bySymbol.clear();
   byName.clear();
   byOffset.clear();
}

Any::Any() : typeInfo(nullptr), ptr(nullptr)
{
   construct(0);
}

Any::Any(const Any& rhs)
{
   typeInfo = rhs.typeInfo;
   ptr = new char[typeInfo->size];
   typeInfo->copyCon(ptr, rhs.ptr, typeInfo);
}

Any::Any(Any&& rhs) : typeInfo(rhs.typeInfo), ptr(rhs.ptr)
{
   rhs.ptr = nullptr;
}  

Any& Any::operator=(const Any& rhs)
{
   delete[] static_cast<char*>(ptr);
   typeInfo = rhs.typeInfo;
   ptr = new char[typeInfo->size];
   typeInfo->copyCon(ptr, rhs.ptr, typeInfo);
   return *this;
}
Any& Any::operator=(Any&& rhs)
{
   delete[] static_cast<char*>(ptr);
   typeInfo = rhs.typeInfo;
   ptr = rhs.ptr;
   rhs.ptr = nullptr;
   return *this;
}

Any::Any(TypeInfo* typeInfo, void* rhs) : typeInfo(typeInfo)
{
   ptr = new char[typeInfo->size];//creates a copy on construction
   typeInfo->copyCon(ptr, rhs, typeInfo);
}
Any::~Any()
{
   delete[] static_cast<char*>(ptr);
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
   return Any(typeInfo->of, *(void**)ptr);
}

Any::operator MMBase& ()
{
   if (typeInfo->kind == kPointer && typeInfo->of->multimethod) return **static_cast<MMBase**>(ptr);
   if (typeInfo->multimethod) return *static_cast<MMBase*>(ptr);
   std::cout << TS + "type mismatch: casting " + typeInfo->name + " to MMBase&" << std::endl;
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
   return typeInfo == tiListClosure || typeInfo == tiStructClosure || typeInfo == tiVarFunc || typeInfo->multimethod || typeInfo->kind == kFunction;
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
   unsigned char* b = (unsigned char*)ptr;
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

Any Any::add(Member* m, Any value)
{

   TypeInfo* typeInfoNew = new TypeInfo(*typeInfo);
   typeInfoNew->add(m);
   void* ptrNew = new char[typeInfoNew->size];
   typeInfo->copyCon(ptrNew, ptr, typeInfo);
   delete[](ptr);
   ptr = ptrNew;
   typeInfo = typeInfoNew;
   m->setMember(m, *this, value);
   return value;
}

Any Any::member(Member* m)
{
#if 0
   TypeInfo* tiNew = new TypeInfo;
   tiNew->kind = kReference;
   tiNew->of = typeInfo;
   tiNew->copyCon = CopyCon<void*>::go;
   tiNew->destroy = destroynothing;
   tiNew->delegPtr = nullptr;
   tiNew->size = sizeof(void*);
   //TypeInfo* ti = *typeSetR.find(typeInfo->cType);
   char* ptrNew = reinterpret_cast<char*>(ptr) + m->offset;
   return Any(tiNew, reinterpret_cast<void*>(&ptrNew));
#else
   return (*m->getMemberRef)(m, *this);
#endif
}

Any Any::member(Symbol* symbol)
{
   Member* m = (*typeInfo)[symbol];
   return member(m);
   //return (*m->getMemberRef)(m, *this);
}

Any Any::member(std::string name)
{
   Member* m = (*typeInfo)[name];
   return member(m);
   //return (*m->getMemberRef)(m, *this);
}

Any Any::member(int64_t n)
{
   Member* m = (*typeInfo)[n];
   return member(m);
   //return (*m->getMemberRef)(m, *this);
}

void TypeInfo::doC3Lin()
{
   typedef deque<TypeList> TypeLists;
   TypeLists baseLins;
   TypeList  result;
   result.push_back(this);

   if (!bases.empty())
   {
      int64_t blCount = bases.size();
      for (TypeLinkList::iterator b = bases.begin(); b != bases.end(); ++b)
      {
         if ((*b)->base->linear.empty()) (*b)->base->doC3Lin();
         baseLins.push_back((*b)->base->linear);
      }
      while (blCount)
      {
         //loop through all the base linearisations, looking at the heads to see if any are good
         for (size_t baseI = 0; baseI < baseLins.size(); ++baseI)
         {
            if (baseLins[baseI].empty()) continue;
            TypeInfo* head = baseLins[baseI][0];

            //loop through the tails of the base linearisations, making sure this head does not occur, if it does it is BAD
            for (TypeLists::iterator tailCheck = baseLins.begin(); tailCheck != baseLins.end(); ++tailCheck)
            {
               for (size_t i = 1; i < tailCheck->size(); ++i)
               {
                  if (head == (*tailCheck)[i]) goto bad;
               }
            }
            result.push_back(head);

            //now remove this good head from ALL the base linearisations
            for (int64_t j = baseLins.size() - 1; j >= 0; --j)
            {
               if (baseLins[j][0] == head)//only need to check the first since we know it's not in the tail
               {
                  baseLins[j].erase(baseLins[j].begin());
                  if (baseLins[j].empty()) --blCount;
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
   linear = result;
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
      for (auto p : bases)
      { 
         p->base->allocate();
         p->offset = size;
         size += p->base->size;
      }

      for (auto m : members.byOffset)
      {
         m->offset = size;
         m->typeInfo->allocate();
         size += m->typeInfo->size;
      }
   }
   layout.clear();
   TypeLinkList tll;
   allocate2(this, 0, tll);
}

void TypeInfo::allocate2(TypeInfo* derived, int64_t baseOffset, TypeLinkList& links)
{
   for (auto p : bases)
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
      Member* newM = new Member(m->symbol, m->typeInfo, baseOffset + m->offset, m->ptrToMember);
      newM->links = links;
      newM->getMember = m->getMember;
      newM->getMemberRef = m->getMemberRef;
      newM->setMember = m->setMember;
      newM->ptrToMember = m->ptrToMember;
      derived->allMembers.add(newM);
   }
}

TypeInfo* findLink(TypeInfo* base, TypeInfo* derived)
{
   for (TypeList::iterator l = derived->linear.begin(); l != derived->linear.end(); ++l)
   {
      if (*l == base) return *l;
   }
   return nullptr;
}

bool hasLink(TypeInfo* base, TypeInfo* derived)
{
   if (base == derived) return true;
   for (TypeList::iterator l = derived->linear.begin(); l != derived->linear.end(); ++l)
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
   for (size_t i = 0; i < derived->linear.size(); ++i)
   {
      if (derived->linear[i] == base) return i;
   }
   return -1;
}

/*
void addMember(Any member, string name)
{
   TypeInfo* ti = member.typeInfo;
   if (!ti->member)
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
   switch (kind)
   {
   case kNormal:
      o << this->name;
      break;
   case kReference:
      o << "&" << of->getName();
      break;
   case kPointer:
      o << "*" << of->getName();
      break;
   case kPtrMem:
      o << members.byOffset[0]->typeInfo->name << "::* " << of->getName();
      break;
   case kArray:
      o << "[" << count << "]" << of->getName();
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
   m->offset = size;
   size += m->typeInfo->size;
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
MissingArg mA;

Any delegPartApply(Any* cl, Any* argsVar, int64_t n)
{
   PartApply* clos = *cl;
   size_t nArgsAll = clos->argsFixed.size() + n;
   Any* argsAll = new Any[nArgsAll];
   size_t afi = 0;
   size_t avi = 0;
   for (size_t aai = 0; aai < nArgsAll; ++aai)
   {
      if (clos->pArgsFixed & (1 << aai))
         argsAll[aai] = clos->argsFixed[afi++];
      else
         argsAll[aai] = argsVar[avi++];
   }
   //Any res = clos->call(argsAll[0], &argsAll[1], nArgsAll - 1);
   Any res = argsAll[0].call(&argsAll[1], nArgsAll - 1);
   delete[] argsAll;
   return res;
}

PartApply* makePartApply(Any f, Any a0)
{
   PartApply* res = new PartApply;
   res->pArgsFixed = 3;//binary - means members 0 and 1 are fixed
   res->argsFixed.push_back(f);
   res->argsFixed.push_back(a0);
   res->delegPtr = f.typeInfo->delegPtr;
   return res;
}

PartApply* makePartApply(Any f, Any a0, Any a1)
{
   PartApply* res = new PartApply;
   res->pArgsFixed = 7;//binary - means members 0, 1 & 2 are fixed
   res->argsFixed.push_back(f);
   res->argsFixed.push_back(a0);
   res->argsFixed.push_back(a1);
   res->delegPtr = f.typeInfo->delegPtr;
   return res;
}

extern TypeInfo* tiListClosure;
extern TypeInfo* tiStructClosure;
extern TypeInfo* tiVarFunc;

Any delegVF(Any* fp, Any* args, int64_t n)
{
   return fp->as<VarFunc>().func(args, n);
}

void addMemberInterp(TypeInfo* _class, std::string name, TypeInfo* type)
{
   _class->members.add(new Member(getSymbol(name), type, _class->size));
}
