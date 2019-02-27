/** 
 * @file llglstates.h
 * @brief LLGL states definitions
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

//THIS HEADER SHOULD ONLY BE INCLUDED FROM llgl.h
#ifndef LL_LLGLSTATES_H
#define LL_LLGLSTATES_H

#include "llimagegl.h"

#ifdef LL_GL_CORE
#define GL_COLOR_MATERIAL_LEGACY 0
#define GL_ALPHA_TEST_LEGACY 0
#define GL_LIGHTING_LEGACY 0
#define GL_FOG_LEGACY 0
#define GL_LINE_SMOOTH_LEGACY 0
#define GL_LINE_STIPPLE_LEGACY 0
#define GL_NORMALIZE_LEGACY 0
#else
#define GL_COLOR_MATERIAL_LEGACY GL_COLOR_MATERIAL
#define GL_ALPHA_TEST_LEGACY GL_ALPHA_TEST
#define GL_LIGHTING_LEGACY GL_LIGHTING
#define GL_FOG_LEGACY GL_FOG
#define GL_LINE_SMOOTH_LEGACY GL_LINE_SMOOTH
#define GL_LINE_STIPPLE_LEGACY GL_LINE_STIPPLE
#define GL_NORMALIZE_LEGACY GL_NORMALIZE
#endif

//----------------------------------------------------------------------------

class LLGLDepthTest
{
	// Enabled by default
public:
	LLGLDepthTest(GLboolean depth_enabled, GLboolean write_enabled = GL_TRUE, GLenum depth_func = GL_LEQUAL);
	
	~LLGLDepthTest();
	
	void checkState();

	GLboolean mPrevDepthEnabled;
	GLenum mPrevDepthFunc;
	GLboolean mPrevWriteEnabled;
private:
	static GLboolean sDepthEnabled; // defaults to GL_FALSE
	static GLenum sDepthFunc; // defaults to GL_LESS
	static GLboolean sWriteEnabled; // defaults to GL_TRUE
};

//----------------------------------------------------------------------------

struct LLGLSDefault
{
private:
	LLGLDisable<GL_BLEND> mBlend;
	LLGLDisable<GL_CULL_FACE> mCullFace;
	LLGLDisable<GL_DITHER> mDither;
	LLGLDisable<GL_POLYGON_SMOOTH> mPolygonSmooth;
	LLGLDisable<GL_MULTISAMPLE_ARB> mGLMultisample;
#ifndef LL_GL_CORE
	LLGLEnable<GL_COLOR_MATERIAL_LEGACY> mColorMaterial;
	LLGLDisable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
	LLGLDisable<GL_LIGHTING_LEGACY> lighting;
	LLGLDisable<GL_FOG_LEGACY> mFog;
	LLGLDisable<GL_LINE_SMOOTH_LEGACY> mLineSmooth;
	LLGLDisable<GL_LINE_STIPPLE_LEGACY> mLineStipple;
	LLGLDisable<GL_NORMALIZE_LEGACY> mNormalize;
#endif
};

struct LLGLSObjectSelect
{
private:
	LLGLDisable<GL_BLEND> mBlend;
	LLGLEnable<GL_CULL_FACE> mCullFace;
#ifndef LL_GL_CORE
	LLGLDisable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
	LLGLDisable<GL_FOG_LEGACY> mFog;
#endif
};

struct LLGLSObjectSelectAlpha
{
#ifndef LL_GL_CORE
private:
	LLGLEnable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
#endif
};

//----------------------------------------------------------------------------

struct LLGLSUIDefault
{ 
private:
	LLGLEnable<GL_BLEND> mBlend;
	LLGLDisable<GL_CULL_FACE> mCullFace;
	//LLGLEnable<GL_SCISSOR_TEST> mScissor;
	LLGLDepthTest mDepthTest;
	LLGLDisable<GL_MULTISAMPLE_ARB> mMSAA;
#ifndef LL_GL_CORE
	LLGLEnable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
#endif
public:
	LLGLSUIDefault()
		: mDepthTest(GL_FALSE, GL_TRUE, GL_LEQUAL)
	{}
};

struct LLGLSNoAlphaTest
{
	LLGLSNoAlphaTest(bool noskip = true)
#ifndef LL_GL_CORE
		: mAlphaTest(noskip)
#endif
	{}
#ifndef LL_GL_CORE
private:
	LLGLDisable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
#endif
};

struct LLGLSAlphaTest
{
	LLGLSAlphaTest(bool noskip = true)
#ifndef LL_GL_CORE
		: mAlphaTest(noskip)
#endif
	{}
#ifndef LL_GL_CORE
private:
	LLGLEnable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
#endif
};

//----------------------------------------------------------------------------

struct LLGLSFog
{
#ifndef LL_GL_CORE
private:
	LLGLEnable<GL_FOG_LEGACY> mFog;
#endif
};

struct LLGLSNoFog
{
#ifndef LL_GL_CORE
private:
	LLGLDisable<GL_FOG_LEGACY> mFog;
#endif
};

//----------------------------------------------------------------------------

struct LLGLSPipeline
{ 
private:
	LLGLEnable<GL_CULL_FACE> mCullFace;
	LLGLDepthTest mDepthTest;
public:
	LLGLSPipeline()
		: mDepthTest(GL_TRUE, GL_TRUE, GL_LEQUAL)
	{}		
};

struct LLGLSPipelineAlpha // : public LLGLSPipeline
{ 
private:
	LLGLEnable<GL_BLEND> mBlend;
#ifndef LL_GL_CORE
	LLGLEnable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
#endif
};

struct LLGLSPipelineEmbossBump
{
private:
	LLGLDisable<GL_FOG> mFog;
};

struct LLGLSPipelineSelection
{ 
private:
	LLGLDisable<GL_CULL_FACE> mCullFace;
};

struct LLGLSPipelineAvatar
{
#ifndef LL_GL_CORE
private:
	LLGLEnable<GL_NORMALIZE_LEGACY> mNormalize;
#endif
};

struct LLGLSPipelineSkyBox
{ 
private:
	LLGLDisable<GL_CULL_FACE> mCullFace;
#ifndef LL_GL_CORE
	LLGLDisable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
	LLGLDisable<GL_FOG_LEGACY> mFog;
	LLGLDisable<GL_LIGHTING_LEGACY> mLighting;
#endif
};

struct LLGLSTracker
{
private:
	LLGLEnable<GL_BLEND> mBlend;
	LLGLEnable<GL_CULL_FACE> mCullFace;
#ifndef LL_GL_CORE
	LLGLEnable<GL_ALPHA_TEST_LEGACY> mAlphaTest;
#endif
};

//----------------------------------------------------------------------------

#ifndef LL_GL_CORE
class LLGLSSpecular
{
public:
	F32 mShininess;
	LLGLSSpecular(const LLColor4& color, F32 shininess)
	{
		mShininess = shininess;
		if (mShininess > 0.0f)
		{
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color.mV);
			S32 shiny = (S32)(shininess*128.f);
			shiny = llclamp(shiny,0,128);
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shiny);
		}
	}
	~LLGLSSpecular()
	{
		if (mShininess > 0.f)
		{
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, LLColor4(0.f,0.f,0.f,0.f).mV);
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 0);
		}
	}
};
#endif

//----------------------------------------------------------------------------

#endif
