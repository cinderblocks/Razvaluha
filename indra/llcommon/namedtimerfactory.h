#include "llfasttimer.h"

// factory class that creates NamedTimers via static DeclareTimer objects
class NamedTimerFactory : public LLSingleton<NamedTimerFactory>
{
	LLSINGLETON(NamedTimerFactory)
		: mActiveTimerRoot(nullptr),
		  mTimerRoot(nullptr),
		  mAppTimer(nullptr),
		  mRootFrameState(nullptr)
	{}
public:

	/*virtual */ void initSingleton()
	{
		mTimerRoot = new LLFastTimer::NamedTimer("root");

		mActiveTimerRoot = new LLFastTimer::NamedTimer("Frame");
		mActiveTimerRoot->setCollapsed(false);

		mRootFrameState = new LLFastTimer::FrameState(mActiveTimerRoot);
		// getFrameState and setParent recursively call this function,
		// so we have to work around that by using a specialized implementation
		// for the special case were mTimerRoot != mActiveTimerRoot -- Aleric
		mRootFrameState->mParent = &LLFastTimer::getFrameStateList()[0];	// &mTimerRoot->getFrameState()
		mRootFrameState->mParent->mActiveCount = 1;
		// And the following four lines are mActiveTimerRoot->setParent(mTimerRoot);
		llassert(!mActiveTimerRoot->mParent);
		mActiveTimerRoot->mParent = mTimerRoot;								// mParent = parent;
		//mRootFrameState->mParent = mRootFrameState->mParent;				// getFrameState().mParent = &parent->getFrameState();
		mTimerRoot->getChildren().push_back(mActiveTimerRoot);				// parent->getChildren().push_back(this);
		mTimerRoot->mNeedsSorting = true;									// parent->mNeedsSorting = true;

		mAppTimer = new LLFastTimer(mRootFrameState);
	}

	~NamedTimerFactory()
	{
		std::for_each(mTimers.begin(), mTimers.end(), DeletePairedPointer());

		delete mAppTimer;
		delete mActiveTimerRoot; 
		delete mTimerRoot;
		delete mRootFrameState;
	}

	LLFastTimer::NamedTimer& createNamedTimer(const std::string& name)
	{
		timer_map_t::iterator found_it = mTimers.find(name);
		if (found_it != mTimers.end())
		{
			return *found_it->second;
		}

		LLFastTimer::NamedTimer* timer = new LLFastTimer::NamedTimer(name);
		timer->setParent(mTimerRoot);
		mTimers.insert(std::make_pair(name, timer));

		return *timer;
	}

	LLFastTimer::NamedTimer* getTimerByName(const std::string& name)
	{
		timer_map_t::iterator found_it = mTimers.find(name);
		if (found_it != mTimers.end())
		{
			return found_it->second;
		}
		return NULL;
	}

	LLFastTimer::NamedTimer* getActiveRootTimer() { return mActiveTimerRoot; }
	LLFastTimer::NamedTimer* getRootTimer() { return mTimerRoot; }
	const LLFastTimer* getAppTimer() { return mAppTimer; }
	LLFastTimer::FrameState& getRootFrameState() { return *mRootFrameState; }

	typedef std::map<std::string, LLFastTimer::NamedTimer*> timer_map_t;
	timer_map_t::iterator beginTimers() { return mTimers.begin(); }
	timer_map_t::iterator endTimers() { return mTimers.end(); }
	S32 timerCount() { return mTimers.size(); }

private:
	timer_map_t mTimers;

	LLFastTimer::NamedTimer*		mActiveTimerRoot;
	LLFastTimer::NamedTimer*		mTimerRoot;
	LLFastTimer*						mAppTimer;
	LLFastTimer::FrameState*		mRootFrameState;		// Points to memory allocated with new, so this pointer is not invalidated.
};
