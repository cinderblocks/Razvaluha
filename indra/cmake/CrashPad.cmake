# -*- cmake -*-
include(Prebuilt)
include(Variables)

if(USE_CRASHPAD)
  if(NOT STANDALONE)
    use_prebuilt_binary(crashpad)
    if (WINDOWS)
      set(CRASHPAD_LIBRARIES 
      debug client.lib util.lib base.lib
      optimized client.lib util.lib base.lib)
    elseif (LINUX)

    else (DARWIN)

    endif ()
    set(CRASHPAD_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/crashpad)
  endif(NOT STANDALONE)
endif(USE_CRASHPAD)
