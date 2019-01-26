# -*- cmake -*-
include(Prebuilt)

set(EXPAT_FIND_QUIETLY ON)
set(EXPAT_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindEXPAT)
else (STANDALONE)
    use_prebuilt_binary(expat)
    if (WINDOWS)
        set(EXPAT_LIBRARIES expat)
    elseif (DARWIN)
        set(EXPAT_LIBRARIES ${ARCH_PREBUILT_DIRS_RELEASE}/libexpat.a)
    else ()
        set(EXPAT_LIBRARIES expat)
    endif ()
    set(EXPAT_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
