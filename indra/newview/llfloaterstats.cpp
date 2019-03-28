/**
 * @file llfloaterstats.cpp
 * @brief Container for statistics view
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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



#include "llviewerprecompiledheaders.h"

#include "llfloaterstats.h"
#include "llcontainerview.h"
#include "llfloater.h"
#include "llstatview.h"
#include "llscrollcontainer.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "pipeline.h"
#include "llviewerobjectlist.h"
#include "llviewertexturelist.h"
#include "lltexturefetch.h"
#include "sgmemstat.h"

const S32 LL_SCROLL_BORDER = 1;

void LLFloaterStats::buildStats()
{
	LLRect rect;
	//
	// Viewer advanced stats
	//
	LLStatView *stat_viewp = NULL;

	//
	// Viewer Basic
	//
	LLStatView::Params params;
	params.name("basic stat view");
	params.show_label(true);
	params.label("Basic");
	params.setting("OpenDebugStatBasic");
	params.rect(rect);

	stat_viewp = LLUICtrlFactory::create<LLStatView>(params);
	addStatView(stat_viewp);

	LLStatBar::Params p;
	p.label("FPS").name("FPS");
	p.stat(LLStatViewer::FPS.getName());
	p.setting("DebugStatModeFPS").show_bar(true).show_history(true);
	p.bar_max(0.f);
	p.bar_max(60.f);
	p.unit_label("fps");
	p.tick_spacing(3.f);
	p.decimal_digits(1);
	//stat_barp->mLabelSpacing = 15.f;
	stat_viewp->addStat(p);

	p.label("UDP Bandwidth").name("Bandwidth");
	p.stat(LLStatViewer::ACTIVE_MESSAGE_DATA_RECEIVED.getName()).setting("DebugStatModeBandwidth").show_history(false);
	p.unit_label("kbps");
	p.bar_max(900.f);
	p.tick_spacing(100.f).decimal_digits(0);
	//stat_barp->mLabelSpacing = 300.f;
	stat_viewp->addStat(p);

	p.label("Packet Loss").name("Packet Loss");
	p.stat(LLStatViewer::PACKETS_LOST.getName()).setting("DebugStatModePacketLoss").show_bar(false);
	p.unit_label("%").bar_max(5.f).tick_spacing(1.f).decimal_digits(1);
	stat_viewp->addStat(p);

	p.label("Ping Sim").name("Ping Sim");
	p.stat(LLStatViewer::SIM_PING.getName()).setting("DebugStatMode").show_bar(false);
	p.unit_label("msec").bar_max(1000.f).tick_spacing(100.f).decimal_digits(0);
	stat_viewp->addStat(p);

	if (auto malloc_stat = SGMemStat::getStat()) {
		p.label("Allocated memory").name("Allocated memory");
		p.stat(malloc_stat->getName()).setting("DebugStatModeMalloc");
		p.unit_label("MB");
		p.bar_max(2048.f);
		p.tick_spacing(128.f);
		stat_viewp->addStat(p);
	}

	params.name("advanced stat view");
	params.show_label(true);
	params.label("Advanced");
	params.setting("OpenDebugStatAdvanced");
	params.rect(rect);
	stat_viewp = LLUICtrlFactory::create<LLStatView>(params);
	addStatView(stat_viewp);

	params.name("render stat view");
	params.show_label(true);
	params.label("Render");
	params.setting("OpenDebugStatRender");
	params.rect(rect);
	LLStatView *render_statviewp = stat_viewp->addStatView(params);

	p.label("KTris Drawn").name("KTris Drawn");
	p.stat(LLStatViewer::TRIANGLES_DRAWN_PER_FRAME.getName()).setting("DebugStatModeKTrisDrawnFr");
	p.unit_label("/fr").bar_max(3000.f).tick_spacing(100.f).decimal_digits(1);
	render_statviewp->addStat(p);

	p.stat(LLStatViewer::TRIANGLES_DRAWN.getName()).setting("DebugStatModeKTrisDrawnSec");
	p.unit_label("/sec").bar_max(100000.f).tick_spacing(4000.f);
	render_statviewp->addStat(p);

	p.label("Total Objs").name("Total Objs");
	p.stat(LLStatViewer::NUM_OBJECTS.getName()).setting("DebugStatModeTotalObjs");
	p.bar_max(10000.f).tick_spacing(2500.f).decimal_digits(0).unit_label(LLStringUtil::null);
	render_statviewp->addStat(p);

	p.label("New Objs").name("New Objs");
	p.stat(LLStatViewer::NUM_NEW_OBJECTS.getName()).setting("DebugStatModeNewObjs");
	p.bar_max(1000.f).tick_spacing(100.f).unit_label("/sec");
	render_statviewp->addStat(p);

	p.label("Object Cache Hit Rate").name("Object Cache Hit Rate");
	p.stat(LLStatViewer::OBJECT_CACHE_HIT_RATE.getName()).setting(LLStringUtil::null);
	p.show_history(true).unit_label("%").tick_spacing(20.f).bar_max(100.f);
	render_statviewp->addStat(p);

	// Texture statistics
	params.name("texture stat view");
	params.show_label(true);
	params.label("Texture");
	params.setting("OpenDebugStatTexture");
	params.rect(rect);
	LLStatView *texture_statviewp = render_statviewp->addStatView(params);

	p.label("Cache Hit Rate").name("Cache Hit Rate");
	p.stat(LLTextureFetch::sCacheHitRate.getName());
	texture_statviewp->addStat(p);

	p.label("Cache Read Latency").name("Cache Read Latency");
	p.stat(LLTextureFetch::sCacheReadLatency.getName()).unit_label("msec").bar_max(1000.f).tick_spacing(100.f);
	texture_statviewp->addStat(p);

	p.label("Count").name("Count");
	p.stat(LLStatViewer::NUM_IMAGES.getName()).setting("DebugStatModeTextureCount").show_history(false);
	p.unit_label(LLStringUtil::null).bar_max(8000.f).tick_spacing(2000.f);
	texture_statviewp->addStat(p);

	p.label("Raw Count").name("Raw Count");
	p.stat(LLStatViewer::NUM_RAW_IMAGES.getName()).setting("DebugStatModeRawCount");
	texture_statviewp->addStat(p);

	p.label("GL Mem").name("GL Mem");
	p.bar_max(400.f).tick_spacing(100.f).stat(LLStatViewer::GL_TEX_MEM.getName()).setting("DebugStatModeGLMem")
		.decimal_digits(1);
	texture_statviewp->addStat(p);

	p.label("GL Mem").name("GL Mem");
	p.stat(LLStatViewer::FORMATTED_MEM.getName()).setting("DebugStatModeFormattedMem");
	texture_statviewp->addStat(p);

	p.label("Raw Mem").name("Raw Mem");
	p.stat(LLStatViewer::RAW_MEM.getName()).setting("DebugStatModeRawMem");
	texture_statviewp->addStat(p);

	p.label("Bound Mem").name("Bound Mem");
	p.stat(LLStatViewer::GL_BOUND_MEM.getName()).setting("DebugStatModeBoundMem");
	texture_statviewp->addStat(p);

	// Network statistics
	params.name("network stat view");
	params.show_label(true);
	params.label("Network");
	params.setting("OpenDebugStatNet");
	params.rect(rect);
	LLStatView *net_statviewp = stat_viewp->addStatView(params);

	p = LLStatBar::Params();
	p.label("UDP Packets In").name("UDP Packets In");
	p.stat(LLStatViewer::PACKETS_IN.getName()).setting("DebugStatModePacketsIn")
		.unit_label("/sec");
	net_statviewp->addStat(p);

	p.label("UDP Packets Out").name("UDP Packets Out");
	p.stat(LLStatViewer::PACKETS_OUT.getName()).setting("DebugStatModePacketsOut");
	net_statviewp->addStat(p);

	p.label("UDP Textures").name("UDP Textures");
	p.bar_min(0.f).bar_max(1024.f).tick_spacing(128.f)
	.unit_label("kbps").stat(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED.getName()).setting("DebugStatModeUDPTexture");
	net_statviewp->addStat(p);

	p.label("Objects (UDP)").name("Objects (UDP)");
	p.stat(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED.getName()).setting("DebugStatModeObjects");
	net_statviewp->addStat(p);

	p.label("Assets (UDP)").name("Assets (UDP)");
	p.stat(LLStatViewer::ASSET_UDP_DATA_RECEIVED.getName()).setting("DebugStatModeAsset");
	net_statviewp->addStat(p);

	p.label("Layers (UDP)").name("Layers (UDP)");
	p.stat(LLStatViewer::LAYERS_NETWORK_DATA_RECEIVED.getName()).setting("DebugStatModeLayers");
	net_statviewp->addStat(p);

	p.label("Actual In (UDP)").name("Actual In (UDP)");
	p.stat(LLStatViewer::MESSAGE_SYSTEM_DATA_IN.getName()).setting("DebugStatModeActualIn");
	p.show_bar(true);
	net_statviewp->addStat(p);

	p.label("Actual Out (UDP)").name("Actual Out (UDP)");
	p.stat(LLStatViewer::MESSAGE_SYSTEM_DATA_OUT.getName()).setting("DebugStatModeActualOut");
	net_statviewp->addStat(p);

	p.label("VFS Pending Ops").name("VFS Pending Ops");
	p.stat(LLStatViewer::PENDING_VFS_OPERATIONS.getName()).setting("DebugStatModeVFSPendingOps").show_bar(false).unit_label(LLStringUtil::null);
	net_statviewp->addStat(p);


	// Simulator stats
	params.name("sim stat view");
	params.show_label(true);
	params.label("Simulator");
	params.setting("OpenDebugStatSim");
	params.rect(rect);
	LLStatView *sim_statviewp = LLUICtrlFactory::create<LLStatView>(params);
	addStatView(sim_statviewp);

	p.label("Time Dilation").name("Time Dilation");
	p.stat(LLStatViewer::SIM_TIME_DILATION.getName()).setting("DebugStatModeTimeDialation");
	p.decimal_digits(2).tick_spacing(0.25f).bar_max(1.f);
	sim_statviewp->addStat(p);

	p.label("Sim FPS").name("Sim FPS");
	p.decimal_digits(0).stat(LLStatViewer::SIM_FPS.getName()).setting("DebugStatModeSimFPS");
	p.tick_spacing(20.f).bar_max(200.f);
	sim_statviewp->addStat(p);

	p.label("Physics FPS").name("Physics FPS");
	p.decimal_digits(0).stat(LLStatViewer::SIM_PHYSICS_FPS.getName()).setting("DebugStatModePhysicsFPS");
	p.tick_spacing(33.f).bar_max(66.f);
	sim_statviewp->addStat(p);

	params.name("phys detail view");
	params.show_label(true);
	params.label("Physics Details");
	params.setting("OpenDebugStatPhysicsDetails");
	params.rect(rect);
	LLStatView *phys_details_viewp = sim_statviewp->addStatView(params);

	p.label("Pinned Objects").name("Pinned Objects");
	p.stat(LLStatViewer::SIM_PHYSICS_PINNED_TASKS.getName()).setting("DebugStatModePinnedObjects");
	p.bar_max(500.f).tick_spacing(10.f);
	phys_details_viewp->addStat(p);

	p.label("Low LOD Objects").name("Low LOD Objects");
	p.stat(LLStatViewer::SIM_PHYSICS_LOD_TASKS.getName()).setting("DebugStatModeLowLODObjects");
	phys_details_viewp->addStat(p);

	p.label("Memory Allocated").name("Memory Allocated");
	p.stat(LLStatViewer::SIM_PHYSICS_MEM.getName()).setting("DebugStatModeMemoryAllocated");
	p.unit_label("MB").bar_max(1024.f).tick_spacing(128.f);
	phys_details_viewp->addStat(p);

	p.label("Agent Updates/Sec").name("Agent Updates/Sec");
	p.stat(LLStatViewer::SIM_AGENT_UPS.getName()).setting("DebugStatModeAgentUpdatesSec");
	p.unit_label(LLStringUtil::null).bar_max(100.f).tick_spacing(25.f).decimal_digits(1);
	phys_details_viewp->addStat(p);

	p.label("Main Agents").name("Main Agents");
	p.stat(LLStatViewer::SIM_MAIN_AGENTS.getName()).setting("DebugStatModeMainAgents");
	p.bar_max(80.f).tick_spacing(10.f).decimal_digits(0);
	phys_details_viewp->addStat(p);

	p.label("Child Agents").name("Child Agents");
	p.stat(LLStatViewer::SIM_CHILD_AGENTS.getName()).setting("DebugStatModeChildAgents");
	p.bar_max(40.f).tick_spacing(5.f);
	phys_details_viewp->addStat(p);

	p.label("Objects").name("Objects");
	p.stat(LLStatViewer::SIM_OBJECTS.getName()).setting("DebugStatModeSimObjects");
	p.bar_max(30000.f).tick_spacing(5000.f);
	phys_details_viewp->addStat(p);

	p.label("Active Objects").name("Active Objects");
	p.stat(LLStatViewer::SIM_ACTIVE_OBJECTS.getName()).setting("DebugStatModeSimActiveObjects");
	p.bar_max(800.f).tick_spacing(100.f);
	phys_details_viewp->addStat(p);

	p.label("Active Scripts").name("Active Scripts");
	p.stat(LLStatViewer::SIM_ACTIVE_SCRIPTS.getName()).setting("DebugStatModeSimActiveScripts");
	p.bar_max(800.f).tick_spacing(100.f);
	phys_details_viewp->addStat(p);

	p.label("Scripts Run").name("Scripts Run");
	p.stat(LLStatViewer::SIM_PERCENTAGE_SCRIPTS_RUN.getName()).setting(LLStringUtil::null).show_history(true);
	p.bar_max(100.f).tick_spacing(10.f).unit_label("%").decimal_digits(3);
	phys_details_viewp->addStat(p);

	p.label("Scripts Events").name("Scripts Events");
	p.stat(LLStatViewer::SIM_SCRIPT_EPS.getName()).setting("DebugStatModeSimScriptEvents").show_history(false);
	p.bar_max(20000.f).tick_spacing(2500.f).unit_label("eps").decimal_digits(0);
	phys_details_viewp->addStat(p);

	params.name("pathfinding view");
	params.show_label(true);
	params.label("Pathfinding Details");
	params.rect(rect);
	LLStatView *pathfinding_viewp = sim_statviewp->addStatView(params);

	p.label("AI Step Time").name("AI Step Time");
	p.stat(LLStatViewer::SIM_AI_TIME.getName()).setting(LLStringUtil::null);
	p.bar_max(40.f).tick_spacing(10.f).unit_label("ms").decimal_digits(3);
	pathfinding_viewp->addStat(p);

	p.label("Skipped Silhouette Steps").name("Skipped Silhouette Steps");
	p.stat(LLStatViewer::SIM_SKIPPED_SILHOUETTE.getName());
	p.bar_max(45.f).tick_spacing(4.f).unit_label("/sec").decimal_digits(0);
	pathfinding_viewp->addStat(p);

	p.label("Characters Updated").name("Characters Updated");
	p.stat(LLStatViewer::SIM_SKIPPED_CHARACTERS_PERCENTAGE.getName());
	p.bar_max(100.f).tick_spacing(10.f).unit_label("$").decimal_digits(1);
	pathfinding_viewp->addStat(p);

	p.label("Packets In").name("Packets In");
	p.stat(LLStatViewer::SIM_IN_PACKETS_PER_SEC.getName()).setting("DebugStatModeSimInPPS");
	p.bar_max(2000.f).tick_spacing(250.f).unit_label("pps").decimal_digits(0);
	pathfinding_viewp->addStat(p);

	p.label("Packets Out").name("Packets Out");
	p.stat(LLStatViewer::SIM_OUT_PACKETS_PER_SEC.getName()).setting("DebugStatModeSimOutPPS");
	pathfinding_viewp->addStat(p);

	p.label("Pending Downloads").name("Pending Downloads");
	p.stat(LLStatViewer::SIM_PENDING_DOWNLOADS.getName()).setting("DebugStatModeSimPendingDownloads");
	p.bar_max(800.f).tick_spacing(100.f).unit_label(LLStringUtil::null);
	pathfinding_viewp->addStat(p);

	p.label("Pending Uploads").name("Pending Uploads");
	p.stat(LLStatViewer::SIM_PENDING_UPLOADS.getName()).setting("SimPendingUploads");
	p.bar_max(100.f).tick_spacing(25.f);
	pathfinding_viewp->addStat(p);

	p.label("Total Unacked Bytes").name("Total Unacked Bytes");
	p.stat(LLStatViewer::SIM_UNACKED_BYTES.getName()).setting("DebugStatModeSimTotalUnackedBytes");
	p.bar_max(100000.f).tick_spacing(25000.f).unit_label("kb");
	pathfinding_viewp->addStat(p);

	params.name("sim perf view");
	params.show_label(true);
	params.label("Time (ms)");
	params.setting("OpenDebugStatSimTime");
	params.rect(rect);
	LLStatView *sim_time_viewp = sim_statviewp->addStatView(params);

	p.label("Total Frame Time").name("Total Frame Time");
	p.stat(LLStatViewer::SIM_FRAME_TIME.getName()).setting("DebugStatModeSimFrameMsec");
	p.bar_max(40.f).tick_spacing(10.f).unit_label("ms").decimal_digits(1);
	sim_time_viewp->addStat(p);

	p.label("Net Time").name("Net Time");
	p.stat(LLStatViewer::SIM_NET_TIME.getName()).setting("DebugStatModeSimNetMsec");
	sim_time_viewp->addStat(p);

	p.label("Physics Time").name("Physics Time");
	p.stat(LLStatViewer::SIM_PHYSICS_TIME.getName()).setting("DebugStatModeSimSimPhysicsMsec");
	sim_time_viewp->addStat(p);

	p.label("Simulation Time").name("Simulation Time");
	p.stat(LLStatViewer::SIM_OTHER_TIME.getName()).setting("DebugStatModeSimSimOtherMsec");
	sim_time_viewp->addStat(p);

	p.label("Agent Time").name("Agent Time");
	p.stat(LLStatViewer::SIM_AGENTS_TIME.getName()).setting("DebugStatModeSimAgentMsec");
	sim_time_viewp->addStat(p);

	p.label("Images Time").name("Images Time");
	p.stat(LLStatViewer::SIM_IMAGES_TIME.getName()).setting("DebugStatModeSimImagesMsec");
	sim_time_viewp->addStat(p);

	p.label("Script Time").name("Script Time");
	p.stat(LLStatViewer::SIM_SCRIPTS_TIME.getName()).setting("DebugStatModeSimScriptMsec");
	sim_time_viewp->addStat(p);

	p.label("Spare Time").name("Spare Time");
	p.stat(LLStatViewer::SIM_SPARE_TIME.getName()).setting("DebugStatModeSimSpareMsec");
	sim_time_viewp->addStat(p);

	
	// 2nd level time blocks under 'Details' second
	params.name("sim perf view");
	params.show_label(true);
	params.label("Time (ms)");
	params.setting("OpenDebugStatSimTimeDetails");
	params.rect(rect);
	LLStatView *detailed_time_viewp = sim_time_viewp->addStatView(params);
	{
		p.label("  Physics Step").name("  Physics Step");
		p.stat(LLStatViewer::SIM_PHYSICS_STEP_TIME.getName()).setting("DebugStatModeSimSimPhysicsStepMsec");
		detailed_time_viewp->addStat(p);

		p.label("  Update Physics Shapes").name("  Update Physics Shapes");
		p.stat(LLStatViewer::SIM_PHYSICS_SHAPE_UPDATE_TIME.getName()).setting("DebugStatModeSimSimPhysicsShapeUpdateMsec");
		detailed_time_viewp->addStat(p);

		p.label("  Physics Other").name("  Physics Other");
		p.stat(LLStatViewer::SIM_PHYSICS_OTHER_TIME.getName()).setting("DebugStatModeSimSimPhysicsOtherMsec");
		detailed_time_viewp->addStat(p);

		p.label("  Sleep Time").name("  Sleep Time");
		p.stat(LLStatViewer::SIM_SLEEP_TIME.getName()).setting("DebugStatModeSimSleepMsec");
		detailed_time_viewp->addStat(p);

		p.label("  Pump IO").name("  Pump IO");
		p.stat(LLStatViewer::SIM_PUMP_IO_TIME.getName()).setting("DebugStatModeSimPumpIOMsec");
		detailed_time_viewp->addStat(p);
	}

	LLRect r = getRect();

	// Reshape based on the parameters we set.
	reshape(r.getWidth(), r.getHeight());
}


LLFloaterStats::LLFloaterStats(const LLSD& val)
	:   LLFloater("floater_stats"),
		mStatsContainer(NULL),
		mScrollContainer(NULL)

{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_statistics.xml", NULL, FALSE);
	
	LLRect stats_rect(0, getRect().getHeight() - LLFLOATER_HEADER_SIZE,
					  getRect().getWidth() - LLFLOATER_CLOSE_BOX_SIZE, 0);

	LLContainerView::Params rvp;
	rvp.name("statistics_view");
	rvp.rect(stats_rect);
	mStatsContainer = LLUICtrlFactory::create<LLContainerView>(rvp);

	LLRect scroll_rect(LL_SCROLL_BORDER, getRect().getHeight() - LLFLOATER_HEADER_SIZE - LL_SCROLL_BORDER,
					   getRect().getWidth() - LL_SCROLL_BORDER, LL_SCROLL_BORDER);
		mScrollContainer = new LLScrollContainer(std::string("statistics_scroll"), scroll_rect, mStatsContainer);
	mScrollContainer->setFollowsAll();
	mScrollContainer->setReserveScrollCorner(TRUE);

	mStatsContainer->setScrollContainer(mScrollContainer);
	
	addChild(mScrollContainer);

	buildStats();
}


LLFloaterStats::~LLFloaterStats()
{
}

void LLFloaterStats::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if (mStatsContainer)
	{
		LLRect rect = mStatsContainer->getRect();

		mStatsContainer->reshape(rect.getWidth() - 2, rect.getHeight(), TRUE);
	}

	LLFloater::reshape(width, height, called_from_parent);
}


void LLFloaterStats::addStatView(LLStatView* stat)
{
	mStatsContainer->addChildInBack(stat);
}

// virtual
void LLFloaterStats::onOpen()
{
	LLFloater::onOpen();
	gSavedSettings.setBOOL("ShowDebugStats", TRUE);
	reshape(getRect().getWidth(), getRect().getHeight());
}

void LLFloaterStats::onClose(bool app_quitting)
{
	setVisible(FALSE);
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowDebugStats", FALSE);
	}
}
