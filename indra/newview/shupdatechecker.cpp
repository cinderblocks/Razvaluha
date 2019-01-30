#include "llviewerprecompiledheaders.h"

#include "llviewerwindow.h"
#include "llwindow.h"
#include "llpanelgeneral.h"
#include "llappviewer.h"
#include "llbutton.h"
#include "llcorehttputil.h"
#include "llviewercontrol.h"
#include "llnotificationsutil.h"
#include "llstartup.h"
#include "llviewerwindow.h"			// to link into child list
#include "llnotify.h"
#include "lluictrlfactory.h"
#include "llversioninfo.h"
#include "llbufferstream.h"
#include "llweb.h"

#include <nlohmann/json.hpp>


void onNotifyButtonPress(const LLSD& notification, const LLSD& response, std::string name, std::string url)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0) // URL
	{
		std::string escaped_url = LLWeb::escapeURL(url);
		if (gViewerWindow)
		{
			gViewerWindow->getWindow()->spawnWebBrowser(escaped_url, true);
		}
	}
	if (option == 1) // Later
	{
	}
}
void onCompleted(const LLSD& data, bool release)
{
	S32 build(LLVersionInfo::getBuild());
	std::string viewer_version = llformat("%s (%i)", LLVersionInfo::getShortVersion().c_str(), build);

#if LL_WINDOWS
	std::string recommended_version = data["recommended"]["windows"].asString();
	std::string minimum_version = data["minimum"]["windows"].asString();
#elif LL_LINUX
	std::string recommended_version = data["recommended"]["linux"].asString();
	std::string minimum_version = data["minimum"]["linux"].asString();
#elif LL_DARWIN
	std::string recommended_version = data["recommended"]["apple"].asString();
	std::string minimum_version = data["minimum"]["apple"].asString();
#endif
		
	S32 minimum_build, recommended_build;
	sscanf(recommended_version.c_str(), "%*i.%*i.%*i (%i)", &recommended_build);
	sscanf(minimum_version.c_str(), "%*i.%*i.%*i (%i)", &minimum_build);

	LL_INFOS("GetUpdateInfoResponder") << build << LL_ENDL;
	LLSD args;
	args["CURRENT_VER"] = viewer_version;
	args["RECOMMENDED_VER"] = recommended_version;
	args["MINIMUM_VER"] = minimum_version;
	args["URL"] = data["url"].asString();
	args["TYPE"] = release ? "Viewer" : "Alpha";

	static LLCachedControl<S32> lastver(release ? "SinguLastKnownReleaseBuild" : "SinguLastKnownAlphaBuild", 0);

	if (build < minimum_build || build < recommended_build)
	{
		if (lastver.get() < recommended_build)
		{
			lastver = recommended_build;
			LLUI::sIgnoresGroup->setWarning("UrgentUpdateModal", true);
			LLUI::sIgnoresGroup->setWarning("UrgentUpdate", true);
			LLUI::sIgnoresGroup->setWarning("RecommendedUpdate", true);
		}
		const std::string&& notification = build < minimum_build ?
			LLUI::sIgnoresGroup->getWarning("UrgentUpdateModal") ? "UrgentUpdateModal" : "UrgentUpdate" :
			"RecommendedUpdate"; //build < recommended_build
		LLNotificationsUtil::add(notification, args, LLSD(), boost::bind(onNotifyButtonPress, _1, _2, notification, data["url"].asString()));
	}	
}

#include "llsdutil.h"
void check_for_updates()
{
#if LL_WINDOWS | LL_LINUX | LL_DARWIN
	// Hard-code the update url for now.
	std::string url = "http://singularity-viewer.github.io/pages/api/get_update_info.json";//gSavedSettings.getString("SHUpdateCheckURL");
	if (!url.empty())
	{
		std::string type;
		auto& channel = LLVersionInfo::getChannel();
		if (channel == "Singularity")
		{
			type = "release";
		}
		else if (channel == "Singularity Test" || channel == "Singularity Alpha")
		{
			type = "alpha";
		}
		else return;
		const std::string&& name = "SHUpdateChecker";
		LLCoros::instance().launch(name, [=] {
			LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter(name, LLCore::HttpRequest::DEFAULT_POLICY_ID));
			LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
			LLSD data = httpAdapter->getJsonAndSuspend(httpRequest, url);
			onCompleted(data[type], type == "release");
		});
	}
#endif
}
