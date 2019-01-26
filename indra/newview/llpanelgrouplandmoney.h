/** 
 * @file llpanelgrouplandmoney.h
 * @brief Panel for group land and L$.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_PANEL_GROUP_LAND_MONEY_H
#define LL_PANEL_GROUP_LAND_MONEY_H

#include "llpanelgroup.h"
#include "lluuid.h"

class LLPanelGroupLandMoney : public LLPanelGroupTab
{
public:
	LLPanelGroupLandMoney(const std::string& name, const LLUUID& group_id);
	virtual ~LLPanelGroupLandMoney();
	BOOL postBuild() override;
	BOOL isVisibleByAgent(LLAgent* agentp) override;

	static void* createTab(void* data);

	void activate() override;
	bool needsApply(std::string& mesg) override;
	bool apply(std::string& mesg) override;
	void cancel() override;
	void update(LLGroupChange gc) override;

	static void processPlacesReply(LLMessageSystem* msg, void**);

	typedef std::map<LLUUID, LLPanelGroupLandMoney*> group_id_map_t;
	static group_id_map_t sGroupIDs;

	static void processGroupAccountDetailsReply(LLMessageSystem* msg,  void** data);
	static void processGroupAccountTransactionsReply(LLMessageSystem* msg, void** data);
	static void processGroupAccountSummaryReply(LLMessageSystem* msg, void** data);

	virtual void onLandSelectionChanged();
	
protected:
	class impl;
	impl* mImplementationp;
};

#endif // LL_PANEL_GROUP_LAND_MONEY_H
