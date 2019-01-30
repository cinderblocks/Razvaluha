# -*- cmake -*-
include(Prebuilt)

set(CURL_FIND_QUIETLY ON)
set(CURL_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindCURL)
else (STANDALONE)
  use_prebuilt_binary(curl)
  use_prebuilt_binary(nghttp2)
  if (WINDOWS)
    set(CURL_LIBRARIES 
    debug libcurl_a_debug.lib nghttp2.lib Normaliz.lib
    optimized libcurl_a.lib nghttp2.lib Normaliz.lib)
  elseif (LINUX)
    set(CURL_LIBRARIES curl)
  else (DARWIN)
    set(CURL_LIBRARIES curl iconv)
  endif ()
  set(CURL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
