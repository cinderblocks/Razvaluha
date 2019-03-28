/** 
 * @file llfloaterabout.cpp
 * @author James Cook
 * @brief The about box from Help->About
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

#include "llfloaterabout.h"

#include "llsys.h"
#include "llgl.h"
#include "llui.h"	// for tr()
#include "v3dmath.h"

#include "llcoros.h"
#include "llimagej2c.h"
#include "llaudioengine.h"

#include "llviewertexteditor.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llviewerstats.h"
#include "llviewerregion.h"
#include "llversioninfo.h"
#include "lluictrlfactory.h"
#include "lluri.h"
#include "llweb.h"
#include "lltrans.h"
#include "llappviewer.h" 
#include "llglheaders.h"
#include "llwindow.h"

#include "hippogridmanager.h"

// [RLVa:KB] - Checked: 2010-04-18 (RLVa-1.4.0)
#include "rlvactions.h"
#include "rlvhelper.h"
// [/RLVa:KB]

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

#include "cef/dullahan.h"
#if VLCPLUGIN
#include "vlc/libvlc_version.h"
#endif // LL_WINDOWS

extern LLMemoryInfo gSysMemory;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

LLFloaterAbout* LLFloaterAbout::sInstance = NULL;

static std::string get_viewer_release_notes_url();

static void onAboutClickCopyToClipboard(void* user_data)
{
	LLFloater* self = (LLFloater*) user_data;
	LLViewerTextEditor *support_widget = 
		self->getChild<LLViewerTextEditor>("support_editor", true);
	support_widget->selectAll();
	support_widget->copy();
	support_widget->deselect();
}

///----------------------------------------------------------------------------
/// Class LLFloaterAbout
///----------------------------------------------------------------------------

// Default constructor
LLFloaterAbout::LLFloaterAbout() 
:	LLFloater(std::string("floater_about"), std::string("FloaterAboutRect"), LLStringUtil::null)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_about.xml");

	// Support for changing product name.
	std::string title("About ");
	title += LLAppViewer::instance()->getSecondLifeTitle();
	setTitle(title);

	LLViewerTextEditor *support_widget = 
		getChild<LLViewerTextEditor>("support_editor", true);

	LLViewerTextEditor *credits_widget = 
		getChild<LLViewerTextEditor>("credits_editor", true);

	LLViewerTextEditor *licenses_widget =
		getChild<LLViewerTextEditor>("licenses_editor", true);
	
	childSetAction("copy_btn", onAboutClickCopyToClipboard, this);

	if (!support_widget || !credits_widget)
	{
		return;
	}

	// For some reason, adding style doesn't work unless this is true.
	support_widget->setParseHTML(TRUE);

	// Text styles for release notes hyperlinks
	LLStyleSP viewer_link_style(new LLStyle);
	viewer_link_style->setVisible(true);
	viewer_link_style->setFontName(LLStringUtil::null);
	viewer_link_style->setLinkHREF(get_viewer_release_notes_url());
	viewer_link_style->setColor(gSavedSettings.getColor4("HTMLLinkColor"));

	// Version string
	std::string version = std::string(LLAppViewer::instance()->getSecondLifeTitle()
#if defined(_WIN64) || defined(__x86_64__)
		+ " (64 bit)"
#endif
		+ llformat(" %d.%d.%d (%d) %s %s (%s)\n",
		LLVersionInfo::getMajor(), LLVersionInfo::getMinor(), LLVersionInfo::getPatch(), LLVersionInfo::getBuild(),
		__DATE__, __TIME__,
		LLVersionInfo::getChannel().c_str()));
	support_widget->appendColoredText(version, FALSE, FALSE, gColors.getColor("TextFgReadOnlyColor"));
	support_widget->appendText(LLTrans::getString("ReleaseNotes"), false, false, viewer_link_style);
	support_widget->appendColoredText("\nThis is an early Experience Tools Support Build, experience ui may fail or crash.", false, false, LLColor4::orange);

	std::stringstream support;
	support << "\n\n"
			<< "Grid: " << gHippoGridManager->getConnectedGrid()->getGridName()
			<< "\n\n";

#if LL_MSVC
    support << llformat("Built with MSVC version %d\n\n", _MSC_VER);
#endif

#if LL_CLANG
    support << llformat("Built with Clang version %d\n\n", CLANG_VERSION);
#endif

#if LL_INTELC
    support << llformat("Built with ICC version %d\n\n", __ICC);
#endif

#if LL_GNUC
    support << llformat("Built with GCC version %d\n\n", GCC_VERSION);
#endif

	// Position
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
// [RLVa:KB] - Checked: 2014-02-24 (RLVa-1.4.10)
		if (RlvActions::canShowLocation())
		{
		const LLVector3d &pos = gAgent.getPositionGlobal();
		LLUIString pos_text = getString("you_are_at");
		pos_text.setArg("[POSITION]",
						llformat("%.1f, %.1f, %.1f ", pos.mdV[VX], pos.mdV[VY], pos.mdV[VZ]));
		support << pos_text.getString();

		if (const LLViewerRegion* region = gAgent.getRegion())
		{
			const LLVector3d& coords(region->getOriginGlobal());
			support << llformat("in %s (%.0f, %.0f) located at ", region->getName().c_str(), coords.mdV[VX]/REGION_WIDTH_METERS, coords.mdV[VY]/REGION_WIDTH_METERS)
					<< region->getHost().getHostName()
					<< " (" << region->getHost().getString() << ')';
		}
		}
		else
			support << RlvStrings::getString(RLV_STRING_HIDDEN_REGION);
// [/RLVa:KN]
		support << '\n';

		support << gLastVersionChannel
				<< '\n';

		const std::string url(region->getCapability("ServerReleaseNotes"));
		if (!url.empty())
		{
			support << '[' << url << ' ' << LLTrans::getString("ReleaseNotes") << ']';
		}

		support << "\n\n";
	}

	// *NOTE: Do not translate text like GPU, Graphics Card, etc -
	//  Most PC users that know what these mean will be used to the english versions,
	//  and this info sometimes gets sent to support
	
	// CPU
	support << "CPU: "
			<< gSysCPU.getCPUString()
			<< '\n';

	U32Megabytes memory = gSysMemory.getPhysicalMemoryKB();
	// Moved hack adjustment to Windows memory size into llsys.cpp

	support << llformat("Memory: %u MB\n", memory)

			<< ("OS Version: ")
			<< LLAppViewer::instance()->getOSInfo().getOSString()
			<< '\n'

			<< "Graphics Card Vendor: "
			<< (const char*) glGetString(GL_VENDOR)
			<< '\n'

			<< "Graphics Card: "
			<< (const char*) glGetString(GL_RENDERER)
			<< '\n';

#if LL_WINDOWS
    getWindow()->incBusyCount();
    getWindow()->setCursor(UI_CURSOR_ARROW);
	support << "Windows Graphics Driver Version: ";
    LLSD driver_info = gDXHardware.getDisplayInfo();
    if (driver_info.has("DriverVersion"))
    {
        support << driver_info["DriverVersion"];
    }
    support << '\n';
    getWindow()->decBusyCount();
    getWindow()->setCursor(UI_CURSOR_ARROW);
#endif

	support << "OpenGL Version: "
			<< (const char*)glGetString(GL_VERSION);
// [RLVa:KB] - Checked: 2010-04-18 (RLVa-1.2.0)
	if (RlvActions::isRlvEnabled())
		support << "\nRLV Version: " << RlvStrings::getVersionAbout();
// [/RLVa:KB]
	support << "\n\n"

			<< "libcurl Version: "
			<< LLCore::LLHttp::getCURLVersion()
			<< '\n'

			<< "J2C Decoder Version: "
			<< LLImageJ2C::getEngineInfo()
			<< '\n'

			<< "Audio Driver Version: "
			<< (gAudiop ? gAudiop->getDriverName(/*want_fullname = */true) : "(none)")
			<< '\n';

	support << "Dullahan: "
			<< DULLAHAN_VERSION_MAJOR
			<< '.'
			<< DULLAHAN_VERSION_MINOR
			<< '.'
			<< DULLAHAN_VERSION_BUILD

			<< " / CEF: " << CEF_VERSION
			<< " / Chrome: " << CHROME_VERSION_MAJOR
			<< '\n';

#if VLCPLUGIN
	support << "LibVLC: ";
	support << LIBVLC_VERSION_MAJOR;
	support << '.';
	support << LIBVLC_VERSION_MINOR;
	support << '.';
	support << LIBVLC_VERSION_REVISION;
	support << '\n';
#endif

	auto& recording = LLViewerStats::instance().getRecording();
	const auto& packets_in = recording.getSum(LLStatViewer::PACKETS_IN);
	if (packets_in > 0)
	{
		const auto& lost = recording.getSum(LLStatViewer::PACKETS_LOST);
		support << llformat("Packets Lost: %.0f/%.0f (%.1f%%)", 
			lost, packets_in, 100.f*lost / packets_in)
			<< '\n';
	}

	support_widget->appendText(support.str(), false, false);

	// Fix views
	support_widget->setCursorPos(0);
	support_widget->setEnabled(FALSE);
	support_widget->setTakesFocus(TRUE);
	support_widget->setHandleEditKeysDirectly(TRUE);

	credits_widget->setCursorPos(0);
	credits_widget->setEnabled(FALSE);
	credits_widget->setTakesFocus(TRUE);
	credits_widget->setHandleEditKeysDirectly(TRUE);

	// Get the Versions and Copyrights, created at build time
	std::string licenses_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "packages-info.txt");
	llifstream licenses_file;
	licenses_file.open(licenses_path);		/* Flawfinder: ignore */
	if (licenses_file.is_open())
	{
		std::string license_line;
		licenses_widget->clear();
		while (std::getline(licenses_file, license_line))
		{
			licenses_widget->appendColoredText(license_line + '\n', FALSE, FALSE, gColors.getColor("TextFgReadOnlyColor"));
		}
		licenses_file.close();
	}
	else
	{
		// this case will use the (out of date) hard coded value from the XUI
		LL_INFOS("AboutInit") << "Could not read licenses file at " << licenses_path << LL_ENDL;
	}
	licenses_widget->setCursorPos(0);
	licenses_widget->setEnabled(FALSE);
	licenses_widget->setTakesFocus(TRUE);
	licenses_widget->setHandleEditKeysDirectly(TRUE);

	center();

	sInstance = this;
}

// Destroys the object
LLFloaterAbout::~LLFloaterAbout()
{
	sInstance = nullptr;
}

// static
void LLFloaterAbout::show(void*)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterAbout();
	}

	sInstance->open();	 /*Flawfinder: ignore*/
}


static std::string get_viewer_release_notes_url()
{
	return LLTrans::getString("APP_SITE");
	/*std::ostringstream version;
	version <<  gVersionMajor
		<< "." << gVersionMinor
		<< "." << gVersionPatch
		<< "." << gVersionBuild;
	LLSD query;

	query["channel"] = gVersionChannel;

	query["version"] = version.str();

	std::ostringstream url;
	url << RELEASE_NOTES_BASE_URL << LLURI::mapToQueryString(query);

	return "http://ascent.balseraph.org/index.php/Ascent_" + version.str();// url.str();*/
}

