/** 
 * @file llui.h
 * @brief General static UI services.
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


#ifndef LL_LLUI_H
#define LL_LLUI_H

#include "llrect.h"
#include "llcoord.h"
#include "llcontrol.h"
#include "llcoord.h"
#include "llcontrol.h"
#include "llglslshader.h"
#include "llinitparam.h"
#include "llregistry.h"
#include "llrender2dutils.h"
#include "llpointer.h"
#include "lluicolor.h"
#include "lluiimage.h"
#include "llframetimer.h"
#include "v2math.h"
#include <functional>
#include <limits>

// for initparam specialization
#include "llfontgl.h"


// LLUIFactory
#include "llsd.h"

class LLHtmlHelp;
class LLUUID;
class LLWindow;
class LLView;

// this enum is used by the llview.h (viewer) and the llassetstorage.h (viewer and sim) 
enum EDragAndDropType
{
	DAD_NONE			= 0,
	DAD_TEXTURE			= 1,
	DAD_SOUND			= 2,
	DAD_CALLINGCARD		= 3,
	DAD_LANDMARK		= 4,
	DAD_SCRIPT			= 5,
	DAD_CLOTHING 		= 6,
	DAD_OBJECT			= 7,
	DAD_NOTECARD		= 8,
	DAD_CATEGORY		= 9,
	DAD_ROOT_CATEGORY 	= 10,
	DAD_BODYPART		= 11,
	DAD_ANIMATION		= 12,
	DAD_GESTURE			= 13,
	DAD_LINK			= 14,
	DAD_MESH            = 15,
	DAD_WIDGET          = 16,
	DAD_PERSON          = 17,
	DAD_COUNT           = 18,   // number of types in this enum
};

// Reasons for drags to be denied.
// ordered by priority for multi-drag
enum EAcceptance
{
	ACCEPT_POSTPONED,	// we are asynchronously determining acceptance
	ACCEPT_NO,			// Uninformative, general purpose denial.
	ACCEPT_NO_CUSTOM,	// Denial with custom message.
	ACCEPT_NO_LOCKED,	// Operation would be valid, but permissions are set to disallow it.
	ACCEPT_YES_COPY_SINGLE,	// We'll take a copy of a single item
	ACCEPT_YES_SINGLE,		// Accepted. OK to drag and drop single item here.
	ACCEPT_YES_COPY_MULTI,	// We'll take a copy of multiple items
	ACCEPT_YES_MULTI		// Accepted. OK to drag and drop multiple items here.
};

enum EAddPosition
{
	ADD_TOP,
	ADD_SORTED,
	ADD_BOTTOM
};


void make_ui_sound(const char* name);

// Used to hide the flashing text cursor when window doesn't have focus.
extern BOOL gShowTextEditCursor;

class LLImageProviderInterface;

typedef	void (*LLUIAudioCallback)(const LLUUID& uuid);

class LLUI
{
	LOG_CLASS(LLUI);
public:
	//
	// Classes
	//

	struct RangeS32 
	{
		struct Params : public LLInitParam::Block<Params>
		{
			Optional<S32>	minimum,
							maximum;

			Params()
			:	minimum("min", 0),
				maximum("max", S32_MAX)
			{}
		};

		// correct for inverted params
		RangeS32(const Params& p = Params())
		:	mMin(p.minimum),
			mMax(p.maximum)
		{
			sanitizeRange();
		}

		RangeS32(S32 minimum, S32 maximum)
		:	mMin(minimum),
			mMax(maximum)
		{
			sanitizeRange();
		}

		S32 clamp(S32 input)
		{
			if (input < mMin) return mMin;
			if (input > mMax) return mMax;
			return input;
		}

		void setRange(S32 minimum, S32 maximum)
		{
			mMin = minimum;
			mMax = maximum;
			sanitizeRange();
		}

		S32 getMin() { return mMin; }
		S32 getMax() { return mMax; }

		bool operator==(const RangeS32& other) const
		{
			return mMin == other.mMin 
				&& mMax == other.mMax;
		}
	private:
		void sanitizeRange()
		{
			if (mMin > mMax)
			{
				LL_WARNS() << "Bad interval range (" << mMin << ", " << mMax << ")" << LL_ENDL;
				// since max is usually the most dangerous one to ignore (buffer overflow, etc), prefer it
				// in the case of a malformed range
				mMin = mMax;
			}
		}


		S32	mMin,
			mMax;
	};

	struct ClampedS32 : public RangeS32
	{
		struct Params : public LLInitParam::Block<Params, RangeS32::Params>
		{
			Mandatory<S32> value;

			Params()
			:	value("", 0)
			{
				addSynonym(value, "value");
			}
		};

		ClampedS32(const Params& p)
		:	RangeS32(p), mValue(0)
		{
		}

		ClampedS32(const RangeS32& range)
		:	RangeS32(range)
		{
			// set value here, after range has been sanitized
			mValue = clamp(0);
		}

		ClampedS32(S32 value, const RangeS32& range = RangeS32())
		:	RangeS32(range)
		{
			mValue = clamp(value);
		}

		S32 get()
		{
			return mValue;
		}

		void set(S32 value)
		{
			mValue = clamp(value);
		}


	private:
		S32 mValue;
	};

	//
	// Methods
	//
	static void initClass(LLControlGroup* config, 
						  LLControlGroup* account,
						  LLControlGroup* ignores,
						  LLControlGroup* colors, 
						  LLImageProviderInterface* image_provider,
						  LLUIAudioCallback audio_callback = nullptr,

						  const LLVector2 *scale_factor = nullptr,
						  const std::string& language = LLStringUtil::null);
	static void cleanupClass();

	static void pushMatrix() { LLRender2D::pushMatrix(); }
	static void popMatrix() { LLRender2D::popMatrix(); }
	static void loadIdentity() { LLRender2D::loadIdentity(); }
	static void translate(F32 x, F32 y, F32 z = 0.0f) { LLRender2D::translate(x, y, z); }

	// Return the ISO639 language name ("en", "ko", etc.) for the viewer UI.
	// http://www.loc.gov/standards/iso639-2/php/code_list.php
	static std::string getLanguage();

	//helper functions (should probably move free standing rendering helper functions here)
	static LLView* getRootView() { return sRootView; }
	static void setRootView(LLView* view) { sRootView = view; }
	static std::string locateSkin(const std::string& filename);
	static void setMousePositionScreen(S32 x, S32 y);
	static void getMousePositionScreen(S32 *x, S32 *y);
	static void setMousePositionLocal(const LLView* viewp, S32 x, S32 y);
	static void getMousePositionLocal(const LLView* viewp, S32 *x, S32 *y);
	static LLVector2& getScaleFactor() { return LLRender2D::sGLScaleFactor; }
	static void setScaleFactor(const LLVector2& scale_factor) { LLRender2D::setScaleFactor(scale_factor); }
	static void setLineWidth(F32 width) { LLRender2D::setLineWidth(width); }
	static LLPointer<LLUIImage> getUIImageByID(const LLUUID& image_id, S32 priority = 0)
		{ return LLRender2D::getUIImageByID(image_id, priority); }
	static LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority = 0)
		{ return LLRender2D::getUIImage(name, priority); }
	static LLVector2 getWindowSize();
	static void screenPointToGL(S32 screen_x, S32 screen_y, S32 *gl_x, S32 *gl_y);
	static void glPointToScreen(S32 gl_x, S32 gl_y, S32 *screen_x, S32 *screen_y);
	static void screenRectToGL(const LLRect& screen, LLRect *gl);
	static void glRectToScreen(const LLRect& gl, LLRect *screen);
	// Returns the control group containing the control name, or the default group
	static LLControlGroup& getControlControlGroup (const std::string& controlname);
	static LLWindow* getWindow() { return sWindow; }
	static void setHtmlHelp(LLHtmlHelp* html_help);

	//
	// Data
	//
	static LLControlGroup* sConfigGroup;
	static LLControlGroup* sAccountGroup;
	static LLControlGroup* sIgnoresGroup;
	static LLControlGroup* sColorsGroup;
	static LLUIAudioCallback sAudioCallback;
	static LLWindow*		sWindow;
	static LLView*			sRootView;
	static BOOL             sShowXUINames;
	static LLHtmlHelp*		sHtmlHelp;

	// *TODO: remove the following when QAR-369 settings clean-up work is in.
	// Also remove the call to this method which will then be obsolete.
	// Search for QAR-369 below to enable the proper accessing of this feature. -MG
	static void setQAMode(BOOL b);
	static BOOL sQAMode;

};

//	FactoryPolicy is a static class that controls the creation and lookup of UI elements, 
//	such as floaters.
//	The key parameter is used to provide a unique identifier and/or associated construction 
//	parameters for a given UI instance
//
//	Specialize this traits for different types, or provide a class with an identical interface 
//	in the place of the traits parameter
//
//	For example:
//
//	template <>
//	class FactoryPolicy<MyClass> /* FactoryPolicy specialized for MyClass */
//	{
//	public:
//		static MyClass* findInstance(const LLSD& key = LLSD())
//		{
//			/* return instance of MyClass associated with key */
//		}
//	
//		static MyClass* createInstance(const LLSD& key = LLSD())
//		{
//			/* create new instance of MyClass using key for construction parameters */
//		}
//	}
//	
//	class MyClass : public LLUIFactory<MyClass>
//	{
//		/* uses FactoryPolicy<MyClass> by default */
//	}

template <class T>
class FactoryPolicy
{
public:
	// basic factory methods
	static T* findInstance(const LLSD& key); // unimplemented, provide specialiation
	static T* createInstance(const LLSD& key); // unimplemented, provide specialiation
};

//	VisibilityPolicy controls the visibility of UI elements, such as floaters.
//	The key parameter is used to store the unique identifier of a given UI instance
//
//	Specialize this traits for different types, or duplicate this interface for specific instances
//	(see above)

template <class T>
class VisibilityPolicy
{
public:
	// visibility methods
	static bool visible(T* instance, const LLSD& key); // unimplemented, provide specialiation
	static void show(T* instance, const LLSD& key); // unimplemented, provide specialiation
	static void hide(T* instance, const LLSD& key); // unimplemented, provide specialiation
};

//	Manages generation of UI elements by LLSD, such that (generally) there is
//	a unique instance per distinct LLSD parameter
//	Class T is the instance type being managed, and the FACTORY_POLICY and VISIBILITY_POLICY 
//	classes provide static methods for creating, accessing, showing and hiding the associated 
//	element T
template <class T, class FACTORY_POLICY = FactoryPolicy<T>, class VISIBILITY_POLICY = VisibilityPolicy<T> >
class LLUIFactory
{
public:
	// give names to the template parameters so derived classes can refer to them
	// except this doesn't work in gcc
	typedef FACTORY_POLICY factory_policy_t;
	typedef VISIBILITY_POLICY visibility_policy_t;

	LLUIFactory() {}
	virtual ~LLUIFactory() {}

	// default show and hide methods
	static T* showInstance(const LLSD& key = LLSD()) 
	{ 
		T* instance = getInstance(key); 
		if (instance != nullptr)
			VISIBILITY_POLICY::show(instance, key);
		return instance;
	}

	static T* hideInstance(const LLSD& key = LLSD()) 
	{ 
		T* instance = getInstance(key); 
		if (instance != nullptr)
			VISIBILITY_POLICY::hide(instance, key);
		return instance;
	}

	static void toggleInstance(const LLSD& key = LLSD())
	{
		instanceVisible(key) ? hideInstance(key) : showInstance(key);
	}

	static bool instanceVisible(const LLSD& key = LLSD())
	{
		T* instance = FACTORY_POLICY::findInstance(key);
		return instance != nullptr && VISIBILITY_POLICY::visible(instance, key);
	}

	static T* getInstance(const LLSD& key = LLSD()) 
	{
		T* instance = FACTORY_POLICY::findInstance(key);
		if (instance == nullptr)
			instance = FACTORY_POLICY::createInstance(key);
		return instance;
	}
};


//	Creates a UI singleton by ignoring the identifying parameter
//	and always generating the same instance via the LLUIFactory interface.
//	Note that since UI elements can be destroyed by their hierarchy, this singleton
//	pattern uses a static pointer to an instance that will be re-created as needed.
//	
//	Usage Pattern:
//	
//	class LLFloaterFoo : public LLFloater, public LLUISingleton<LLFloaterFoo>
//	{
//		friend class LLUISingleton<LLFloaterFoo>;
//		private:
//			LLFloaterFoo(const LLSD& key);
//	};
//	
//	Note that LLUISingleton takes an option VisibilityPolicy parameter that defines
//	how showInstance(), hideInstance(), etc. work.
// 
//  https://wiki.lindenlab.com/mediawiki/index.php?title=LLUISingleton&oldid=79352

template <class T, class VISIBILITY_POLICY = VisibilityPolicy<T> >
class LLUISingleton: public LLUIFactory<T, LLUISingleton<T, VISIBILITY_POLICY>, VISIBILITY_POLICY>
{
protected:

	// T must derive from LLUISingleton<T>
	LLUISingleton() { sInstance = static_cast<T*>(this); }

	~LLUISingleton() { sInstance = nullptr; }

public:
	static T* findInstance(const LLSD& key = LLSD())
	{
		return sInstance;
	}
	
	static T* createInstance(const LLSD& key = LLSD())
	{
		if (sInstance == nullptr)
			sInstance = new T(key);
		return sInstance;
	}

private:
	static T*	sInstance;
};

template <class T, class U> T* LLUISingleton<T,U>::sInstance = nullptr;

// Moved LLLocalClipRect to lllocalcliprect.h

// useful parameter blocks
struct TimeIntervalParam : public LLInitParam::ChoiceBlock<TimeIntervalParam>
{
	Alternative<F32>		seconds;
	Alternative<S32>		frames;
	TimeIntervalParam()
	:	seconds("seconds"),
		frames("frames")
	{}
};

template <class T>
class LLUICachedControl : public LLCachedControl<T>
{
public:
	// This constructor will declare a control if it doesn't exist in the contol group
	LLUICachedControl(const std::string& name,
					  const T& default_value,
					  const std::string& comment = "Declared In Code")
	:	LLCachedControl<T>(LLUI::getControlControlGroup(name), name, default_value, comment)
	{}

	// This constructor will signal an error if the control doesn't exist in the control group
	LLUICachedControl(const std::string& name)
	:	LLCachedControl<T>(LLUI::getControlControlGroup(name), name)
	{}
};


template <typename DERIVED>
class LLParamBlock
{
protected:
	LLParamBlock() { sBlock = (DERIVED*)this; }

	typedef typename boost::add_const<DERIVED>::type Tconst;

	template <typename T>
	class LLMandatoryParam
	{
	public:
		typedef typename boost::add_const<T>::type T_const;

		LLMandatoryParam(T_const initial_val) : mVal(initial_val), mBlock(sBlock) {}
		LLMandatoryParam(const LLMandatoryParam<T>& other) : mVal(other.mVal) {}

		DERIVED& operator ()(T_const set_value) { mVal = set_value; return *mBlock; }
		operator T() const { return mVal; } 
		T operator=(T_const set_value) { mVal = set_value; return mVal; }

	private: 
		T	mVal;
		DERIVED* mBlock;
	};

	template <typename T>
	class LLOptionalParam
	{
	public:
		typedef typename boost::add_const<T>::type T_const;

		LLOptionalParam(T_const initial_val) : mVal(initial_val), mBlock(sBlock) {}
		LLOptionalParam() : mBlock(sBlock) {}
		LLOptionalParam(const LLOptionalParam<T>& other) : mVal(other.mVal) {}

		DERIVED& operator ()(T_const set_value) { mVal = set_value; return *mBlock; }
		operator T() const { return mVal; } 
		T operator=(T_const set_value) { mVal = set_value; return mVal; }

	private:
		T	mVal;
		DERIVED* mBlock;
	};

	// specialization that requires initialization for reference types 
	template <typename T>
	class LLOptionalParam <T&>
	{
	public:
		typedef typename boost::add_const<T&>::type T_const;

		LLOptionalParam(T_const initial_val) : mVal(initial_val), mBlock(sBlock) {}
		LLOptionalParam(const LLOptionalParam<T&>& other) : mVal(other.mVal) {}

		DERIVED& operator ()(T_const set_value) { mVal = set_value; return *mBlock; }
		operator T&() const { return mVal; } 
		T& operator=(T_const set_value) { mVal = set_value; return mVal; }

	private:
		T&	mVal;
		DERIVED* mBlock;
	};

	// specialization that initializes pointer params to nullptr
	template<typename T> 
	class LLOptionalParam<T*>
	{
	public:
		typedef typename boost::add_const<T*>::type T_const;

		LLOptionalParam(T_const initial_val) : mVal(initial_val), mBlock(sBlock) {}
		LLOptionalParam() : mVal(nullptr), mBlock(sBlock)  {}
		LLOptionalParam(const LLOptionalParam<T*>& other) : mVal(other.mVal) {}

		DERIVED& operator ()(T_const set_value) { mVal = set_value; return *mBlock; }
		operator T*() const { return mVal; } 
		T* operator=(T_const set_value) { mVal = set_value; return mVal; }
	private:
		T*	mVal;
		DERIVED* mBlock;
	};

	static DERIVED* sBlock;
};

template <typename T> T* LLParamBlock<T>::sBlock = nullptr;


namespace LLInitParam
{
	template<>
	class ParamValue<LLRect> 
	:	public CustomParamValue<LLRect>
	{
        typedef CustomParamValue<LLRect> super_t;
	public:
		Optional<S32>	left,
						top,
						right,
						bottom,
						width,
						height;

		ParamValue(const LLRect& value);

		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};

	template<>
	class ParamValue<LLUIColor> 
	:	public CustomParamValue<LLUIColor>
	{
        typedef CustomParamValue<LLUIColor> super_t;

	public:
		Optional<F32>			red,
								green,
								blue,
								alpha;
		Optional<std::string>	control;

		ParamValue(const LLUIColor& color);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};

	template<>
	class ParamValue<const LLFontGL*> 
	:	public CustomParamValue<const LLFontGL* >
	{
        typedef CustomParamValue<const LLFontGL*> super_t;
	public:
		Optional<std::string>	name,
								size,
								style;

		ParamValue(const LLFontGL* value);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};

	template<>
	struct TypeValues<LLFontGL::HAlign> : public TypeValuesHelper<LLFontGL::HAlign>
	{
		static void declareValues();
	};

	template<>
	struct TypeValues<LLFontGL::VAlign> : public TypeValuesHelper<LLFontGL::VAlign>
	{
		static void declareValues();
	};

	template<>
	struct TypeValues<LLFontGL::ShadowType> : public TypeValuesHelper<LLFontGL::ShadowType>
	{
		static void declareValues();
	};

	template<>
	struct ParamCompare<const LLFontGL*, false>
	{
		static bool equals(const LLFontGL* a, const LLFontGL* b);
	};


	template<>
	class ParamValue<LLCoordGL>
	:	public CustomParamValue<LLCoordGL>
	{
		typedef CustomParamValue<LLCoordGL> super_t;
	public:
		Optional<S32>	x,
						y;

		ParamValue(const LLCoordGL& val);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};
}

#endif
