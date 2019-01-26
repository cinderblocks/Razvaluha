// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llwind.cpp
 * @brief LLWind class implementation
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

// Wind is a lattice.  It is computed on the simulator, and transmitted to the viewer.
// It drives special effects like smoke blowing, trees bending, and grass wiggling.
//
// Currently wind lattice does not interpolate correctly to neighbors.  This will need 
// work.

#include "llviewerprecompiledheaders.h"
#include "indra_constants.h"

#include "llwind.h"

// linden libraries
#include "llgl.h"
#include "patch_dct.h"
#include "patch_code.h"

// viewer
#include "v4color.h"
#include "llagent.h"	// for gAgent
#include "llworld.h"
#include "llviewerregion.h"	// for getRegion()

const F32 CLOUD_DIVERGENCE_COEF = 0.5f; 

#include <numeric>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LLWind::LLWind()
:	mCloudDensityp(nullptr)
{
	init();
}


LLWind::~LLWind()
{
}


//////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////


void LLWind::init()
{
	LL_DEBUGS("Wind") << "initializing wind size: "<< WIND_SIZE << LL_ENDL;

	// Initialize vector data
	mVelX.fill(0.5f);
	mVelY.fill(0.5f);
	mCloudVelX.fill(0.0f);
	mCloudVelY.fill(0.0f);
}


void LLWind::decompress(LLBitPack &bitpack, LLGroupHeader *group_headerp)
{
	static const LLCachedControl<bool> wind_enabled("WindEnabled",false); 
	if (!mCloudDensityp || !wind_enabled) return;

	LLPatchHeader  patch_header;
	S32 buffer[ARRAY_SIZE];

	init_patch_decompressor(group_headerp->patch_size);

	// Don't use the packed group_header stride because the strides used on
	// simulator and viewer are not equal.
	group_headerp->stride = group_headerp->patch_size;	
	set_group_of_patch_header(group_headerp);

	// X component
	decode_patch_header(bitpack, &patch_header);
	decode_patch(bitpack, buffer);
	decompress_patch(mVelX.data(), buffer, &patch_header);

	// Y component
	decode_patch_header(bitpack, &patch_header);
	decode_patch(bitpack, buffer);
	decompress_patch(mVelY.data(), buffer, &patch_header);

	S32 i, j, k;
	// HACK -- mCloudVelXY is the same as mVelXY, except we add a divergence
	// that is proportional to the gradient of the cloud density 
	// ==> this helps to clump clouds together
	// NOTE ASSUMPTION: cloud density has the same dimensions as the wind field
	// This needs to be fixed... causes discrepency at region boundaries

	for (j=1; j<WIND_SIZE-1; j++)
	{
		for (i=1; i<WIND_SIZE-1; i++)
		{
			k = i + j * WIND_SIZE;
			mCloudVelX[k] = mVelX[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 1) - *(mCloudDensityp + k - 1));
			mCloudVelY[k] = mVelY[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + WIND_SIZE) - *(mCloudDensityp + k - WIND_SIZE));
		}
	}

	i = WIND_SIZE - 1;
	for (j=1; j<WIND_SIZE-1; j++)
	{
		k = i + j * WIND_SIZE;
		mCloudVelX[k] = mVelX[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k) - *(mCloudDensityp + k - 2));
		mCloudVelY[k] = mVelY[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + WIND_SIZE) - *(mCloudDensityp + k - WIND_SIZE));
	}
	i = 0;
	for (j=1; j<WIND_SIZE-1; j++)
	{
		k = i + j * WIND_SIZE;
		mCloudVelX[k] = mVelX[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 2) - *(mCloudDensityp + k));
		mCloudVelY[k] = mVelY[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + WIND_SIZE) - *(mCloudDensityp + k + WIND_SIZE));
	}
	j = WIND_SIZE - 1;
	for (i=1; i<WIND_SIZE-1; i++)
	{
		k = i + j * WIND_SIZE;
		mCloudVelX[k] = mVelX[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 1) - *(mCloudDensityp + k - 1));
		mCloudVelY[k] = mVelY[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k) - *(mCloudDensityp + k - 2*WIND_SIZE));
	}
	j = 0;
	for (i=1; i<WIND_SIZE-1; i++)
	{
		k = i + j * WIND_SIZE;
		mCloudVelX[k] = mVelX[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 1) - *(mCloudDensityp + k -1));
		mCloudVelY[k] = mVelY[k] + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 2*WIND_SIZE) - *(mCloudDensityp + k));
	}
}


LLVector3 LLWind::getAverage()
{
	static const LLCachedControl<bool> wind_enabled("WindEnabled",false); 
	if (!wind_enabled) return LLVector3(0.f, 0.f, 0.f);

	constexpr F32 scalar = 1.f/((F32)(ARRAY_SIZE)) * WIND_SCALE_HACK;
	//  Returns in average_wind the average wind velocity 
	return LLVector3(
		std::accumulate(mVelX.begin(), mVelX.end(), 0.0f) * scalar,
		std::accumulate(mVelY.begin(), mVelY.end(), 0.0f) * scalar,
		0.0f
	);
}


LLVector3 LLWind::getVelocityNoisy(const LLVector3 &pos_region, const F32 dim)
{
	static const LLCachedControl<bool> wind_enabled("WindEnabled",false); 
	if (!wind_enabled) return LLVector3(0.f, 0.f, 0.f);
	
	//  Resolve a value, using fractal summing to perturb the returned value 
	LLVector3 r_val(0,0,0);
	F32 norm = 1.0f;
	if (dim == 8)
	{
		norm = 1.875;
	}
	else if (dim == 4)
	{
		norm = 1.75;
	}
	else if (dim == 2)
	{
		norm = 1.5;
	}

	F32 temp_dim = dim;
	while (temp_dim >= 1.0)
	{
		LLVector3 pos_region_scaled(pos_region * temp_dim);
		r_val += getVelocity(pos_region_scaled) * (1.0f/temp_dim);
		temp_dim /= 2.0;
	}
	
	return r_val * (1.0f/norm) * WIND_SCALE_HACK;
}


LLVector3 LLWind::getVelocity(const LLVector3 &pos_region)
{
	static const LLCachedControl<bool> wind_enabled("WindEnabled",false); 
	if (!wind_enabled) return LLVector3(0.f, 0.f, 0.f);

	// Resolves value of wind at a location relative to SW corner of region
	//  
	// Returns wind magnitude in X,Y components of vector3
	LLVector3 r_val;
	F32 dx,dy;
	S32 k;

	LLVector3 pos_clamped_region(pos_region);
	
	F32 region_width_meters = gAgent.getRegion()->getWidth();

	if (pos_clamped_region.mV[VX] < 0.f)
	{
		pos_clamped_region.mV[VX] = 0.f;
	}
	else if (pos_clamped_region.mV[VX] >= region_width_meters)
	{
		pos_clamped_region.mV[VX] = (F32) fmod(pos_clamped_region.mV[VX], region_width_meters);
	}

	if (pos_clamped_region.mV[VY] < 0.f)
	{
		pos_clamped_region.mV[VY] = 0.f;
	}
	else if (pos_clamped_region.mV[VY] >= region_width_meters)
	{
		pos_clamped_region.mV[VY] = (F32) fmod(pos_clamped_region.mV[VY], region_width_meters);
	}
	
	
	S32 i = llfloor(pos_clamped_region.mV[VX] * WIND_SIZE / region_width_meters);
	S32 j = llfloor(pos_clamped_region.mV[VY] * WIND_SIZE / region_width_meters);
	k = i + j*WIND_SIZE;
	dx = ((pos_clamped_region.mV[VX] * WIND_SIZE / region_width_meters) - (F32) i);
	dy = ((pos_clamped_region.mV[VY] * WIND_SIZE / region_width_meters) - (F32) j);

	if ((i < WIND_SIZE-1) && (j < WIND_SIZE-1))
	{
		//  Interior points, no edges
		r_val.mV[VX] =  mVelX[k]*(1.0f - dx)*(1.0f - dy) + 
						mVelX[k + 1]*dx*(1.0f - dy) + 
						mVelX[k + WIND_SIZE]*dy*(1.0f - dx) + 
						mVelX[k + WIND_SIZE + 1]*dx*dy;
		r_val.mV[VY] =  mVelY[k]*(1.0f - dx)*(1.0f - dy) + 
						mVelY[k + 1]*dx*(1.0f - dy) + 
						mVelY[k + WIND_SIZE]*dy*(1.0f - dx) + 
						mVelY[k + WIND_SIZE + 1]*dx*dy;
	}
	else 
	{
		r_val.mV[VX] = mVelX[k];
		r_val.mV[VY] = mVelY[k];
	}

	r_val.mV[VZ] = 0.f;
	return r_val * WIND_SCALE_HACK;
}

LLVector3 LLWind::getCloudVelocity(const LLVector3 &pos_region)
{
	static const LLCachedControl<bool> wind_enabled("WindEnabled",false); 
	if (!wind_enabled) return LLVector3(0.f, 0.f, 0.f);

	// Resolves value of wind at a location relative to SW corner of region
	//  
	// Returns wind magnitude in X,Y components of vector3
	LLVector3 r_val;
	F32 dx,dy;
	S32 k;

	LLVector3 pos_clamped_region(pos_region);
	
	F32 region_width_meters = gAgent.getRegion()->getWidth();

	if (pos_clamped_region.mV[VX] < 0.f)
	{
		pos_clamped_region.mV[VX] = 0.f;
	}
	else if (pos_clamped_region.mV[VX] >= region_width_meters)
	{
		pos_clamped_region.mV[VX] = (F32) fmod(pos_clamped_region.mV[VX], region_width_meters);
	}

	if (pos_clamped_region.mV[VY] < 0.f)
	{
		pos_clamped_region.mV[VY] = 0.f;
	}
	else if (pos_clamped_region.mV[VY] >= region_width_meters)
	{
		pos_clamped_region.mV[VY] = (F32) fmod(pos_clamped_region.mV[VY], region_width_meters);
	}
	
	
	S32 i = llfloor(pos_clamped_region.mV[VX] * WIND_SIZE / region_width_meters);
	S32 j = llfloor(pos_clamped_region.mV[VY] * WIND_SIZE / region_width_meters);
	k = i + j*WIND_SIZE;
	dx = ((pos_clamped_region.mV[VX] * WIND_SIZE / region_width_meters) - (F32) i);
	dy = ((pos_clamped_region.mV[VY] * WIND_SIZE / region_width_meters) - (F32) j);

	if ((i < WIND_SIZE-1) && (j < WIND_SIZE-1))
	{
		//  Interior points, no edges
		r_val.mV[VX] =  mCloudVelX[k]*(1.0f - dx)*(1.0f - dy) + 
						mCloudVelX[k + 1]*dx*(1.0f - dy) + 
						mCloudVelX[k + WIND_SIZE]*dy*(1.0f - dx) + 
						mCloudVelX[k + WIND_SIZE + 1]*dx*dy;
		r_val.mV[VY] =  mCloudVelY[k]*(1.0f - dx)*(1.0f - dy) + 
						mCloudVelY[k + 1]*dx*(1.0f - dy) + 
						mCloudVelY[k + WIND_SIZE]*dy*(1.0f - dx) + 
						mCloudVelY[k + WIND_SIZE + 1]*dx*dy;
	}
	else 
	{
		r_val.mV[VX] = mCloudVelX[k];
		r_val.mV[VY] = mCloudVelY[k];
	}

	r_val.mV[VZ] = 0.f;
	return r_val * WIND_SCALE_HACK;
}

//keep the setters for now, in case wind is re-enabled during runtime
void LLWind::setCloudDensityPointer(F32 *densityp)
{
	mCloudDensityp = densityp;
}

void LLWind::setOriginGlobal(const LLVector3d &origin_global)
{
	mOriginGlobal = origin_global;
}


