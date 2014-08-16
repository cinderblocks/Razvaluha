/** 
 * @file llcompilequeue.h
 * @brief LLCompileQueue class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLCOMPILEQUEUE_H
#define LL_LLCOMPILEQUEUE_H

#include "llinstancetracker.h"
#include "llinventory.h"
#include "llviewerobject.h"
#include "llvoinventorylistener.h"
#include "lluuid.h"

#include "llfloater.h"

#include "llviewerinventory.h"

#include "llevents.h"

class LLScrollListCtrl;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterScriptQueue
//
// This class provides a mechanism of adding objects to a list that
// will go through and execute action for the scripts on each object. The
// objects will be accessed serially and the scripts may be
// manipulated in parallel. For example, selecting two objects each
// with three scripts will result in the first object having all three
// scripts manipulated.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterScriptQueue : public LLFloater/*, public LLVOInventoryListener*/
, public LLInstanceTracker<LLFloaterScriptQueue, LLUUID>
{
public:
	LLFloaterScriptQueue(const std::string& name, const LLRect& rect,
						 const std::string& title, const std::string& start_string);
	virtual ~LLFloaterScriptQueue();

	/*virtual*/ BOOL postBuild() override;

	void setMono(bool mono) { mMono = mono; }

	// addObject() accepts an object id.
	void addObject(const LLUUID& id, std::string name);

	// start() returns TRUE if the queue has started, otherwise FALSE.
	BOOL start();

    void addProcessingMessage(const std::string &message, const LLSD &args);
    void addStringMessage(const std::string &message);

    std::string getStartString() const { return mStartString; }

protected:
	static void onCloseBtn(void* user_data);

	// returns true if this is done
	BOOL isDone() const;

	virtual bool startQueue() = 0;

	void setStartString(const std::string& s) { mStartString = s; }

protected:
	// UI
	LLScrollListCtrl* mMessages;
	LLButton* mCloseBtn;

	// Object Queue
	struct ObjectData
	{
		LLUUID mObjectId;
		std::string mObjectName;
	};
	typedef std::vector<ObjectData> object_data_list_t;

	object_data_list_t mObjectList;
	LLUUID mCurrentObjectID;
	bool mDone;

	std::string mStartString;
	bool mMono;

    typedef std::function<bool(const LLPointer<LLViewerObject> &, LLInventoryObject*, LLEventPump &)>   fnQueueAction_t;
    static void objectScriptProcessingQueueCoro(std::string action, LLHandle<LLFloaterScriptQueue> hfloater, object_data_list_t objectList, fnQueueAction_t func);

};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterCompileQueue
//
// This script queue will recompile each script.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct LLCompileQueueData
{
	LLUUID mQueueID;
	LLUUID mItemId;
	LLCompileQueueData(const LLUUID& q_id, const LLUUID& item_id) :
		mQueueID(q_id), mItemId(item_id) {}
};

class LLFloaterCompileQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a compile queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterCompileQueue* create(bool mono);

	void experienceIdsReceived(const LLSD& content);
	BOOL hasExperience(const LLUUID& id) const;

protected:
	LLFloaterCompileQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterCompileQueue();
	
	bool startQueue() override;

    static bool processScript(LLHandle<LLFloaterCompileQueue> hfloater, const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump);

    //bool checkAssetId(const LLUUID &assetId);
    static void handleHTTPResponse(std::string pumpName, const LLSD &expresult);
    static void handleScriptRetrieval(LLVFS *vfs, const LLUUID& assetId, LLAssetType::EType type, void* userData, S32 status, LLExtStat extStatus);

private:
    static void processExperienceIdResults(LLSD result, LLUUID parent);
    //uuid_list_t mAssetIds;  // list of asset IDs processed.
	uuid_list_t mExperienceIds;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterResetQueue
//
// This script queue will reset each script.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterResetQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a reset queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterResetQueue* create();

protected:
	LLFloaterResetQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterResetQueue();

    static bool resetObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump);

	bool startQueue() override;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterRunQueue
//
// This script queue will set each script as running.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterRunQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a run queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterRunQueue* create();

protected:
	LLFloaterRunQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterRunQueue();

    static bool runObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump);

	bool startQueue() override;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterNotRunQueue
//
// This script queue will set each script as not running.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterNotRunQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a not run queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterNotRunQueue* create();

protected:
	LLFloaterNotRunQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterNotRunQueue();
	
    static bool stopObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump);

	bool startQueue() override;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterDeleteQueue
//
// This script queue will remove each script.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterDeleteQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a not run queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterDeleteQueue* create();

protected:
	LLFloaterDeleteQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterDeleteQueue();

	static bool deleteObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump);

	bool startQueue() override;
};
#endif // LL_LLCOMPILEQUEUE_H
