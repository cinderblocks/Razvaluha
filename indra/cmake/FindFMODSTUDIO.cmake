# -*- cmake -*-

# - Find FMODSTUDIO
# Find the FMODSTUDIO includes and library
# This module defines
#  FMODSTUDIO_INCLUDE_DIR, where to find fmod.h and fmod_errors.h
#  FMODSTUDIO_LIBRARIES, the libraries needed to use FMODSTUDIO.
#  FMODSTUDIO, If false, do not try to use FMODSTUDIO.
# also defined, but not for general use are
#  FMODSTUDIO_LIBRARY, where to find the FMODSTUDIO library.

if(WINDOWS)
  get_filename_component(FMODSTUDIO_REG_DIR [HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Windows] ABSOLUTE)
  if(ADDRESS_SIZE EQUAL 64)
    set(FMOD_ARCH_SUFFIX x64)
  elseif(ADDRESS_SIZE EQUAL 32)
    set(FMOD_ARCH_SUFFIX x86)
  endif()
  find_library(FMODSTUDIO_LIBRARY
    fmod_vc fmodL_vc 
    PATHS
    ${FMODSTUDIO_REG_DIR}/api/core/lib/${FMOD_ARCH_SUFFIX}
    ${FMODSTUDIO_REG_DIR}/api/lib
    )
  find_file(FMODSTUDIO_DLL
    NAMES
    fmod64.dll fmod64L.dll fmod.dll fmodL.dll
    PATHS
    ${FMODSTUDIO_REG_DIR}/api/core/lib/${FMOD_ARCH_SUFFIX}
    ${FMODSTUDIO_REG_DIR}/api/lib
    )
  find_path(FMODSTUDIO_INCLUDE_DIR fmod.h
    PATHS
    ${FMODSTUDIO_REG_DIR}/api/core/inc
    ${FMODSTUDIO_REG_DIR}/api/inc
    )
  if(FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
    set(FMODSTUDIO_LIBRARIES ${FMODSTUDIO_LIBRARY})
    set(FMODSTUDIO_FOUND "YES")
  endif (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
else(WINDOWS)
  set(FMODSTUDIO_NAMES fmod)
  find_path(FMODSTUDIO_INCLUDE_DIR fmod.h PATH_SUFFIXES fmod)
  find_library(FMODSTUDIO_LIBRARY
    NAMES fmod
    PATH_SUFFIXES fmod
    )
endif(WINDOWS)

if(FMODSTUDIO_FOUND)
  if(NOT FMODSTUDIO_FIND_QUIETLY)
    MESSAGE(STATUS "Found FMODSTUDIO: ${FMODSTUDIO_LIBRARIES}")
  endif(NOT FMODSTUDIO_FIND_QUIETLY)
else(FMODSTUDIO_FOUND)
  if(FMODSTUDIO_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find FMODSTUDIO library")
  endif(FMODSTUDIO_FIND_REQUIRED)
endif(FMODSTUDIO_FOUND)

mark_as_advanced(
  FMOD_ARCH_SUFFIX
  FMOD_DLL_NAME
  FMODSTUDIO_REG_DIR
  FMODSTUDIO_LIBRARY
  FMODSTUDIO_INCLUDE_DIR
  )
