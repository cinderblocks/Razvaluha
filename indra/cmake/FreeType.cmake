# -*- cmake -*-
include(Prebuilt)

if (LINUX OR STANDALONE)
  include(FindPkgConfig)

  pkg_check_modules(FREETYPE REQUIRED freetype2)
  pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
else (LINUX OR STANDALONE)
  if(DARWIN)
    set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/freetype2)
  else()
    set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/)
  endif()
  if (WINDOWS)
    set(FREETYPE_LIBRARIES
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/freetyped.lib
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/freetype.lib)
  else()
    set(FREETYPE_LIBRARIES freetype)
  endif()
endif (LINUX OR STANDALONE)

