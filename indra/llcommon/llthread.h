/** 
 * @file llthread.h
 * @brief Base classes for thread, mutex and condition handling.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLTHREAD_H
#define LL_LLTHREAD_H

#define IS_LLCOMMON_INLINE (!LL_COMMON_LINK_SHARED || defined(llcommon_EXPORTS))

#if LL_GNUC
// Needed for is_main_thread() when compiling with optimization (relwithdebinfo).
// It doesn't hurt to just always specify it though.
//#pragma interface
#endif

#include "llapp.h"
#include "llapr.h"
#include "llaprpool.h"
#include "llatomic.h"
#include "llmemory.h"
#include "aithreadid.h"

class LLThread;
class LLMutex;
class LLCondition;

class LL_COMMON_API LLThreadLocalDataMember
{
public:
	virtual ~LLThreadLocalDataMember() { };
};

class LL_COMMON_API LLThreadLocalData
{
private:
	static apr_threadkey_t* sThreadLocalDataKey;

public:
	// Thread-local memory pool.
	LLAPRRootPool mRootPool;
	LLVolatileAPRPool mVolatileAPRPool;
	LLThreadLocalDataMember* mCurlMultiHandle;	// Initialized by AICurlMultiHandle::getInstance
	char* mCurlErrorBuffer;						// NULL, or pointing to a buffer used by libcurl.
	std::string mName;							// "main thread", or a copy of LLThread::mName.

	static void init(void);
	static void destroy(void* thread_local_data);
	static void create(LLThread* pthread);
	static LLThreadLocalData& tldata(void);

private:
	LLThreadLocalData(char const* name);
	~LLThreadLocalData();
};

// Print to llerrs if the current thread is not the main thread.
LL_COMMON_API void assert_main_thread();

class LL_COMMON_API LLThread
{
private:
	static U32 sIDIter;
	static LLAtomicS32	sCount;
	static LLAtomicS32	sRunning;

public:
	typedef enum e_thread_status
	{
		STOPPED = 0,	// The thread is not running.  Not started, or has exited its run function
		RUNNING = 1,	// The thread is currently running
		QUITTING= 2 	// Someone wants this thread to quit
	} EThreadStatus;

	LLThread(std::string const& name);
	virtual ~LLThread(); // Warning!  You almost NEVER want to destroy a thread unless it's in the STOPPED state.
	virtual void shutdown(); // stops the thread
	
	bool isQuitting() const { return (QUITTING == mStatus); }
	bool isStopped() const { return (STOPPED == mStatus); }
	
	static S32 getCount() { return sCount; }	
	static S32 getRunning() { return sRunning; }
	static void yield(); // Static because it can be called by the main thread, which doesn't have an LLThread data structure.

public:
	// PAUSE / RESUME functionality. See source code for important usage notes.
	// Called from MAIN THREAD.
	void pause();
	void unpause();
	bool isPaused() { return isStopped() || mPaused; }
	
	// Cause the thread to wake up and check its condition
	void wake();

	// Same as above, but to be used when the condition is already locked.
	void wakeLocked();

	// Called from run() (CHILD THREAD). Pause the thread if requested until unpaused.
	void checkPause();

	// this kicks off the apr thread
	void start(void);

	// Can be used to tell the thread we're not interested anymore and it should abort.
	void setQuitting();
	
	// Return thread-local data for the current thread.
	static LLThreadLocalData& tldata(void) { return LLThreadLocalData::tldata(); }

private:
	bool				mPaused;
	
	// static function passed to APR thread creation routine
	static void *APR_THREAD_FUNC staticRun(apr_thread_t *apr_threadp, void *datap);

protected:
	std::string			mName;
	LLCondition*		mRunCondition;

	apr_thread_t		*mAPRThreadp;
	volatile EThreadStatus		mStatus;
	
	friend void LLThreadLocalData::create(LLThread* threadp);
	LLThreadLocalData*  mThreadLocalData;

	// virtual function overridden by subclass -- this will be called when the thread runs
	virtual void run(void) = 0; 
	
	// This class is completely done (called from THREAD!).
	virtual void terminated(void) { mStatus = STOPPED; }

	// virtual predicate function -- returns true if the thread should wake up, false if it should sleep.
	virtual bool runCondition(void);

	// Lock/Unlock Run Condition -- use around modification of any variable used in runCondition()
	inline void lockData();
	inline void unlockData();
	
	// This is the predicate that decides whether the thread should sleep.  
	// It should only be called with mRunCondition locked, since the virtual runCondition() function may need to access
	// data structures that are thread-unsafe.
	bool shouldSleep(void) { return (mStatus == RUNNING) && (isPaused() || (!runCondition())); }

	// To avoid spurious signals (and the associated context switches) when the condition may or may not have changed, you can do the following:
	// mRunCondition->lock();
	// if(!shouldSleep())
	//     mRunCondition->signal();
	// mRunCondition->unlock();
};

#ifdef SHOW_ASSERT
#define ASSERT_SINGLE_THREAD do { static AIThreadID first_thread_id; llassert(first_thread_id.equals_current_thread()); } while(0)
#else
#define ASSERT_SINGLE_THREAD do { } while(0)
#endif

//============================================================================

#define MUTEX_POOL(arg)

#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp> 
#include <boost/thread/condition_variable.hpp>
typedef boost::recursive_mutex LLMutexImpl;
typedef boost::condition_variable_any LLConditionVariableImpl;

#include "llfasttimer.h"

class LL_COMMON_API LLMutex : public LLMutexImpl
{
public:
	LLMutex(MUTEX_POOL(native_pool_type& pool = LLThread::tldata().mRootPool)) 
	:	LLMutexImpl(MUTEX_POOL(pool)), mLockingThread(AIThreadID::sNone) { }
	~LLMutex() { }

	void lock(LLTrace::BlockTimerStatHandle* timer = NULL)	// blocks
	{
		if (inc_lock_if_recursive()) { return; }
#if IS_LLCOMMON_INLINE
		if (AIThreadID::in_main_thread_inline() && LLApp::isRunning())
#else
		if (AIThreadID::in_main_thread() && LLApp::isRunning())
#endif
		{
			if (!LLMutexImpl::try_lock())
			{
				lock_main(timer);
			}
		}
		else
		{
			LLMutexImpl::lock();
		}
#if IS_LLCOMMON_INLINE
		mLockingThread.reset_inline();
#else
		mLockingThread.reset();
#endif
	}

	void unlock()
	{
		mLockingThread = AIThreadID::sNone;
		LLMutexImpl::unlock();
	}

	// Returns true if lock was obtained successfully.
	bool try_lock()
	{
		if (inc_lock_if_recursive())
			return true;
		if (!LLMutexImpl::try_lock())
			return false;
#if IS_LLCOMMON_INLINE
		mLockingThread.reset_inline();
#else
		mLockingThread.reset();
#endif
		return true;
	}
	
	// Returns true if locked not by this thread
	bool isLocked()
	{
		if (isSelfLocked())
			return false;
		if (LLMutexImpl::try_lock())
		{
			LLMutexImpl::unlock();
			return false;
		}
		return true;
	}
	// Returns true if locked by this thread.
	bool isSelfLocked() const
	{
#if IS_LLCOMMON_INLINE
		return mLockingThread.equals_current_thread_inline();
#else
		return mLockingThread.equals_current_thread();
#endif
	}

private:
	void lock_main(LLTrace::BlockTimerStatHandle* timer);

	bool inc_lock_if_recursive()
	{
		return false;
	}

	mutable AIThreadID	mLockingThread;
};

class LLGlobalMutex : public LLMutex
{
public:
	LLGlobalMutex() : LLMutex(MUTEX_POOL(LLAPRRootPool::get())), mbInitalized(true)
	{}
	bool isInitalized() const
	{
		return mbInitalized;
	}
private:
	bool mbInitalized;
};

typedef LLMutex LLMutexRootPool;

// Actually a condition/mutex pair (since each condition needs to be associated with a mutex).
class LLCondition : public LLConditionVariableImpl, public LLMutex
{
public:
	LLCondition(MUTEX_POOL(native_pool_type& pool = LLThread::tldata().mRootPool)) : 
		LLMutex(MUTEX_POOL(pool)), 
		LLConditionVariableImpl(MUTEX_POOL(pool))
	{}
	~LLCondition()
	{}
	void wait()
	{
#if IS_LLCOMMON_INLINE
		if (AIThreadID::in_main_thread_inline())
#else
		if (AIThreadID::in_main_thread())
#endif
			wait_main();
		else LLConditionVariableImpl::wait(*this);
	}
	void signal()		{ LLConditionVariableImpl::notify_one(); }
	void broadcast()	{ LLConditionVariableImpl::notify_all(); }
private:
	LL_COMMON_API void wait_main();	//Cannot be inline. Uses internal fasttimer.
};

class LLMutexLock
{
public:
	LLMutexLock(LLMutex* mutex)
	{
		mMutex = mutex;
		lock();
	}
	LLMutexLock(LLMutex& mutex)
	{
		mMutex = &mutex;
		lock();
	}
	~LLMutexLock()
	{
		if (mMutex) mMutex->unlock();
	}
private:
	LL_COMMON_API void lock();	//Cannot be inline. Uses internal fasttimer.
	LLMutex* mMutex;
};

class AIRWLock
{
public:
	AIRWLock(LLAPRPool& parent = LLThread::tldata().mRootPool) :
		mWriterWaitingMutex(MUTEX_POOL(parent)), mNoHoldersCondition(MUTEX_POOL(parent)), mHoldersCount(0), mWriterIsWaiting(false) { }

private:
	LLMutex mWriterWaitingMutex;		//!< This mutex is locked while some writer is waiting for access.
	LLCondition mNoHoldersCondition;	//!< Access control for mHoldersCount. Condition true when there are no more holders.
	int mHoldersCount;					//!< Number of readers or -1 if a writer locked this object.
	// This is volatile because we read it outside the critical area of mWriterWaitingMutex, at [1].
	// That means that other threads can change it while we are already in the (inlined) function rdlock.
	// Without volatile, the following assembly would fail:
	// register x = mWriterIsWaiting;
	// /* some thread changes mWriterIsWaiting */
	// if (x ...
	// However, because the function is fuzzy to begin with (we don't mind that this race
	// condition exists) it would work fine without volatile. So, basically it's just here
	// out of principle ;).	-- Aleric
    bool volatile mWriterIsWaiting;		//!< True when there is a writer waiting for write access.

public:
	void rdlock(bool high_priority = false)
	{
		// Give a writer a higher priority (kinda fuzzy).
		if (mWriterIsWaiting && !high_priority)	// [1] If there is a writer interested,
		{
			mWriterWaitingMutex.lock();			// [2] then give it precedence and wait here.
			// If we get here then the writer got it's access; mHoldersCount == -1.
			mWriterWaitingMutex.unlock();
		}
		mNoHoldersCondition.lock();				// [3] Get exclusive access to mHoldersCount.
		while (mHoldersCount == -1)				// [4]
		{
		  	mNoHoldersCondition.wait();			// [5] Wait till mHoldersCount is (or just was) 0.
		}
		++mHoldersCount;						// One more reader.
		mNoHoldersCondition.unlock();			// Release lock on mHoldersCount.
	}
	void rdunlock(void)
	{
		mNoHoldersCondition.lock();				// Get exclusive access to mHoldersCount.
		if (--mHoldersCount == 0)				// Was this the last reader?
		{
			mNoHoldersCondition.signal();		// Tell waiting threads, see [5], [6] and [7].
		}
		mNoHoldersCondition.unlock();			// Release lock on mHoldersCount.
	}
	void wrlock(void)
	{
		mWriterWaitingMutex.lock();				// Block new readers, see [2],
		mWriterIsWaiting = true;				// from this moment on, see [1].
		mNoHoldersCondition.lock();				// Get exclusive access to mHoldersCount.
		while (mHoldersCount != 0)				// Other readers or writers have this lock?
		{
		  	mNoHoldersCondition.wait();			// [6] Wait till mHoldersCount is (or just was) 0.
		}
		mWriterIsWaiting = false;				// Stop checking the lock for new readers, see [1].
		mWriterWaitingMutex.unlock();			// Release blocked readers, they will still hang at [3].
		mHoldersCount = -1;						// We are a writer now (will cause a hang at [5], see [4]).
		mNoHoldersCondition.unlock();			// Release lock on mHolders (readers go from [3] to [5]).
	}
	void wrunlock(void)
	{
		mNoHoldersCondition.lock();				// Get exclusive access to mHoldersCount.
		mHoldersCount = 0;						// We have no writer anymore.
		mNoHoldersCondition.signal();			// Tell waiting threads, see [5], [6] and [7].
		mNoHoldersCondition.unlock();			// Release lock on mHoldersCount.
	}
	void rd2wrlock(void)
	{
		mNoHoldersCondition.lock();				// Get exclusive access to mHoldersCount. Blocks new readers at [3].
		if (--mHoldersCount > 0)				// Any other reads left?
		{
			mWriterWaitingMutex.lock();			// Block new readers, see [2],
			mWriterIsWaiting = true;			// from this moment on, see [1].
			while (mHoldersCount != 0)			// Other readers (still) have this lock?
			{
				mNoHoldersCondition.wait();		// [7] Wait till mHoldersCount is (or just was) 0.
			}
			mWriterIsWaiting = false;			// Stop checking the lock for new readers, see [1].
			mWriterWaitingMutex.unlock();		// Release blocked readers, they will still hang at [3].
		}
		mHoldersCount = -1;						// We are a writer now (will cause a hang at [5], see [4]).
		mNoHoldersCondition.unlock();			// Release lock on mHolders (readers go from [3] to [5]).
	}
	void wr2rdlock(void)
	{
		mNoHoldersCondition.lock();				// Get exclusive access to mHoldersCount.
		mHoldersCount = 1;						// Turn writer into a reader.
		mNoHoldersCondition.signal();			// Tell waiting readers, see [5].
		mNoHoldersCondition.unlock();			// Release lock on mHoldersCount.
	}
#if LL_DEBUG
	// Really only intended for debugging purposes:
	bool isLocked(void)
	{
		mNoHoldersCondition.lock();
		bool res = mHoldersCount;
		mNoHoldersCondition.unlock();
		return res;
	}
#endif
};

#if LL_DEBUG
class AINRLock
{
private:
	int read_locked;
	int write_locked;

	mutable bool mAccessed;
	mutable AIThreadID mTheadID;

	void accessed(void) const
	{
		if (!mAccessed)
		{
			mAccessed = true;
			mTheadID.reset();
		}
		else
		{
			llassert_always(mTheadID.equals_current_thread());
		}
	}

public:
	AINRLock(void) : read_locked(false), write_locked(false), mAccessed(false) { }

	bool isLocked() const { return read_locked || write_locked; }

	void rdlock(bool high_priority = false) { accessed(); ++read_locked; }
	void rdunlock() { --read_locked; }
	void wrlock() { llassert(!isLocked()); accessed(); ++write_locked; }
	void wrunlock() { --write_locked; }
	void wr2rdlock() { llassert(false); }
	void rd2wrlock() { llassert(false); }
};
#endif

//============================================================================

void LLThread::lockData()
{
	mRunCondition->lock();
}

void LLThread::unlockData()
{
	mRunCondition->unlock();
}


//============================================================================

// see llmemory.h for LLPointer<> definition

class LLThreadSafeRefCount
{
private:
	LLThreadSafeRefCount(const LLThreadSafeRefCount&); // not implemented
	LLThreadSafeRefCount&operator=(const LLThreadSafeRefCount&); // not implemented

protected:
	virtual ~LLThreadSafeRefCount() // use unref()
	{
		if (mRef != 0)
		{
			LL_ERRS() << "deleting non-zero reference" << LL_ENDL;
		}
	}

public:
	LLThreadSafeRefCount() : mRef(0)
	{}

	void ref()
	{
		mRef++;
	}

	void unref()
	{
		llassert(mRef > 0);
		if (--mRef == 0) delete this;
	}
	S32 getNumRefs() const
	{
		return mRef;
	}

private: 
	LLAtomicS32	mRef; 
};

//============================================================================

// Simple responder for self destructing callbacks
// Pure virtual class
class LLResponder : public LLThreadSafeRefCount
{
protected:
	virtual ~LLResponder() {}
public:
	virtual void completed(bool success) = 0;
};

//============================================================================

#endif // LL_LLTHREAD_H
