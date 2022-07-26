# -*- cmake -*-
include(Prebuilt)
include(Linking)

set(CURL_FIND_QUIETLY ON)
set(CURL_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindCURL)
else (STANDALONE)
  use_prebuilt_binary(c-ares)
  use_prebuilt_binary(curl)
  add_definitions("-DCARES_STATICLIB")
  if (WINDOWS)
    set(CURL_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/libcurld.lib
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libcurl.lib
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/libcaresd.lib
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libcares.lib
      Normaliz.lib
      Iphlpapi.lib
    )
  elseif(DARWIN)
    set(CURL_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/libcurld.a
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libcurl.a
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/libcares.a
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libcares.a
      resolv
    )
  else ()
    set(CURL_LIBRARIES libcurl.a
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/libcares.a
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libcares.a
      )
  endif ()
  set(CURL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
