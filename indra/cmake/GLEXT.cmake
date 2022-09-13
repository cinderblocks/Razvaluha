# -*- cmake -*-
include(Prebuilt)

if (NOT STANDALONE)
  set(GLEXT_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
endif (NOT STANDALONE)
