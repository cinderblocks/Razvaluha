# -*- cmake -*-

set(URIPARSER_FIND_QUIETLY ON)
set(URIPARSER_FIND_REQUIRED ON)

include(Prebuilt)

if (STANDALONE)
  include(FindURIPARSER)
else (STANDALONE)
  use_prebuilt_binary(uriparser)
  if (WINDOWS)
    set(URIPARSER_LIBRARY
      debug uriparserd
      optimized uriparser)
  elseif (DARWIN OR LINUX)
    set(URIPARSER_LIBRARY uriparser)
  endif (WINDOWS)
  set(URIPARSER_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/uriparser)
endif (STANDALONE)
