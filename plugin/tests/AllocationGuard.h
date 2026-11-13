#pragma once
#include <cstdlib>
#include <new>
namespace allocationGuard {inline thread_local bool active=false;inline thread_local size_t allocations=0,frees=0;}
void* operator new(size_t n){if(allocationGuard::active)++allocationGuard::allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void* operator new(size_t n,const std::nothrow_t&)noexcept{try{return ::operator new(n);}catch(...){return nullptr;}}
void* operator new[](size_t n,const std::nothrow_t& tag)noexcept{return ::operator new(n,tag);}
void operator delete(void* p)noexcept{if(p&&allocationGuard::active)++allocationGuard::frees;std::free(p);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,size_t)noexcept{::operator delete(p);}
void operator delete[](void* p,size_t)noexcept{::operator delete(p);}
void operator delete(void* p,const std::nothrow_t&)noexcept{::operator delete(p);}
void operator delete[](void* p,const std::nothrow_t&)noexcept{::operator delete(p);}
