include(Prebuilt)
include(Linking)

set(NGHTTP2_FIND_QUIETLY ON)
set(NGHTTP2_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindNGHTTP2)
else (STANDALONE)
  use_prebuilt_binary(nghttp2)
  if (WINDOWS)
    set(NGHTTP2_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/nghttp2.lib
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/nghttp2.lib
      )
  elseif (DARWIN)
    set(NGHTTP2_LIBRARIES libnghttp2.a)
  else (WINDOWS)
    set(NGHTTP2_LIBRARIES libnghttp2.a)
  endif (WINDOWS)
  set(NGHTTP2_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/nghttp2)
endif (STANDALONE)
