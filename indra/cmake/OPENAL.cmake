# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (OPENAL)
  set(OPENAL_LIB_INCLUDE_DIRS "${LIBS_PREBUILT_DIR}/include/AL")
  if (STANDALONE)
    include(FindPkgConfig)
    include(FindOpenAL)
    pkg_check_modules(OPENAL_LIB REQUIRED openal)
    pkg_check_modules(FREEALUT_LIB REQUIRED freealut)
  else (STANDALONE)
    use_prebuilt_binary(openal)
  endif (STANDALONE)
  if(WINDOWS)
  set(OPENAL_LIBRARIES 
    OpenAL32
    alut
    )
  else()
  set(OPENAL_LIBRARIES 
  openal
  alut
    )
  endif()
  message(STATUS "Building with OpenAL audio support")
endif (OPENAL)
