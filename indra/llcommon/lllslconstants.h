/** 
 * @file lllslconstants.h
 * @author James Cook
 * @brief Constants used in lsl.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLLSLCONSTANTS_H
#define LL_LLLSLCONSTANTS_H

// Possible error results
const U32 LSL_STATUS_OK                 = 0;
const U32 LSL_STATUS_MALFORMED_PARAMS   = 1000;
const U32 LSL_STATUS_TYPE_MISMATCH      = 1001;
const U32 LSL_STATUS_BOUNDS_ERROR       = 1002;
const U32 LSL_STATUS_NOT_FOUND          = 1003;
const U32 LSL_STATUS_NOT_SUPPORTED      = 1004;
const U32 LSL_STATUS_INTERNAL_ERROR     = 1999;

// Start per-function errors below, starting at 2000:
const U32 LSL_STATUS_WHITELIST_FAILED   = 2001;

#endif
