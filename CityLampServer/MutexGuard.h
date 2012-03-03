#pragma once
#include <windows.h>
class Mutex {
public:
    Mutex() {InitializeCriticalSection(&m_cs);}
    virtual ~Mutex() {DeleteCriticalSection(&m_cs);}
    void Lock() {EnterCriticalSection(&m_cs);}
    void UnLock() {LeaveCriticalSection(&m_cs);}
private:
    CRITICAL_SECTION m_cs;
};
class MutexGuard
{
public:
	explicit MutexGuard(Mutex &mutex) : m_mutex(mutex) { m_mutex.Lock();}
	virtual ~MutexGuard() { m_mutex.UnLock();}
private:
	Mutex& m_mutex;
};

