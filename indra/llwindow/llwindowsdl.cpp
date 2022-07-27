/** 
 * @file llwindowsdl.cpp
 * @brief SDL implementation of LLWindow class
 * @author This module has many fathers, and it shows.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#if LL_SDL

#include "linden_common.h"

#include "llwindowsdl.h"

#include "llwindowcallbacks.h"
#include "llkeyboardsdl.h"

#include "llerror.h"
#include "llgl.h"
#include "llglslshader.h"
#include "llstring.h"
#include "lldir.h"
#include "llfindlocale.h"

#include <SDL_misc.h>

#if LL_GTK
extern "C" {
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#if GTK_CHECK_VERSION(2, 24, 0)
#include <gdk/gdkx.h>
#endif
}
#include <clocale>
#endif // LL_GTK

extern "C" {
# include "fontconfig/fontconfig.h"
}

#if LL_LINUX
// not necessarily available on random SDL platforms, so #if LL_LINUX
// for execv(), waitpid(), fork()
# include <unistd.h>
# include <sys/types.h>
# include <sys/wait.h>
#endif // LL_LINUX

extern BOOL gDebugWindowProc;

const S32 MAX_NUM_RESOLUTIONS = 200;

//
// LLWindowSDL
//

#if LL_X11
# include <X11/Xutil.h>
# include <boost/regex.hpp>
#endif //LL_X11

// TOFU HACK -- (*exactly* the same hack as LLWindowMacOSX for a similar
// set of reasons): Stash a pointer to the LLWindowSDL object here and
// maintain in the constructor and destructor.  This assumes that there will
// be only one object of this class at any time.  Currently this is true.
static LLWindowSDL *gWindowImplementation = NULL;

#if LL_GTK
// Lazily initialize and check the runtime GTK version for goodness.
// static
bool LLWindowSDL::ll_try_gtk_init(void)
{
	static BOOL done_gtk_diag = FALSE;
	static BOOL gtk_is_good = FALSE;
	static BOOL done_setlocale = FALSE;
	static BOOL tried_gtk_init = FALSE;

	if (!done_setlocale)
	{
		LL_INFOS() << "Starting GTK Initialization." << LL_ENDL;
		gtk_disable_setlocale();
		done_setlocale = TRUE;
	}
	
	if (!tried_gtk_init)
	{
		tried_gtk_init = TRUE;
		gtk_is_good = gtk_init_check(NULL, NULL);
		if (!gtk_is_good)
			LL_WARNS() << "GTK Initialization failed." << LL_ENDL;
	}

	if (gtk_is_good && !done_gtk_diag)
	{
		LL_INFOS() << "GTK Initialized." << LL_ENDL;
		LL_INFOS() << "- Compiled against GTK version "
			<< GTK_MAJOR_VERSION << "."
			<< GTK_MINOR_VERSION << "."
			<< GTK_MICRO_VERSION << LL_ENDL;
		LL_INFOS() << "- Running against GTK version "
			<< gtk_major_version << "."
			<< gtk_minor_version << "."
			<< gtk_micro_version << LL_ENDL;

		const gchar* gtk_warning = gtk_check_version(
			GTK_MAJOR_VERSION,
			GTK_MINOR_VERSION,
			GTK_MICRO_VERSION);
		if (gtk_warning)
		{
			LL_WARNS() << "- GTK COMPATIBILITY WARNING: " <<
				gtk_warning << LL_ENDL;
			gtk_is_good = FALSE;
		} else {
			LL_INFOS() << "- GTK version is good." << LL_ENDL;
		}

		done_gtk_diag = TRUE;
	}

	return gtk_is_good;
}
#endif // LL_GTK


#if LL_X11
// static
Window LLWindowSDL::get_SDL_XWindowID(void)
{
	if (gWindowImplementation) {
		return gWindowImplementation->mSDL_XWindowID;
	}
	return None;
}

//static
Display* LLWindowSDL::get_SDL_Display(void)
{
	if (gWindowImplementation) {
		return gWindowImplementation->mSDL_Display;
	}
	return NULL;
}
#endif // LL_X11

void sdlLogOutputFunc(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    LLError::ELevel level = LLError::LEVEL_INFO;
    std::string level_str;
    switch (priority) {
        case SDL_LOG_PRIORITY_VERBOSE:
            level = LLError::LEVEL_DEBUG;
            level_str = "VERBOSE";
            break;
        case SDL_LOG_PRIORITY_DEBUG:
            level = LLError::LEVEL_DEBUG;
            level_str = "DEBUG";
            break;
        case SDL_LOG_PRIORITY_INFO:
            level = LLError::LEVEL_INFO;
            level_str = "INFO";
            break;
        case SDL_LOG_PRIORITY_WARN:
            level = LLError::LEVEL_WARN;
            level_str = "WARN";
            break;
        case SDL_LOG_PRIORITY_ERROR:
            level = LLError::LEVEL_WARN;
            level_str = "ERROR";
            break;
        case SDL_LOG_PRIORITY_CRITICAL:
            level = LLError::LEVEL_WARN;
            level_str = "CRITICAL";
            break;
        case SDL_NUM_LOG_PRIORITIES:
            break;
    }

    std::string category_str;
    switch (category)
    {
    case SDL_LOG_CATEGORY_APPLICATION:
        category_str = "Application";
        break;
    case SDL_LOG_CATEGORY_ERROR:
        category_str = "Error";
        break;
    case SDL_LOG_CATEGORY_ASSERT:
        category_str = "Assert";
        break;
    case SDL_LOG_CATEGORY_SYSTEM:
        category_str = "System";
        break;
    case SDL_LOG_CATEGORY_AUDIO:
        category_str = "Audio";
        break;
    case SDL_LOG_CATEGORY_VIDEO:
        category_str = "Video";
        break;
    case SDL_LOG_CATEGORY_RENDER:
        category_str = "Render";
        break;
    case SDL_LOG_CATEGORY_INPUT:
        category_str = "Input";
        break;
    case SDL_LOG_CATEGORY_TEST:
        category_str = "Test";
        break;
    }

    LL_VLOGS(level, "LLWindowSDL") << "SDL [" << level_str << "] <" << category_str << "> : " << message << LL_ENDL;
}

LLWindowSDL::LLWindowSDL(LLWindowCallbacks* callbacks,
			 const std::string& title, S32 x, S32 y, S32 width,
			 S32 height, U32 flags,
			 BOOL fullscreen, BOOL clearBg,
			 BOOL disable_vsync, BOOL use_gl,
			 BOOL ignore_pixel_depth, U32 fsaa_samples)
	: LLWindow(callbacks, fullscreen, flags),
	  mGamma(1.0f)
{
	SDL_SetMainReady();
	SDL_LogSetOutputFunction(&sdlLogOutputFunc, nullptr);

#if LL_X11
	XInitThreads();
#endif

	if (SDL_InitSubSystem(SDL_INIT_EVENTS|SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_JOYSTICK) != 0)
	{
		LL_WARNS() << "Failed to initialize SDL due to error: " << SDL_GetError() << LL_ENDL;
		return;
	}
	
	// Initialize the keyboard
	gKeyboard = new LLKeyboardSDL();
	gKeyboard->setCallbacks(callbacks);
	// Note that we can't set up key-repeat until after SDL has init'd video

	// Ignore use_gl for now, only used for drones on PC
	mWindow = nullptr;
	mGLContext = nullptr;
	mNeedsResize = FALSE;
	mOverrideAspectRatio = 0.f;
	mGrabbyKeyFlags = 0;
	mReallyCapturedCount = 0;
	mFSAASamples = fsaa_samples;
	mPreeditor = nullptr;
	mLanguageTextInputAllowed = false;

#if LL_X11
	mSDL_XWindowID = None;
	mSDL_Display = NULL;
#endif // LL_X11

#if LL_GTK
	// We MUST be the first to initialize GTK so that GTK doesn't get badly
	// initialized with a non-C locale and cause lots of serious random
	// weirdness.
	ll_try_gtk_init();
#endif // LL_GTK

	// Assume 4:3 aspect ratio until we know better
	mOriginalAspectRatio = 1024.0 / 768.0;

	if (title.empty())
		mWindowTitle = "SDL Window";  // *FIX: (?)
	else
		mWindowTitle = title;

	// Create the GL context and set it up for windowed or fullscreen, as appropriate.
	if(createContext(x, y, width, height, 32, fullscreen, disable_vsync))
	{
		gGLManager.initGL();

		//start with arrow cursor
		initCursors();
		setCursor( UI_CURSOR_ARROW );


		allowLanguageTextInput(NULL, FALSE);
	}

	stop_glerror();

	// Stash an object pointer for OSMessageBox()
	gWindowImplementation = this;

	mFlashing = FALSE;

	mKeyScanCode = 0;
	mKeyVirtualKey = 0;
	mKeyModifiers = KMOD_NONE;
}

static SDL_Surface *Load_BMP_Resource(const char *basename)
{
	const int PATH_BUFFER_SIZE=1000;
	char path_buffer[PATH_BUFFER_SIZE];	/* Flawfinder: ignore */
	
	// Figure out where our BMP is living on the disk
	snprintf(path_buffer, PATH_BUFFER_SIZE-1, "%s%sres-sdl%s%s",	
		 gDirUtilp->getAppRODataDir().c_str(),
		 gDirUtilp->getDirDelimiter().c_str(),
		 gDirUtilp->getDirDelimiter().c_str(),
		 basename);
	path_buffer[PATH_BUFFER_SIZE-1] = '\0';
	
	return SDL_LoadBMP(path_buffer);
}

#if LL_X11
// This is an XFree86/XOrg-specific hack for detecting the amount of Video RAM
// on this machine.  It works by searching /var/log/var/log/Xorg.?.log or
// /var/log/XFree86.?.log for a ': (VideoRAM ?|Memory): (%d+) kB' regex, where
// '?' is the X11 display number derived from $DISPLAY
#define X11_VRAM_REGEX "(VRAM|Memory|Video\\s?RAM)\\D*(\\d+)\\s?([kK]B?)"

static int x11_detect_VRAM_kb_fp(FILE *fp) 
{
	boost::regex pattern(X11_VRAM_REGEX);
	if(fp)
	{
		char * line = NULL;
		size_t len = 0;
		ssize_t read;
		while((read = getline(&line, &len, fp)) != -1) {
			std::string sline(line, len);
			boost::cmatch match;
			if(boost::regex_search(line, match, pattern)) {
				long int vram = strtol(std::string(match[2].str()).c_str(), NULL, 10);
				LL_INFOS() << "Found VRAM " << vram << LL_ENDL;
                if (line)
                    free(line);
				return (int)vram;
			}
		}
		if (line)
            free(line);
	}
	return 0; // not detected
}

static int x11_detect_VRAM_kb()
{
	const std::string x_log_location("/var/log/");
	int rtn = 0; // 'could not detect'
	FILE *fp;

	// *TODO: we could be smarter and see which of Xorg/XFree86 has the
	// freshest time-stamp.

	// try journald first
	fp = popen("journalctl -e _COMM=Xorg{,.bin}", "r");
	if (fp)
	{
		LL_INFOS() << "Looking in journald for VRAM info..." << LL_ENDL;
		rtn = x11_detect_VRAM_kb_fp(fp);
		pclose(fp);
	} else {
		LL_INFOS() << "Failed to popen journalctl (expected on non-systemd)" << LL_ENDL;
	}

	if (rtn == 0)
	{
		// Try Xorg log last
		// XXX: this will not work on system that output Xorg logs to syslog
		std::string fname;
		int display_num = 0;
		char *display_env = getenv("DISPLAY"); // e.g. :0 or :0.0 or :1.0 etc
		// parse DISPLAY number so we can go grab the right log file
		if (display_env[0] == ':' &&
			display_env[1] >= '0' && display_env[1] <= '9')
		{
			display_num = display_env[1] - '0';
		}

		fname = getenv("HOME");
		fname += "/.local/share/xorg/Xorg.";
		fname += ('0' + display_num);
		fname += ".log";
		fp = fopen(fname.c_str(), "r");
		if (fp)
		{
			LL_INFOS() << "Looking in " << fname
				<< " for VRAM info..." << LL_ENDL;
			rtn = x11_detect_VRAM_kb_fp(fp);
			fclose(fp);
		}
		else
		{
			fname = x_log_location;
			fname += "Xorg.";
			fname += ('0' + display_num);
			fname += ".log";
			fp = fopen(fname.c_str(), "r");
			if (fp)
			{
				LL_INFOS() << "Looking in " << fname
					<< " for VRAM info..." << LL_ENDL;
				rtn = x11_detect_VRAM_kb_fp(fp);
				fclose(fp);
			}
			else
			{
				LL_INFOS() << "Could not open " << fname
					<< " - skipped." << LL_ENDL;
				// Try old XFree86 log otherwise
				fname = x_log_location;
				fname += "XFree86.";
				fname += ('0' + display_num);
				fname += ".log";
				fp = fopen(fname.c_str(), "r");
				if (fp)
				{
					LL_INFOS() << "Looking in " << fname
						<< " for VRAM info..." << LL_ENDL;
					rtn = x11_detect_VRAM_kb_fp(fp);
					fclose(fp);
				}
				else
				{
					LL_INFOS() << "Could not open " << fname
						<< " - skipped." << LL_ENDL;
				}
			}
		}
	}
	return rtn;
}
#endif // LL_X11

BOOL LLWindowSDL::createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync)
{
	//bool			glneedsinit = false;

	LL_INFOS() << "createContext, fullscreen=" << fullscreen <<
	    " size=" << width << "x" << height << LL_ENDL;

	// captures don't survive contexts
	mGrabbyKeyFlags = 0;
	mReallyCapturedCount = 0;

	SDL_version c_sdl_version;
	SDL_VERSION(&c_sdl_version);
	LL_INFOS() << "Compiled against SDL "
		<< int(c_sdl_version.major) << "."
		<< int(c_sdl_version.minor) << "."
		<< int(c_sdl_version.patch) << LL_ENDL;
	SDL_version r_sdl_version;
	SDL_GetVersion(&r_sdl_version);
	LL_INFOS() << " Running against SDL "
		<< int(r_sdl_version.major) << "."
		<< int(r_sdl_version.minor) << "."
		<< int(r_sdl_version.patch) << LL_ENDL;

	SDL_DisplayMode current_mode;
	if (SDL_GetCurrentDisplayMode(0, &current_mode) != 0)
	{
		LL_INFOS() << "SDL_GetCurrentDisplayMode failed! " << SDL_GetError() << LL_ENDL;
		setupFailure("SDL_GetCurrentDisplayMode failed!", "Error", OSMB_OK);
		return FALSE;
	}

	if (current_mode.h > 0)
	{
		mOriginalAspectRatio = (double) current_mode.w / (double) current_mode.h;
		LL_INFOS() << "Original aspect ratio was " << current_mode.w << ":" << current_mode.h << "=" << mOriginalAspectRatio << LL_ENDL;

	}

	// Setup default window flags
   	mSDLFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	// Setup default backing colors
    GLint redBits{8}, greenBits{8}, blueBits{8}, alphaBits{8};
	
	GLint depthBits{24}, stencilBits{8};

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, redBits);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, greenBits);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blueBits);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alphaBits);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBits);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if (mFSAASamples > 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, mFSAASamples);
	}

	if (LLRender::sGLCoreProfile)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		LLGLSLShader::sNoFixedFunction = true;
	}

	U32 context_flags = 0;
	if (gDebugGL)
	{
		context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);

	if (mWindow == nullptr)
	{
		if (width == 0)
			width = 1024;
		if (height == 0)
			height = 768;

		if (x == 0)
			x = SDL_WINDOWPOS_UNDEFINED;
		if (y == 0)
			y = SDL_WINDOWPOS_UNDEFINED;

		LL_INFOS() << "Creating window " << width << "x" << height << "x" << bits << LL_ENDL;
		mWindow = SDL_CreateWindow(mWindowTitle.c_str(), x, y, width, height, mSDLFlags);
		if (mWindow == nullptr)
		{
			LL_WARNS() << "Window creation failure. SDL: " << SDL_GetError() << LL_ENDL;
			setupFailure("Window creation error", "Error", OSMB_OK);
            return FALSE;
        }
        // Set the application icon.
        SDL_Surface* bmpsurface = Load_BMP_Resource("ll_icon.BMP");
        if (bmpsurface)
        {
            // This attempts to give a black-keyed mask to the icon.
            SDL_SetColorKey(bmpsurface, SDL_TRUE, SDL_MapRGB(bmpsurface->format, 255, 0, 246));
            SDL_SetWindowIcon(mWindow, bmpsurface);
            // The SDL examples cheerfully avoid freeing the icon
            // surface, but I'm betting that's leaky.
            SDL_FreeSurface(bmpsurface);
            bmpsurface = NULL;
        }
    }

    mFullscreen = fullscreen;
    if (mFullscreen)
    {
        int ret = SDL_SetWindowFullscreen(mWindow, SDL_WINDOW_FULLSCREEN);
        if (ret == 0)
        {
            LL_INFOS() << "createContext: setting up fullscreen " << width << "x" << height << LL_ENDL;

            // If the requested width or height is 0, find the best default for the monitor.
            if ((width == 0) || (height == 0))
            {
                // Scan through the list of modes, looking for one which has:
                //		height between 700 and 800
                //		aspect ratio closest to the user's original mode
                S32 resolutionCount = 0;
                LLWindowResolution *resolutionList = getSupportedResolutions(resolutionCount);

                if (resolutionList != nullptr)
                {
                    F32 closestAspect = 0;
                    U32 closestHeight = 0;
                    U32 closestWidth = 0;
                    int i;

                    LL_INFOS() << "createContext: searching for a display mode, original aspect is "
                               << mOriginalAspectRatio << LL_ENDL;

                    for (i = 0; i < resolutionCount; i++)
                    {
                        F32 aspect = (F32) resolutionList[i].mWidth / (F32) resolutionList[i].mHeight;

                        LL_INFOS() << "createContext: width " << resolutionList[i].mWidth << " height "
                                   << resolutionList[i].mHeight << " aspect " << aspect << LL_ENDL;

                        if ((resolutionList[i].mHeight >= 700) && (resolutionList[i].mHeight <= 800) &&
                            (fabs(aspect - mOriginalAspectRatio) < fabs(closestAspect - mOriginalAspectRatio)))
                        {
                            LL_INFOS() << " (new closest mode) " << LL_ENDL;

                            // This is the closest mode we've seen yet.
                            closestWidth = resolutionList[i].mWidth;
                            closestHeight = resolutionList[i].mHeight;
                            closestAspect = aspect;
                        }
                    }

                    width = closestWidth;
                    height = closestHeight;
                }
            }

            if ((width == 0) || (height == 0))
            {
                // Mode search failed for some reason.  Use the old-school default.
                width = 1024;
                height = 768;
            }
            SDL_DisplayMode target_mode;
            SDL_zero(target_mode);
            target_mode.w = width;
            target_mode.h = height;

            SDL_DisplayMode closest_mode;
            SDL_zero(closest_mode);
            SDL_GetClosestDisplayMode(SDL_GetWindowDisplayIndex(mWindow), &target_mode, &closest_mode);
            if (SDL_SetWindowDisplayMode(mWindow, &closest_mode) == 0)
            {
                SDL_DisplayMode mode;
                SDL_GetWindowDisplayMode(mWindow, &mode);
                mFullscreen = TRUE;
                mFullscreenWidth = mode.w;
                mFullscreenHeight = mode.h;
                mFullscreenBits = SDL_BITSPERPIXEL(mode.format);
                mFullscreenRefresh = mode.refresh_rate;

                mOverrideAspectRatio = (float) mode.w / (float) mode.h;

                LL_INFOS() << "Running at " << mFullscreenWidth << "x" << mFullscreenHeight << "x" << mFullscreenBits
                           << " @ " << mFullscreenRefresh << LL_ENDL;
            }
            else
            {
                LL_WARNS() << "createContext: fullscreen creation failure. SDL: " << SDL_GetError() << LL_ENDL;
                // No fullscreen support
                mFullscreen = FALSE;
                mFullscreenWidth = -1;
                mFullscreenHeight = -1;
                mFullscreenBits = -1;
                mFullscreenRefresh = -1;

                std::string error = llformat("Unable to run fullscreen at %d x %d.\nRunning in window.", width, height);
                OSMessageBox(error, "Error", OSMB_OK);
                SDL_SetWindowFullscreen(mWindow, 0);
                SDL_SetWindowResizable(mWindow, SDL_TRUE);
            }
        }
        else
        {
            LL_WARNS() << "Failed to set fullscreen. SDL: " << SDL_GetError() << LL_ENDL;
        }
    }

    // Create the context
    if (LLRender::sGLCoreProfile)
    {
        U32 major_gl_version = 4;
        U32 minor_gl_version = 6;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major_gl_version);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor_gl_version);

        bool done = false;
        while (!done)
        {
            mGLContext = SDL_GL_CreateContext(mWindow);

            if (!mGLContext)
            {
                if (minor_gl_version > 0)
                { //decrement minor version
                    minor_gl_version--;
                    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor_gl_version);
                }
                else if (major_gl_version > 3)
                { //decrement major version and start minor version over at 3
                    major_gl_version--;
                    minor_gl_version = 3;
                    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major_gl_version);
                    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor_gl_version);
                }
                else
                { //we reached 3.0 and still failed, bail out
                    done = true;
                }
            }
            else
            {
                done = true;
            }
        }
    }
    else
    {
        mGLContext = SDL_GL_CreateContext(mWindow);
    }

    if (!mGLContext)
	{
		LL_WARNS() << "OpenGL context creation failure. SDL: " << SDL_GetError() << LL_ENDL;
		setupFailure("Context creation error", "Error", OSMB_OK);
		return FALSE;
	}
	if (SDL_GL_MakeCurrent(mWindow, mGLContext) != 0)
	{
		LL_WARNS() << "Failed to make context current. SDL: " << SDL_GetError() << LL_ENDL;
		setupFailure("Context current failure", "Error", OSMB_OK);
		return FALSE;
	}

   	int major_gl_version = 0;
    int minor_gl_version = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major_gl_version);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor_gl_version);

    LL_INFOS() << "Created OpenGL " << llformat("%d.%d", major_gl_version, minor_gl_version) <<
				(LLRender::sGLCoreProfile ? " core" : " compatibility") << " context." << LL_ENDL;
	
	int vsync_enable = 1;
	if(disable_vsync)
		vsync_enable = 0;
		
	if(SDL_GL_SetSwapInterval(vsync_enable) == -1)
	{
		LL_INFOS() << "Swap interval not supported with sdl err: " << SDL_GetError() << LL_ENDL;
	}
	
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &redBits);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &greenBits);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &blueBits);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &alphaBits);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depthBits);
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencilBits);

    LL_INFOS() << "GL buffer:" << LL_ENDL;
    LL_INFOS() << "  Red Bits " << S32(redBits) << LL_ENDL;
    LL_INFOS() << "  Green Bits " << S32(greenBits) << LL_ENDL;
    LL_INFOS() << "  Blue Bits " << S32(blueBits) << LL_ENDL;
    LL_INFOS() << "  Alpha Bits " << S32(alphaBits) << LL_ENDL;
    LL_INFOS() << "  Depth Bits " << S32(depthBits) << LL_ENDL;
    LL_INFOS() << "  Stencil Bits " << S32(stencilBits) << LL_ENDL;

    GLint colorBits = redBits + greenBits + blueBits + alphaBits;
    // fixme: actually, it's REALLY important for picking that we get at
    // least 8 bits each of red,green,blue.  Alpha we can be a bit more
    // relaxed about if we have to.
    if (colorBits < 32)
    {
        close();
        setupFailure("Alchemy requires True Color (32-bit) to run in a window.\n"
                     "Please go to Control Panels -> Display -> Settings and\n"
                     "set the screen to 32-bit color.\n"
                     "Alternately, if you choose to run fullscreen, Alchemy\n"
                     "will automatically adjust the screen each time it runs.",
                     "Error", OSMB_OK);
        return FALSE;
    }

    if (alphaBits < 8)
    {
        close();
        setupFailure("Alchemy is unable to run because it can't get an 8 bit alpha\n"
                     "channel.  Usually this is due to video card driver issues.\n"
                     "Please make sure you have the latest video card drivers installed.\n"
                     "Also be sure your monitor is set to True Color (32-bit) in\n"
                     "Control Panels -> Display -> Settings.\n"
                     "If you continue to receive this message, contact customer service.",
                     "Error", OSMB_OK);
        return FALSE;
    }

#if LL_X11
    /* Grab the window manager specific information */
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(mWindow, &info) == SDL_TRUE)
    {
        /* Save the information for later use */
        if (info.subsystem == SDL_SYSWM_X11)
        {
            mSDL_Display = info.info.x11.display;
            mSDL_XWindowID = info.info.x11.window;
        }
        else
        {
            LL_WARNS() << "We're not running under X11?  Wild." << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS() << "We're not running under any known WM. SDL Err: " << SDL_GetError() << LL_ENDL;
    }

	{
		gGLManager.mVRAM = x11_detect_VRAM_kb() / 1024;
		if (gGLManager.mVRAM != 0)
		{
			LL_INFOS() << "X11 log-parser detected " << gGLManager.mVRAM << "MB VRAM." << LL_ENDL;
		}
	}
# endif // LL_X11

    // make sure multisampling is disabled by default
    glDisable(GL_MULTISAMPLE_ARB);

    // Don't need to get the current gamma, since there's a call that restores it to the system defaults.
    return TRUE;
}

// changing fullscreen resolution, or switching between windowed and fullscreen mode.
BOOL LLWindowSDL::switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp)
{
	const BOOL needsRebuild = TRUE;  // Just nuke the context and start over.
	BOOL result = true;

	LL_INFOS() << "switchContext, fullscreen=" << fullscreen << LL_ENDL;
	stop_glerror();
	if(needsRebuild)
	{
		destroyContext();
		result = createContext(0, 0, size.mX, size.mY, 32, fullscreen, disable_vsync);
		if (result)
		{
			gGLManager.initGL();

			//start with arrow cursor
			initCursors();
			setCursor( UI_CURSOR_ARROW );
		}
	}

	stop_glerror();

	return result;
}

void LLWindowSDL::destroyContext()
{
	LL_INFOS() << "destroyContext begins" << LL_ENDL;

	// Stop unicode input
	SDL_StopTextInput();

	// Clean up remaining GL state before blowing away window
	LL_INFOS() << "shutdownGL begins" << LL_ENDL;
	gGLManager.shutdownGL();

#if LL_X11
	mSDL_Display = NULL;
	mSDL_XWindowID = None;
#endif // LL_X11

	LL_INFOS() << "Destroying SDL cursors" << LL_ENDL;
	quitCursors();

    if (mGLContext)
    {
        LL_INFOS() << "Destroying SDL GL Context" << LL_ENDL;
        SDL_GL_DeleteContext(mGLContext);
        mGLContext = nullptr;
    }
    else
    {
        LL_INFOS() << "SDL GL Context already destroyed" << LL_ENDL;
    }

    if (mWindow)
    {
        LL_INFOS() << "Destroying SDL Window" << LL_ENDL;
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }
    else
    {
        LL_INFOS() << "SDL Window already destroyed" << LL_ENDL;
    }
	LL_INFOS() << "destroyContext end" << LL_ENDL;
}

LLWindowSDL::~LLWindowSDL()
{
	destroyContext();

	if(mSupportedResolutions != NULL)
	{
		delete []mSupportedResolutions;
	}

	gWindowImplementation = NULL;
}


void LLWindowSDL::show()
{
	if (mWindow)
	{
		SDL_ShowWindow(mWindow);
	}
}

void LLWindowSDL::hide()
{
	if (mWindow)
	{
		SDL_HideWindow(mWindow);
	}
}

//virtual
void LLWindowSDL::minimize()
{
	if (mWindow)
	{
		SDL_MinimizeWindow(mWindow);
	}
}

//virtual
void LLWindowSDL::restore()
{
	if (mWindow)
	{
		SDL_RestoreWindow(mWindow);
	}
}

// close() destroys all OS-specific code associated with a window.
// Usually called from LLWindowManager::destroyWindow()
void LLWindowSDL::close()
{
	// Is window is already closed?
	//	if (!mWindow)
	//	{
	//		return;
	//	}

	// Make sure cursor is visible and we haven't mangled the clipping state.
	setMouseClipping(FALSE);
	showCursor();

	destroyContext();
}

BOOL LLWindowSDL::isValid()
{
	return (mWindow != NULL);
}

BOOL LLWindowSDL::getVisible()
{
	BOOL result = FALSE;
	if (mWindow)
	{
		Uint32 flags = SDL_GetWindowFlags(mWindow);
		if (flags & SDL_WINDOW_SHOWN)
		{
			result = TRUE;
		}
	}
	return result;
}

BOOL LLWindowSDL::getMinimized()
{
	BOOL result = FALSE;
	if (mWindow)
	{
		Uint32 flags = SDL_GetWindowFlags(mWindow);
		if (flags & SDL_WINDOW_MINIMIZED)
		{
			result = TRUE;
		}
	}
	return result;
}

BOOL LLWindowSDL::getMaximized()
{
	if (mWindow)
	{
		Uint32 flags = SDL_GetWindowFlags(mWindow);
		if (flags & SDL_WINDOW_MAXIMIZED)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLWindowSDL::maximize()
{
	if (mWindow)
	{
		SDL_MaximizeWindow(mWindow);
		return TRUE;
	}
	return FALSE;
}

BOOL LLWindowSDL::getFullscreen()
{
	return mFullscreen;
}

BOOL LLWindowSDL::getPosition(LLCoordScreen *position)
{
	if (mWindow)
	{
		SDL_GetWindowPosition(mWindow, &position->mX, &position->mY);
		return TRUE;
	}
	return FALSE;
}

BOOL LLWindowSDL::getSize(LLCoordScreen *size)
{
	if (mWindow)
	{
		SDL_GetWindowSize(mWindow, &size->mX, &size->mY);
		return TRUE;
	}

	return FALSE;
}

BOOL LLWindowSDL::getSize(LLCoordWindow *size)
{
	if (mWindow)
	{
		SDL_GetWindowSize(mWindow, &size->mX, &size->mY);
		return (TRUE);
	}

	return (FALSE);
}

BOOL LLWindowSDL::setPosition(LLCoordScreen position)
{
	if (mWindow)
	{
		SDL_SetWindowPosition(mWindow, position.mX, position.mY);
		return TRUE;
	}

	return FALSE;
}

BOOL LLWindowSDL::setSizeImpl(LLCoordScreen size)
{
	if (mWindow)
	{
		S32 width = llmax(size.mX, (S32) mMinWindowWidth);
		S32 height = llmax(size.mY, (S32) mMinWindowHeight);

		if (mFullscreen)
		{
			SDL_DisplayMode target_mode;
			SDL_zero(target_mode);
			target_mode.w = width;
			target_mode.h = height;

			SDL_DisplayMode closest_mode;
			SDL_zero(closest_mode);
			SDL_GetClosestDisplayMode(SDL_GetWindowDisplayIndex(mWindow), &target_mode, &closest_mode);
			if (SDL_SetWindowDisplayMode(mWindow, &closest_mode) == 0)
			{
				SDL_DisplayMode mode;
				SDL_GetWindowDisplayMode(mWindow, &mode);
				mFullscreenWidth = mode.w;
				mFullscreenHeight = mode.h;
				mFullscreenBits = SDL_BITSPERPIXEL(mode.format);
				mFullscreenRefresh = mode.refresh_rate;
				mOverrideAspectRatio = (float) mode.w / (float) mode.h;

				LL_INFOS() << "Running at " << mFullscreenWidth
					<< "x" << mFullscreenHeight
					<< "x" << mFullscreenBits
					<< " @ " << mFullscreenRefresh
					<< LL_ENDL;
			}
		}
		else
		{
			SDL_SetWindowSize(mWindow, width, height);
		}
		return TRUE;
	}
		
	return FALSE;
}

BOOL LLWindowSDL::setSizeImpl(LLCoordWindow size)
{
	if(mWindow)
	{
		S32 width = llmax(size.mX, (S32) mMinWindowWidth);
		S32 height = llmax(size.mY, (S32) mMinWindowHeight);

		if (mFullscreen)
		{
			SDL_DisplayMode target_mode;
			SDL_zero(target_mode);
			target_mode.w = width;
			target_mode.h = height;

			SDL_DisplayMode closest_mode;
			SDL_zero(closest_mode);
			SDL_GetClosestDisplayMode(SDL_GetWindowDisplayIndex(mWindow), &target_mode, &closest_mode);
			if (SDL_SetWindowDisplayMode(mWindow, &closest_mode) == 0)
			{
				SDL_DisplayMode mode;
				SDL_GetWindowDisplayMode(mWindow, &mode);
				mFullscreenWidth = mode.w;
				mFullscreenHeight = mode.h;
				mFullscreenBits = SDL_BITSPERPIXEL(mode.format);
				mFullscreenRefresh = mode.refresh_rate;
				mOverrideAspectRatio = (float) mode.w / (float) mode.h;

				LL_INFOS() << "Running at " << mFullscreenWidth
					<< "x" << mFullscreenHeight
					<< "x" << mFullscreenBits
					<< " @ " << mFullscreenRefresh
					<< LL_ENDL;
			}
		}
		else
		{
			SDL_SetWindowSize(mWindow, width, height);
		}
		return TRUE;
	}

	return FALSE;
}


void LLWindowSDL::swapBuffers()
{
	if (mWindow)
	{
		SDL_GL_SwapWindow(mWindow);
	}
}

U32 LLWindowSDL::getFSAASamples()
{
	return mFSAASamples;
}

void LLWindowSDL::setFSAASamples(const U32 samples)
{
	mFSAASamples = samples;
}

F32 LLWindowSDL::getGamma()
{
	return 1.f / mGamma;
}

BOOL LLWindowSDL::restoreGamma()
{
	if (mWindow)
	{
		Uint16 ramp[256];
		SDL_CalculateGammaRamp(1.f, ramp);
		SDL_SetWindowGammaRamp(mWindow, ramp, ramp, ramp);
	}
	return true;
}

BOOL LLWindowSDL::setGamma(const F32 gamma)
{
	if (mWindow)
	{
		Uint16 ramp[256];

		mGamma = gamma;
		if (mGamma == 0) mGamma = 0.1f;
		mGamma = 1.f / mGamma;

		SDL_CalculateGammaRamp(mGamma, ramp);
		SDL_SetWindowGammaRamp(mWindow, ramp, ramp, ramp);
	}
	return true;
}

BOOL LLWindowSDL::isCursorHidden()
{
	return mCursorHidden;
}

// Constrains the mouse to the window.
void LLWindowSDL::setMouseClipping( BOOL b )
{
    //SDL_WM_GrabInput(b ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

// virtual
void LLWindowSDL::setMinSize(U32 min_width, U32 min_height, bool enforce_immediately)
{
	LLWindow::setMinSize(min_width, min_height, enforce_immediately);

	if (mWindow && min_width > 0 && min_height > 0)
	{
		SDL_SetWindowMinimumSize(mWindow, mMinWindowWidth, mMinWindowHeight);
	}
}

BOOL LLWindowSDL::setCursorPosition(LLCoordWindow position)
{
	BOOL result = TRUE;
	LLCoordScreen screen_pos;

	if (!convertCoords(position, &screen_pos))
	{
		return FALSE;
	}

	//LL_INFOS() << "setCursorPosition(" << screen_pos.mX << ", " << screen_pos.mY << ")" << LL_ENDL;

	// do the actual forced cursor move.
	SDL_WarpMouseInWindow(mWindow, screen_pos.mX, screen_pos.mY);

	//LL_INFOS() << llformat("llcw %d,%d -> scr %d,%d", position.mX, position.mY, screen_pos.mX, screen_pos.mY) << LL_ENDL;

	return result;
}

BOOL LLWindowSDL::getCursorPosition(LLCoordWindow *position)
{
	//Point cursor_point;
	LLCoordScreen screen_pos;

	SDL_GetMouseState(&screen_pos.mX, &screen_pos.mY);
	return convertCoords(screen_pos, position);
}


F32 LLWindowSDL::getNativeAspectRatio()
{
#if 0
	// RN: this hack presumes that the largest supported resolution is monitor-limited
	// and that pixels in that mode are square, therefore defining the native aspect ratio
	// of the monitor...this seems to work to a close approximation for most CRTs/LCDs
	S32 num_resolutions;
	LLWindowResolution* resolutions = getSupportedResolutions(num_resolutions);


	return ((F32)resolutions[num_resolutions - 1].mWidth / (F32)resolutions[num_resolutions - 1].mHeight);
	//rn: AC
#endif

	// MBW -- there are a couple of bad assumptions here.  One is that the display list won't include
	//		ridiculous resolutions nobody would ever use.  The other is that the list is in order.

	// New assumptions:
	// - pixels are square (the only reasonable choice, really)
	// - The user runs their display at a native resolution, so the resolution of the display
	//    when the app is launched has an aspect ratio that matches the monitor.

	//RN: actually, the assumption that there are no ridiculous resolutions (above the display's native capabilities) has 
	// been born out in my experience.  
	// Pixels are often not square (just ask the people who run their LCDs at 1024x768 or 800x600 when running fullscreen, like me)
	// The ordering of display list is a blind assumption though, so we should check for max values
	// Things might be different on the Mac though, so I'll defer to MBW

	// The constructor for this class grabs the aspect ratio of the monitor before doing any resolution
	// switching, and stashes it in mOriginalAspectRatio.  Here, we just return it.

	if (mOverrideAspectRatio > 0.f)
	{
		return mOverrideAspectRatio;
	}

	return mOriginalAspectRatio;
}

F32 LLWindowSDL::getPixelAspectRatio()
{
	F32 pixel_aspect = 1.f;
	if (getFullscreen())
	{
		LLCoordScreen screen_size;
		if (getSize(&screen_size))
		{
			pixel_aspect = getNativeAspectRatio() * (F32)screen_size.mY / (F32)screen_size.mX;
		}
	}

	return pixel_aspect;
}


// This is to support 'temporarily windowed' mode so that
// dialogs are still usable in fullscreen.
void LLWindowSDL::beforeDialog()
{
	LL_INFOS() << "LLWindowSDL::beforeDialog()" << LL_ENDL;

	if (SDLReallyCaptureInput(FALSE)) // must ungrab input so popup works!
	{
		if (mFullscreen)
		{
			// need to temporarily go non-fullscreen; bless SDL
			// for providing a SDL_WM_ToggleFullScreen() - though
			// it only works in X11
			if (mWindow)
			{
				SDL_SetWindowFullscreen( mWindow, 0 );
			}
		}
	}

#if LL_X11
	if (mSDL_Display)
	{
		// Everything that we/SDL asked for should happen before we
		// potentially hand control over to GTK.
		XSync(mSDL_Display, False);
	}
#endif // LL_X11

#if LL_GTK
	// this is a good time to grab some GTK version information for
	// diagnostics, if not already done.
	ll_try_gtk_init();
#endif // LL_GTK
}

void LLWindowSDL::afterDialog()
{
	LL_INFOS() << "LLWindowSDL::afterDialog()" << LL_ENDL;

	if (mFullscreen)
	{
		// need to restore fullscreen mode after dialog - only works
		// in X11
		if (mWindow)
		{
			SDL_SetWindowFullscreen( mWindow, SDL_WINDOW_FULLSCREEN);
		}
	}

	SDL_GL_MakeCurrent(mWindow, mGLContext);	
}


void LLWindowSDL::flashIcon(F32 seconds)
{
	LL_INFOS() << "LLWindowSDL::flashIcon(" << seconds << ")" << LL_ENDL;
	
	F32 remaining_time = mFlashTimer.getRemainingTimeF32();
	if (remaining_time < seconds)
		remaining_time = seconds;
	mFlashTimer.reset();
	mFlashTimer.setTimerExpirySec(remaining_time);

	SDL_FlashWindow(mWindow, SDL_FLASH_UNTIL_FOCUSED);
	mFlashing = TRUE;
}


#if LL_GTK
BOOL LLWindowSDL::isClipboardTextAvailable()
{
	if (ll_try_gtk_init())
	{
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_NONE);
		return gtk_clipboard_wait_is_text_available(clipboard) ?
			TRUE : FALSE;
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::pasteTextFromClipboard(LLWString &text)
{
	if (ll_try_gtk_init())
	{
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_NONE);
		gchar * const data = gtk_clipboard_wait_for_text(clipboard);
		if (data)
		{
			text = LLWString(utf8str_to_wstring(data));
			g_free(data);
			return TRUE;
		}
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::copyTextToClipboard(const LLWString &text)
{
	if (ll_try_gtk_init())
	{
		const std::string utf8 = wstring_to_utf8str(text);
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_NONE);
		gtk_clipboard_set_text(clipboard, utf8.c_str(), utf8.length());
		return TRUE;
	}
	return FALSE; // failure
}


BOOL LLWindowSDL::isPrimaryTextAvailable()
{
	if (ll_try_gtk_init())
	{
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		return gtk_clipboard_wait_is_text_available(clipboard) ?
			TRUE : FALSE;
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::pasteTextFromPrimary(LLWString &text)
{
	if (ll_try_gtk_init())
	{
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		gchar * const data = gtk_clipboard_wait_for_text(clipboard);
		if (data)
		{
			text = LLWString(utf8str_to_wstring(data));
			g_free(data);
			return TRUE;
		}
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::copyTextToPrimary(const LLWString &text)
{
	if (ll_try_gtk_init())
	{
		const std::string utf8 = wstring_to_utf8str(text);
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		gtk_clipboard_set_text(clipboard, utf8.c_str(), utf8.length());
		return TRUE;
	}
	return FALSE; // failure
}

#else

BOOL LLWindowSDL::isClipboardTextAvailable()
{
	return FALSE; // unsupported
}

BOOL LLWindowSDL::pasteTextFromClipboard(LLWString &dst)
{
	return FALSE; // unsupported
}

BOOL LLWindowSDL::copyTextToClipboard(const LLWString &s)
{
	return FALSE;  // unsupported
}

BOOL LLWindowSDL::isPrimaryTextAvailable()
{
	return FALSE; // unsupported
}

BOOL LLWindowSDL::pasteTextFromPrimary(LLWString &dst)
{
	return FALSE; // unsupported
}

BOOL LLWindowSDL::copyTextToPrimary(const LLWString &s)
{
	return FALSE;  // unsupported
}

#endif // LL_GTK

LLWindow::LLWindowResolution* LLWindowSDL::getSupportedResolutions(S32 &num_resolutions)
{
	if (!mSupportedResolutions)
	{
		mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
	}
		mNumSupportedResolutions = 0;
	int mode_count = SDL_GetNumDisplayModes(0);
		mode_count = llclamp( mode_count, 0, MAX_NUM_RESOLUTIONS );
	for (int i = mode_count - 1; i >= 0; i--)
			{
		SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
		if (SDL_GetDisplayMode(0, i, &mode) == 0)

			{
			int w = mode.w;
			int h = mode.h;
				if ((w >= 800) && (h >= 600))
				{
					// make sure we don't add the same resolution multiple times!
					if ( (mNumSupportedResolutions == 0) ||
						 ((mSupportedResolutions[mNumSupportedResolutions-1].mWidth != w) &&
					(mSupportedResolutions[mNumSupportedResolutions - 1].mHeight != h))
					)
					{
						mSupportedResolutions[mNumSupportedResolutions].mWidth = w;
						mSupportedResolutions[mNumSupportedResolutions].mHeight = h;
						mNumSupportedResolutions++;
				}
			}
		}
	}

	num_resolutions = mNumSupportedResolutions;
	return mSupportedResolutions;
}

BOOL LLWindowSDL::convertCoords(LLCoordGL from, LLCoordWindow *to)
{
	if (!to)
		return FALSE;

	if (!mWindow)
		return FALSE;
	S32 height;
	SDL_GetWindowSize(mWindow, nullptr, &height);

	to->mX = from.mX;
	to->mY = height - from.mY - 1;

	return TRUE;
}

BOOL LLWindowSDL::convertCoords(LLCoordWindow from, LLCoordGL* to)
{
	if (!to)
		return FALSE;

	if (!mWindow)
		return FALSE;
	S32 height;
	SDL_GetWindowSize(mWindow, nullptr, &height);

	to->mX = from.mX;
	to->mY = height - from.mY - 1;

	return TRUE;
}

BOOL LLWindowSDL::convertCoords(LLCoordScreen from, LLCoordWindow* to)
{
	if (!to)
		return FALSE;

	// In the fullscreen case, window and screen coordinates are the same.
	to->mX = from.mX;
	to->mY = from.mY;
	return (TRUE);
}

BOOL LLWindowSDL::convertCoords(LLCoordWindow from, LLCoordScreen *to)
{
    if (!to)
		return FALSE;

	// In the fullscreen case, window and screen coordinates are the same.
	to->mX = from.mX;
	to->mY = from.mY;
    return (TRUE);
}

BOOL LLWindowSDL::convertCoords(LLCoordScreen from, LLCoordGL *to)
{
	LLCoordWindow window_coord;

	return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}

BOOL LLWindowSDL::convertCoords(LLCoordGL from, LLCoordScreen *to)
{
	LLCoordWindow window_coord;

	return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}

void LLWindowSDL::setupFailure(const std::string& text, const std::string& caption, U32 type)
{
	destroyContext();

	OSMessageBox(text, caption, type);
}

BOOL LLWindowSDL::SDLReallyCaptureInput(BOOL capture)
{
	// note: this used to be safe to call nestedly, but in the
	// end that's not really a wise usage pattern, so don't.

	if (capture)
		mReallyCapturedCount = 1;
	else
		mReallyCapturedCount = 0;
	
	bool wantGrab;
	if (mReallyCapturedCount <= 0) // uncapture
	{
		wantGrab = false;
	} else // capture
	{
		wantGrab = true;
	}
	
	if (mReallyCapturedCount < 0) // yuck, imbalance.
	{
		mReallyCapturedCount = 0;
		LL_WARNS() << "ReallyCapture count was < 0" << LL_ENDL;
	}

	bool newGrab = wantGrab;
    if (!mFullscreen) /* only bother if we're windowed anyway */
    {
		int result;
		if (wantGrab)
		{
			// LL_INFOS() << "X11 POINTER GRABBY" << LL_ENDL;
			result = SDL_CaptureMouse(SDL_TRUE);
			if (0 == result)
				newGrab = true;
			else
				newGrab = false;
		}
		else
		{
			newGrab = false;
			result = SDL_CaptureMouse(SDL_FALSE);
		}
    }
		// pretend we got what we wanted, when really we don't care.
	
	// return boolean success for whether we ended up in the desired state
	return capture == newGrab;
}

U32 LLWindowSDL::SDLCheckGrabbyKeys(SDL_Keycode keysym, BOOL gain)
{
	/* part of the fix for SL-13243: Some popular window managers like
	   to totally eat alt-drag for the purposes of moving windows.  We
	   spoil their day by acquiring the exclusive X11 mouse lock for as
	   long as ALT is held down, so the window manager can't easily
	   see what's happening.  Tested successfully with Metacity.
	   And... do the same with CTRL, for other darn WMs.  We don't
	   care about other metakeys as SL doesn't use them with dragging
	   (for now). */

	/* We maintain a bitmap of critical keys which are up and down
	   instead of simply key-counting, because SDL sometimes reports
	   misbalanced keyup/keydown event pairs to us for whatever reason. */

	U32 mask = 0;
	switch (keysym)
	{
	case SDLK_LALT:
		mask = 1U << 0; break;
	case SDLK_RALT:
		mask = 1U << 1; break;
	case SDLK_LCTRL:
		mask = 1U << 2; break;
	case SDLK_RCTRL:
		mask = 1U << 3; break;
	default:
		break;
	}

	if (gain)
		mGrabbyKeyFlags |= mask;
	else
		mGrabbyKeyFlags &= ~mask;

	//LL_INFOS() << "mGrabbyKeyFlags=" << mGrabbyKeyFlags << LL_ENDL;

	/* 0 means we don't need to mousegrab, otherwise grab. */
	return mGrabbyKeyFlags;
}


void check_vm_bloat()
{
#if LL_LINUX
	// watch our own VM and RSS sizes, warn if we bloated rapidly
	static const std::string STATS_FILE = "/proc/self/stat";
	FILE *fp = fopen(STATS_FILE.c_str(), "r");
	if (fp)
	{
		static long long last_vm_size = 0;
		static long long last_rss_size = 0;
		const long long significant_vm_difference = 250 * 1024*1024;
		const long long significant_rss_difference = 50 * 1024*1024;
		long long this_vm_size = 0;
		long long this_rss_size = 0;

		ssize_t res;
		size_t dummy;
		char *ptr = NULL;
		for (int i=0; i<22; ++i) // parse past the values we don't want
		{
			res = getdelim(&ptr, &dummy, ' ', fp);
			if (-1 == res)
			{
				LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
				goto finally;
			}
			free(ptr);
			ptr = NULL;
		}
		// 23rd space-delimited entry is vsize
		res = getdelim(&ptr, &dummy, ' ', fp);
		llassert(ptr);
		if (-1 == res)
		{
			LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
			goto finally;
		}
		this_vm_size = atoll(ptr);
		free(ptr);
		ptr = NULL;
		// 24th space-delimited entry is RSS
		res = getdelim(&ptr, &dummy, ' ', fp);
		llassert(ptr);
		if (-1 == res)
		{
			LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
			goto finally;
		}
		this_rss_size = getpagesize() * atoll(ptr);
		free(ptr);
		ptr = NULL;

		LL_INFOS() << "VM SIZE IS NOW " << (this_vm_size/(1024*1024)) << " MB, RSS SIZE IS NOW " << (this_rss_size/(1024*1024)) << " MB" << LL_ENDL;

		if (llabs(last_vm_size - this_vm_size) >
		    significant_vm_difference)
		{
			if (this_vm_size > last_vm_size)
			{
				LL_WARNS() << "VM size grew by " << (this_vm_size - last_vm_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "VM size shrank by " << (last_vm_size - this_vm_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
			}
		}

		if (llabs(last_rss_size - this_rss_size) >
		    significant_rss_difference)
		{
			if (this_rss_size > last_rss_size)
			{
				LL_WARNS() << "RSS size grew by " << (this_rss_size - last_rss_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "RSS size shrank by " << (last_rss_size - this_rss_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
			}
		}

		last_rss_size = this_rss_size;
		last_vm_size = this_vm_size;

finally:
		if (NULL != ptr)
		{
			free(ptr);
			ptr = NULL;
		}
		fclose(fp);
	}
#endif // LL_LINUX
}


// virtual
void LLWindowSDL::processMiscNativeEvents()
{
#if LL_GTK
	// Pump GTK events to avoid starvation for:
	// * DBUS servicing
	// * Anything else which quietly hooks into the default glib/GTK loop
    if (ll_try_gtk_init())
    {
	    // Yuck, Mozilla's GTK callbacks play with the locale - push/pop
	    // the locale to protect it, as exotic/non-C locales
	    // causes our code lots of general critical weirdness
	    // and crashness. (SL-35450)
	    static std::string saved_locale;
	    saved_locale = ll_safe_string(setlocale(LC_ALL, NULL));

	    // Pump until we've nothing left to do or passed 1/15th of a
	    // second pumping for this frame.
	    static LLTimer pump_timer;
	    pump_timer.reset();
	    pump_timer.setTimerExpirySec(1.0f / 15.0f);
	    do {
		     // Always do at least one non-blocking pump
		    gtk_main_iteration_do(FALSE);
	    } while (gtk_events_pending() &&
		     !pump_timer.hasExpired());

	    setlocale(LC_ALL, saved_locale.c_str() );
    }
#endif // LL_GTK

    // hack - doesn't belong here - but this is just for debugging
    if (getenv("LL_DEBUG_BLOAT"))
    {
	    check_vm_bloat();
    }
}

void LLWindowSDL::gatherInput()
{
    // Handle all outstanding SDL events
    SDL_Event event;

    // Handle all outstanding SDL events
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_MOUSEMOTION:
        {
            LLCoordWindow winCoord(event.motion.x, event.motion.y);
            LLCoordGL openGlCoord;
            convertCoords(winCoord, &openGlCoord);
            MASK mask = gKeyboard->currentMask(TRUE);
            mCallbacks->handleMouseMove(this, openGlCoord, mask);
            break;
        }
        case SDL_MOUSEWHEEL:
        {
            if (event.wheel.y != 0)
            {
                mCallbacks->handleScrollWheel(this, -event.wheel.y);
            }
            if (event.wheel.x != 0)
            {
                mCallbacks->handleScrollHWheel(this, -event.wheel.x);
            }
            break;
        }
        case SDL_TEXTINPUT:
        {
            auto string = utf8str_to_wstring(event.text.text);
            for (auto key : string)
            {
                //mKeyVirtualKey = key;
                mCallbacks->handleUnicodeChar(key, gKeyboard->currentMask(FALSE));
            }
            break;
        }
        case SDL_KEYDOWN:
        {
            mKeyScanCode = event.key.keysym.scancode;
            mKeyVirtualKey = event.key.keysym.sym;
            mKeyModifiers = (SDL_Keymod) event.key.keysym.mod;

            gKeyboard->handleKeyDown(mKeyVirtualKey, mKeyModifiers);
            if (mKeyVirtualKey == SDLK_RETURN || mKeyVirtualKey == SDLK_KP_ENTER)
                handleUnicodeUTF16(SDLK_RETURN, gKeyboard->currentMask(FALSE));
            // part of the fix for SL-13243
            if (SDLCheckGrabbyKeys(event.key.keysym.sym, TRUE) != 0)
                SDLReallyCaptureInput(TRUE);

            break;
        }
        case SDL_KEYUP:
            mKeyScanCode = event.key.keysym.scancode;
            mKeyVirtualKey = event.key.keysym.sym;
            mKeyModifiers = (SDL_Keymod) event.key.keysym.mod;

            if (SDLCheckGrabbyKeys(mKeyVirtualKey, FALSE) == 0)
                SDLReallyCaptureInput(FALSE); // part of the fix for SL-13243

            gKeyboard->handleKeyUp(mKeyVirtualKey, mKeyModifiers);
            break;

        case SDL_MOUSEBUTTONDOWN:
        {
            LLCoordWindow winCoord(event.button.x, event.button.y);
            LLCoordGL openGlCoord;
            convertCoords(winCoord, &openGlCoord);
            MASK mask = gKeyboard->currentMask(TRUE);

            if (event.button.button == SDL_BUTTON_LEFT) // SDL doesn't manage double clicking...
            {
                if (event.button.clicks >= 2)
                    mCallbacks->handleDoubleClick(this, openGlCoord, mask);
                else
                    mCallbacks->handleMouseDown(this, openGlCoord, mask);
            }

            else if (event.button.button == SDL_BUTTON_RIGHT) // right
            {
                mCallbacks->handleRightMouseDown(this, openGlCoord, mask);
            }

            else if (event.button.button == SDL_BUTTON_MIDDLE) // middle
            {
                mCallbacks->handleMiddleMouseDown(this, openGlCoord, mask);
            }

            break;
        }

        case SDL_MOUSEBUTTONUP:
        {
            LLCoordWindow winCoord(event.button.x, event.button.y);
            LLCoordGL openGlCoord;
            convertCoords(winCoord, &openGlCoord);
            MASK mask = gKeyboard->currentMask(TRUE);

            if (event.button.button == SDL_BUTTON_LEFT) // left
                mCallbacks->handleMouseUp(this, openGlCoord, mask);
            else if (event.button.button == SDL_BUTTON_RIGHT) // right
                mCallbacks->handleRightMouseUp(this, openGlCoord, mask);
            else if (event.button.button == SDL_BUTTON_MIDDLE) // middle
                mCallbacks->handleMiddleMouseUp(this, openGlCoord, mask);
            // don't handle mousewheel here...

            break;
        }

        case SDL_WINDOWEVENT:
        {
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_SHOWN:
                break;
            case SDL_WINDOWEVENT_HIDDEN:
                break;
            case SDL_WINDOWEVENT_EXPOSED:
            {
                int width, height = 0;
                SDL_GetWindowSize(mWindow, &width, &height);

                mCallbacks->handlePaint(this, 0, 0, width, height);
                break;
            }
            case SDL_WINDOWEVENT_MOVED:
                break;
            //case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
            {
                S32 width = llmax(event.window.data1, (S32) mMinWindowWidth);
                S32 height = llmax(event.window.data2, (S32) mMinWindowHeight);
                mCallbacks->handleResize(this, width, height);
                break;
            }
            case SDL_WINDOWEVENT_MINIMIZED:
                [[fallthrough]];
            case SDL_WINDOWEVENT_MAXIMIZED:
                [[fallthrough]];
            case SDL_WINDOWEVENT_RESTORED:
            {
                Uint32 flags = SDL_GetWindowFlags(mWindow);
                bool minimized = (flags & SDL_WINDOW_MINIMIZED);

                mCallbacks->handleActivate(this, !minimized);
                LL_INFOS() << "SDL deiconification state switched to " << minimized << LL_ENDL;
                break;
            }
            case SDL_WINDOWEVENT_ENTER:
                break;
            case SDL_WINDOWEVENT_LEAVE:
				mCallbacks->handleMouseLeave(this);
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                mCallbacks->handleFocus(this);
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                mCallbacks->handleFocusLost(this);
                break;
            case SDL_WINDOWEVENT_CLOSE:
                break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
            case SDL_WINDOWEVENT_TAKE_FOCUS:
                break;
            case SDL_WINDOWEVENT_HIT_TEST:
                break;
#endif
            default:
                break;
            }
            break;
        }
        case SDL_QUIT:
            if (mCallbacks->handleCloseRequest(this))
            {
                // Get the app to initiate cleanup.
                mCallbacks->handleQuit(this);
                // The app is responsible for calling destroyWindow when done with GL
            }
            break;
        default:
            // LL_INFOS() << "Unhandled SDL event type " << event.type << LL_ENDL;
            break;
        }
    }

    updateCursor();

    // This is a good time to stop flashing the icon if our mFlashTimer has
    // expired.
    if (mFlashing && mFlashTimer.hasExpired())
    {
        SDL_FlashWindow(mWindow, SDL_FLASH_CANCEL);
        mFlashing = FALSE;
    }
}

static SDL_Cursor *makeSDLCursorFromBMP(const char *filename, int hotx, int hoty)
{
	SDL_Cursor *sdlcursor = NULL;
	SDL_Surface *bmpsurface;

	// Load cursor pixel data from BMP file
	bmpsurface = Load_BMP_Resource(filename);
	if (bmpsurface && bmpsurface->w%8==0)
	{
		SDL_Surface *cursurface;
		LL_DEBUGS() << "Loaded cursor file " << filename << " "
			 << bmpsurface->w << "x" << bmpsurface->h << LL_ENDL;
		cursurface = SDL_CreateRGBSurface (SDL_SWSURFACE,
						   bmpsurface->w,
						   bmpsurface->h,
						   32,
						   SDL_SwapLE32(0xFFU),
						   SDL_SwapLE32(0xFF00U),
						   SDL_SwapLE32(0xFF0000U),
						   SDL_SwapLE32(0xFF000000U));
		SDL_FillRect(cursurface, NULL, SDL_SwapLE32(0x00000000U));

		// Blit the cursor pixel data onto a 32-bit RGBA surface so we
		// only have to cope with processing one type of pixel format.
		if (0 == SDL_BlitSurface(bmpsurface, NULL,
					 cursurface, NULL))
		{
			// n.b. we already checked that width is a multiple of 8.
			const int bitmap_bytes = (cursurface->w * cursurface->h) / 8;
			unsigned char *cursor_data = new unsigned char[bitmap_bytes];
			unsigned char *cursor_mask = new unsigned char[bitmap_bytes];
			memset(cursor_data, 0, bitmap_bytes);
			memset(cursor_mask, 0, bitmap_bytes);
			int i,j;
			// Walk the RGBA cursor pixel data, extracting both data and
			// mask to build SDL-friendly cursor bitmaps from.  The mask
			// is inferred by color-keying against 200,200,200
			for (i=0; i<cursurface->h; ++i) {
				for (j=0; j<cursurface->w; ++j) {
					U8 *pixelp =
						((U8*)cursurface->pixels)
						+ cursurface->pitch * i
						+ j*cursurface->format->BytesPerPixel;
					U8 srcred = pixelp[0];
					U8 srcgreen = pixelp[1];
					U8 srcblue = pixelp[2];
					BOOL mask_bit = (srcred != 200)
						|| (srcgreen != 200)
						|| (srcblue != 200);
					BOOL data_bit = mask_bit && (srcgreen <= 80);//not 0x80
					unsigned char bit_offset = (cursurface->w/8) * i
						+ j/8;
					cursor_data[bit_offset]	|= (data_bit) << (7 - (j&7));
					cursor_mask[bit_offset]	|= (mask_bit) << (7 - (j&7));
				}
			}
			sdlcursor = SDL_CreateCursor((Uint8*)cursor_data,
						     (Uint8*)cursor_mask,
						     cursurface->w, cursurface->h,
						     hotx, hoty);
			delete[] cursor_data;
			delete[] cursor_mask;
		} else {
			LL_WARNS() << "CURSOR BLIT FAILURE, cursurface: " << cursurface << LL_ENDL;
		}
		SDL_FreeSurface(cursurface);
		SDL_FreeSurface(bmpsurface);
	} else {
		LL_WARNS() << "CURSOR LOAD FAILURE " << filename << LL_ENDL;
	}

	return sdlcursor;
}

void LLWindowSDL::updateCursor()
{
	if (mCurrentCursor != mNextCursor)
	{
		if (mNextCursor < UI_CURSOR_COUNT)
		{
			SDL_Cursor *sdlcursor = mSDLCursors[mNextCursor];
			// Try to default to the arrow for any cursors that
			// did not load correctly.
			if (!sdlcursor && mSDLCursors[UI_CURSOR_ARROW])
				sdlcursor = mSDLCursors[UI_CURSOR_ARROW];
			if (sdlcursor)
				SDL_SetCursor(sdlcursor);
		} else {
			LL_WARNS() << "Tried to set invalid cursor number " << mNextCursor << LL_ENDL;
		}
		mCurrentCursor = mNextCursor;
	}
}

void LLWindowSDL::initCursors()
{
	int i;
	// Blank the cursor pointer array for those we may miss.
	for (i=0; i<UI_CURSOR_COUNT; ++i)
	{
		mSDLCursors[i] = NULL;
	}
	// Pre-make an SDL cursor for each of the known cursor types.
	// We hardcode the hotspots - to avoid that we'd have to write
	// a .cur file loader.
	// NOTE: SDL doesn't load RLE-compressed BMP files.
	mSDLCursors[UI_CURSOR_ARROW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	mSDLCursors[UI_CURSOR_WAIT] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	mSDLCursors[UI_CURSOR_HAND] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	mSDLCursors[UI_CURSOR_IBEAM] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	mSDLCursors[UI_CURSOR_CROSS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	mSDLCursors[UI_CURSOR_SIZENWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	mSDLCursors[UI_CURSOR_SIZENESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	mSDLCursors[UI_CURSOR_SIZEWE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	mSDLCursors[UI_CURSOR_SIZENS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    mSDLCursors[UI_CURSOR_SIZEALL] = makeSDLCursorFromBMP("sizeall.BMP", 17, 17);
	mSDLCursors[UI_CURSOR_NO] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
	mSDLCursors[UI_CURSOR_WORKING] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW);
	mSDLCursors[UI_CURSOR_TOOLGRAB] = makeSDLCursorFromBMP("lltoolgrab.BMP", 2, 13);
	mSDLCursors[UI_CURSOR_TOOLLAND] = makeSDLCursorFromBMP("lltoolland.BMP", 1, 6);
	mSDLCursors[UI_CURSOR_TOOLFOCUS] = makeSDLCursorFromBMP("lltoolfocus.BMP", 8, 5);
	mSDLCursors[UI_CURSOR_TOOLCREATE] = makeSDLCursorFromBMP("lltoolcreate.BMP", 7, 7);
	mSDLCursors[UI_CURSOR_ARROWDRAG] = makeSDLCursorFromBMP("arrowdrag.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_ARROWCOPY] = makeSDLCursorFromBMP("arrowcop.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_ARROWDRAGMULTI] = makeSDLCursorFromBMP("llarrowdragmulti.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_ARROWCOPYMULTI] = makeSDLCursorFromBMP("arrowcopmulti.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_NOLOCKED] = makeSDLCursorFromBMP("llnolocked.BMP", 8, 8);
	mSDLCursors[UI_CURSOR_ARROWLOCKED] = makeSDLCursorFromBMP("llarrowlocked.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_GRABLOCKED] = makeSDLCursorFromBMP("llgrablocked.BMP", 2, 13);
	mSDLCursors[UI_CURSOR_TOOLTRANSLATE] = makeSDLCursorFromBMP("lltooltranslate.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_TOOLROTATE] = makeSDLCursorFromBMP("lltoolrotate.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_TOOLSCALE] = makeSDLCursorFromBMP("lltoolscale.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_TOOLCAMERA] = makeSDLCursorFromBMP("lltoolcamera.BMP", 7, 5);
	mSDLCursors[UI_CURSOR_TOOLPAN] = makeSDLCursorFromBMP("lltoolpan.BMP", 7, 5);
	mSDLCursors[UI_CURSOR_TOOLZOOMIN] = makeSDLCursorFromBMP("lltoolzoomin.BMP", 7, 5);
    mSDLCursors[UI_CURSOR_TOOLZOOMOUT] = makeSDLCursorFromBMP("lltoolzoomout.BMP", 7, 5);
	mSDLCursors[UI_CURSOR_TOOLPICKOBJECT3] = makeSDLCursorFromBMP("toolpickobject3.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_TOOLPLAY] = makeSDLCursorFromBMP("toolplay.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_TOOLPAUSE] = makeSDLCursorFromBMP("toolpause.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_TOOLMEDIAOPEN] = makeSDLCursorFromBMP("toolmediaopen.BMP", 0, 0);
	mSDLCursors[UI_CURSOR_PIPETTE] = makeSDLCursorFromBMP("lltoolpipette.BMP", 2, 28);
	mSDLCursors[UI_CURSOR_TOOLSIT] = makeSDLCursorFromBMP("toolsit.BMP", 20, 15);
	mSDLCursors[UI_CURSOR_TOOLBUY] = makeSDLCursorFromBMP("toolbuy.BMP", 20, 15);
	mSDLCursors[UI_CURSOR_TOOLOPEN] = makeSDLCursorFromBMP("toolopen.BMP", 20, 15);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING] = makeSDLCursorFromBMP("lltoolpathfinding.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_START] = makeSDLCursorFromBMP("lltoolpathfindingpathstart.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD] = makeSDLCursorFromBMP("lltoolpathfindingpathstartadd.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_END] = makeSDLCursorFromBMP("lltoolpathfindingpathend.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD] = makeSDLCursorFromBMP("lltoolpathfindingpathendadd.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLNO] = makeSDLCursorFromBMP("llno.BMP", 8, 8);
}

void LLWindowSDL::quitCursors()
{
	int i;
	if (mWindow)
	{
		for (i=0; i<UI_CURSOR_COUNT; ++i)
		{
			if (mSDLCursors[i])
			{
				SDL_FreeCursor(mSDLCursors[i]);
				mSDLCursors[i] = NULL;
			}
		}
	} else {
		// SDL doesn't refcount cursors, so if the window has
		// already been destroyed then the cursors have gone with it.
		LL_INFOS() << "Skipping quitCursors: mWindow already gone." << LL_ENDL;
		for (i=0; i<UI_CURSOR_COUNT; ++i)
			mSDLCursors[i] = NULL;
	}
}

void LLWindowSDL::captureMouse()
{
	// SDL already enforces the semantics that captureMouse is
	// used for, i.e. that we continue to get mouse events as long
	// as a button is down regardless of whether we left the
	// window, and in a less obnoxious way than SDL_WM_GrabInput
	// which would confine the cursor to the window too.

	LL_DEBUGS() << "LLWindowSDL::captureMouse" << LL_ENDL;
}

void LLWindowSDL::releaseMouse()
{
	// see LWindowSDL::captureMouse()
	
	LL_DEBUGS() << "LLWindowSDL::releaseMouse" << LL_ENDL;
}

void LLWindowSDL::hideCursor()
{
	if(!mCursorHidden)
	{
		// LL_INFOS() << "hideCursor: hiding" << LL_ENDL;
		mCursorHidden = TRUE;
		mHideCursorPermanent = TRUE;
		SDL_ShowCursor(SDL_DISABLE);
	}
	else
	{
		// LL_INFOS() << "hideCursor: already hidden" << LL_ENDL;
	}
}

void LLWindowSDL::showCursor()
{
	if(mCursorHidden)
	{
		// LL_INFOS() << "showCursor: showing" << LL_ENDL;
		mCursorHidden = FALSE;
		mHideCursorPermanent = FALSE;
		SDL_ShowCursor(SDL_ENABLE);
	}
	else
	{
		// LL_INFOS() << "showCursor: already visible" << LL_ENDL;
	}
}

void LLWindowSDL::showCursorFromMouseMove()
{
	if (!mHideCursorPermanent)
	{
		showCursor();
	}
}

void LLWindowSDL::hideCursorUntilMouseMove()
{
	if (!mHideCursorPermanent)
	{
		hideCursor();
		mHideCursorPermanent = FALSE;
	}
}



//
// LLSplashScreenSDL - I don't think we'll bother to implement this; it's
// fairly obsolete at this point.
//
LLSplashScreenSDL::LLSplashScreenSDL()
{
}

LLSplashScreenSDL::~LLSplashScreenSDL()
{
}

void LLSplashScreenSDL::showImpl()
{
}

void LLSplashScreenSDL::updateImpl(const std::string& mesg)
{
}

void LLSplashScreenSDL::hideImpl()
{
}



#if LL_GTK
static void response_callback (GtkDialog *dialog,
			       gint       arg1,
			       gpointer   user_data)
{
	gint *response = (gint*)user_data;
	*response = arg1;
	gtk_widget_destroy(GTK_WIDGET(dialog));
	gtk_main_quit();
}

S32 OSMessageBoxSDL(const std::string& text, const std::string& caption, U32 type)
{
	S32 rtn = OSBTN_CANCEL;

	if(gWindowImplementation != NULL)
		gWindowImplementation->beforeDialog();

	if (LLWindowSDL::ll_try_gtk_init())
	{
		GtkWidget *win = NULL;

		LL_INFOS() << "Creating a dialog because we're in windowed mode and GTK is happy." << LL_ENDL;
		
		GtkDialogFlags flags = GTK_DIALOG_MODAL;
		GtkMessageType messagetype;
		GtkButtonsType buttons;
		switch (type)
		{
		default:
		case OSMB_OK:
			messagetype = GTK_MESSAGE_WARNING;
			buttons = GTK_BUTTONS_OK;
			break;
		case OSMB_OKCANCEL:
			messagetype = GTK_MESSAGE_QUESTION;
			buttons = GTK_BUTTONS_OK_CANCEL;
			break;
		case OSMB_YESNO:
			messagetype = GTK_MESSAGE_QUESTION;
			buttons = GTK_BUTTONS_YES_NO;
			break;
		}
		win = gtk_message_dialog_new(NULL, flags, messagetype, buttons, "%s",
									 text.c_str());

# if LL_X11
		// Make GTK tell the window manager to associate this
		// dialog with our non-GTK SDL window, which should try
		// to keep it on top etc.
		if (gWindowImplementation &&
		    gWindowImplementation->mSDL_XWindowID != None)
		{
			gtk_widget_realize(GTK_WIDGET(win)); // so we can get its gdkwin
            GdkWindow* gdkwin = gdk_x11_window_foreign_new_for_display(gdk_display_get_default(), static_cast<Window>(gWindowImplementation->mSDL_XWindowID));
			gdk_window_set_transient_for(gtk_widget_get_window(GTK_WIDGET(win)), gdkwin);
		}
# endif //LL_X11

		gtk_window_set_position(GTK_WINDOW(win),
					GTK_WIN_POS_CENTER_ON_PARENT);

		gtk_window_set_type_hint(GTK_WINDOW(win),
					 GDK_WINDOW_TYPE_HINT_DIALOG);

		if (!caption.empty())
			gtk_window_set_title(GTK_WINDOW(win), caption.c_str());

		gint response = GTK_RESPONSE_NONE;
		g_signal_connect (win,
				  "response", 
				  G_CALLBACK (response_callback),
				  &response);

		// we should be able to use a gtk_dialog_run(), but it's
		// apparently not written to exist in a world without a higher
		// gtk_main(), so we manage its signal/destruction outselves.
		gtk_widget_show_all (win);
		gtk_main();

		//LL_INFOS() << "response: " << response << LL_ENDL;
		switch (response)
		{
		case GTK_RESPONSE_OK:     rtn = OSBTN_OK; break;
		case GTK_RESPONSE_YES:    rtn = OSBTN_YES; break;
		case GTK_RESPONSE_NO:     rtn = OSBTN_NO; break;
		case GTK_RESPONSE_APPLY:  rtn = OSBTN_OK; break;
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_DELETE_EVENT:
		default: rtn = OSBTN_CANCEL;
		}
	}
	else
	{
		LL_INFOS() << "MSGBOX: " << caption << ": " << text << LL_ENDL;
		LL_INFOS() << "Skipping dialog because we're in fullscreen mode or GTK is not happy." << LL_ENDL;
		rtn = OSBTN_OK;
	}

	if(gWindowImplementation != NULL)
		gWindowImplementation->afterDialog();

	return rtn;
}

// static void color_changed_callback(GtkWidget *widget,
// 				   gpointer user_data)
// {
// 	GtkColorSelection *colorsel = GTK_COLOR_SELECTION(widget);
// 	GdkColor *colorp = (GdkColor*)user_data;
	
// 	gtk_color_selection_get_current_color(colorsel, colorp);
// }


/*
        Make the raw keyboard data available - used to poke through to LLQtWebKit so
        that Qt/Webkit has access to the virtual keycodes etc. that it needs
*/
LLSD LLWindowSDL::getNativeKeyData()
{
	LLSD result = LLSD::emptyMap();

	result["scan_code"] = (S32)mKeyScanCode;
	result["virtual_key"] = (S32)mKeyVirtualKey;
	result["modifiers"] = (S32)mKeyModifiers;

	return result;
}


BOOL LLWindowSDL::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
	BOOL rtn = FALSE;

	beforeDialog();

// 	if (ll_try_gtk_init())
// 	{
// 		GtkWidget* win = gtk_color_chooser_dialog_new();

// # if LL_X11
// 		// Get GTK to tell the window manager to associate this
// 		// dialog with our non-GTK SDL window, which should try
// 		// to keep it on top etc.
// 		if (mSDL_XWindowID != None)
// 		{
// 			gtk_widget_realize(GTK_WIDGET(win)); // so we can get its gdkwin
//             GdkWindow* gdkwin = gdk_x11_window_foreign_new_for_display(gdk_display_get_default(), static_cast<Window>(mSDL_XWindowID));
// 			gdk_window_set_transient_for(gtk_widget_get_window(GTK_WIDGET(win)), gdkwin);
// 		}
// # endif //LL_X11

// 		GtkColorSelection *colorsel = GTK_COLOR_SELECTION (gtk_color_selection_dialog_get_color_selection (GTK_COLOR_SELECTION_DIALOG(win)));

// 		GdkColor color, orig_color;
// 		orig_color.pixel = 0;
// 		orig_color.red = guint16(65535 * *r);
// 		orig_color.green= guint16(65535 * *g);
// 		orig_color.blue = guint16(65535 * *b);
// 		color = orig_color;

// 		gtk_color_selection_set_previous_color (colorsel, &color);
// 		gtk_color_selection_set_current_color (colorsel, &color);
// 		gtk_color_selection_set_has_palette (colorsel, TRUE);
// 		gtk_color_selection_set_has_opacity_control(colorsel, FALSE);

// 		gint response = GTK_RESPONSE_NONE;
// 		g_signal_connect (win,
// 				  "response", 
// 				  G_CALLBACK (response_callback),
// 				  &response);

// 		g_signal_connect (G_OBJECT (colorsel), "color_changed",
// 				  G_CALLBACK (color_changed_callback),
// 				  &color);

// 		gtk_window_set_modal(GTK_WINDOW(win), TRUE);
// 		gtk_widget_show_all(win);
// 		gtk_main();

// 		if (response == GTK_RESPONSE_OK &&
// 		    (orig_color.red != color.red
// 		     || orig_color.green != color.green
// 		     || orig_color.blue != color.blue) )
// 		{
// 			*r = color.red / 65535.0f;
// 			*g = color.green / 65535.0f;
// 			*b = color.blue / 65535.0f;
// 			rtn = TRUE;
// 		}
// 	}

	afterDialog();

	return rtn;
}
#else
S32 OSMessageBoxSDL(const std::string& text, const std::string& caption, U32 type)
{
	LL_INFOS() << "MSGBOX: " << caption << ": " << text << LL_ENDL;
	return 0;
}

BOOL LLWindowSDL::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
	return (FALSE);
}
#endif // LL_GTK

// Open a URL with the user's default web browser.
// Must begin with protocol identifier.
void LLWindowSDL::spawnWebBrowser(const std::string& escaped_url, bool async)
{
	bool found = false;
	S32 i;
	for (i = 0; i < gURLProtocolWhitelistCount; i++)
	{
		if (escaped_url.find(gURLProtocolWhitelist[i]) != std::string::npos)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		LL_WARNS() << "spawn_web_browser called for url with protocol not on whitelist: " << escaped_url << LL_ENDL;
		return;
	}

	LL_INFOS() << "spawn_web_browser: " << escaped_url << LL_ENDL;
	
	if (SDL_OpenURL(escaped_url.c_str()) != 0)
	{
		LL_WARNS() << "spawn_web_browser failed with error: " << SDL_GetError() << LL_ENDL;		
	}

	LL_INFOS() << "spawn_web_browser returning." << LL_ENDL;
}


void *LLWindowSDL::getPlatformWindow()
{
	// Unixoid mozilla really needs GTK.
	return NULL;
}

void LLWindowSDL::bringToFront()
{
	// This is currently used when we are 'launched' to a specific
	// map position externally.
	LL_INFOS() << "bringToFront" << LL_ENDL;

	if (mWindow && !mFullscreen)
	{
		SDL_RaiseWindow(mWindow);
	}
}

void LLWindowSDL::allowLanguageTextInput(LLPreeditor *preeditor, BOOL b)
{
	if (preeditor != mPreeditor && !b)
	{
		// This condition may occur by a call to
		// setEnabled(BOOL) against LLTextEditor or LLLineEditor
		// when the control is not focused.
		// We need to silently ignore the case so that
		// the language input status of the focused control
		// is not disturbed.
		return;
	}
    
	// Take care of old and new preeditors.
	if (preeditor != mPreeditor || !b)
	{
		// We need to interrupt before updating mPreeditor,
		// so that the fix string from input method goes to
		// the old preeditor.
		if (mLanguageTextInputAllowed)
		{
			interruptLanguageTextInput();
		}
		mPreeditor = (b ? preeditor : NULL);
	}
	
	if (b == mLanguageTextInputAllowed)
	{
		return;
	}
    mLanguageTextInputAllowed = b;
    if (mLanguageTextInputAllowed)
    {
        SDL_StartTextInput();
    }
    else
    {
        SDL_StopTextInput();
    }
}

void LLWindowSDL::updateLanguageTextInputArea()
{
	if (mLanguageTextInputAllowed && mPreeditor)
	{
		LLCoordGL caret_coord;
		LLRect preedit_bounds;
		if (mPreeditor->getPreeditLocation(-1, &caret_coord, &preedit_bounds, NULL))
		{
			LLCoordWindow window_pos;
			convertCoords(caret_coord, &window_pos);

			SDL_Rect coords;
			coords.x = window_pos.mX;
			coords.y = window_pos.mY;
			coords.w = preedit_bounds.getWidth() - coords.x;
			coords.h = preedit_bounds.getHeight() - coords.y;
			SDL_SetTextInputRect(&coords);
		}
	}
}

void LLWindowSDL::setLanguageTextInput( const LLCoordGL & pos )
{
	if (mLanguageTextInputAllowed && mPreeditor)
	{
		LLCoordGL caret_coord;
		LLRect preedit_bounds;
		if (mPreeditor->getPreeditLocation(-1, &caret_coord, &preedit_bounds, NULL))
		{
			LLCoordWindow window_pos;
			convertCoords(pos, &window_pos);

			SDL_Rect coords;
			coords.x = window_pos.mX;
			coords.y = window_pos.mY;
			coords.w = preedit_bounds.getWidth() - coords.x;
			coords.h = preedit_bounds.getHeight() - coords.y;
			SDL_SetTextInputRect(&coords);
		}
	}
}

//static
std::vector<std::string> LLWindowSDL::getDynamicFallbackFontList()
{
	// Use libfontconfig to find us a nice ordered list of fallback fonts
	// specific to this system.
	std::string final_fallback("/usr/share/fonts/truetype/kochi/kochi-gothic.ttf");
	const int max_font_count_cutoff = 40; // fonts are expensive in the current system, don't enumerate an arbitrary number of them
	// Our 'ideal' font properties which define the sorting results.
	// slant=0 means Roman, index=0 means the first face in a font file
	// (the one we actually use), weight=80 means medium weight,
	// spacing=0 means proportional spacing.
	std::string sort_order("slant=0:index=0:weight=80:spacing=0");
	// elide_unicode_coverage removes fonts from the list whose unicode
	// range is covered by fonts earlier in the list.  This usually
	// removes ~90% of the fonts as redundant (which is great because
	// the font list can be huge), but might unnecessarily reduce the
	// renderable range if for some reason our FreeType actually fails
	// to use some of the fonts we want it to.
	const bool elide_unicode_coverage = true;
	std::vector<std::string> rtns;
	FcFontSet *fs = NULL;
	FcPattern *sortpat = NULL;

	LL_INFOS() << "Getting system font list from FontConfig..." << LL_ENDL;

	// If the user has a system-wide language preference, then favor
	// fonts from that language group.  This doesn't affect the types
	// of languages that can be displayed, but ensures that their
	// preferred language is rendered from a single consistent font where
	// possible.
	FL_Locale *locale = NULL;
	FL_Success success = FL_FindLocale(&locale, FL_MESSAGES);
	if (success != 0)
	{
		if (success >= 2 && locale->lang) // confident!
		{
			LL_INFOS("AppInit") << "Language " << locale->lang << LL_ENDL;
			LL_INFOS("AppInit") << "Location " << locale->country << LL_ENDL;
			LL_INFOS("AppInit") << "Variant " << locale->variant << LL_ENDL;

			LL_INFOS() << "Preferring fonts of language: "
				<< locale->lang
				<< LL_ENDL;
			sort_order = "lang=" + std::string(locale->lang) + ":"
				+ sort_order;
		}
	}
	FL_FreeLocale(&locale);

	if (!FcInit())
	{
		LL_WARNS() << "FontConfig failed to initialize." << LL_ENDL;
		rtns.push_back(final_fallback);
		return rtns;
	}

	sortpat = FcNameParse((FcChar8*) sort_order.c_str());
	if (sortpat)
	{
		// Sort the list of system fonts from most-to-least-desirable.
		FcResult result;
		fs = FcFontSort(NULL, sortpat, elide_unicode_coverage,
				NULL, &result);
		FcPatternDestroy(sortpat);
	}

	int found_font_count = 0;
	if (fs)
	{
		// Get the full pathnames to the fonts, where available,
		// which is what we really want.
		found_font_count = fs->nfont;
		for (int i=0; i<fs->nfont; ++i)
		{
			FcChar8 *filename;
			if (FcResultMatch == FcPatternGetString(fs->fonts[i],
								FC_FILE, 0,
								&filename)
			    && filename)
			{
				rtns.emplace_back((const char*)filename);
				if (rtns.size() >= max_font_count_cutoff)
					break; // hit limit
			}
		}
		FcFontSetDestroy (fs);
	}

	LL_DEBUGS() << "Using font list: " << LL_ENDL;
	for (std::vector<std::string>::iterator it = rtns.begin();
		 it != rtns.end();
		 ++it)
	{
		LL_DEBUGS() << "  file: " << *it << LL_ENDL;
	}
	LL_INFOS() << "Using " << rtns.size() << "/" << found_font_count << " system fonts." << LL_ENDL;

	rtns.push_back(final_fallback);
	return rtns;
}

void LLWindowSDL::setWindowTitle(const std::string& title)
{
	if(mWindow)
	{
		SDL_SetWindowTitle(mWindow, title.c_str());
	}
}
#endif // LL_SDL
