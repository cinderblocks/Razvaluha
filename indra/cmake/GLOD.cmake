# -*- cmake -*-
include(Linking)
include(Prebuilt)

set(GLOD_FIND_QUIETLY OFF)
set(GLOD_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindGLOD)
else (STANDALONE)
  use_prebuilt_binary(glod)

  set(GLOD_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
  if (WINDOWS)
      set(GLOD_LIBRARIES 
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/glod.lib
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/glod.lib)
  else (WINDOWS)
    set(GLOD_LIBRARIES GLOD)
  endif (WINDOWS)
endif (STANDALONE)
