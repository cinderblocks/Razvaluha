# -*- cmake -*-
include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindPkgConfig)

  pkg_check_modules(FREETYPE REQUIRED freetype2)
else (USESYSTEMLIBS)
  use_prebuilt_binary(freetype)
  if(WINDOWS)
    set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/freetype/)
  else(WINDOWS)
    set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/freetype2/)
  endif(WINDOWS)
  set(FREETYPE_LIBRARIES freetype)
endif (USESYSTEMLIBS)

link_directories(${FREETYPE_LIBRARY_DIRS})
