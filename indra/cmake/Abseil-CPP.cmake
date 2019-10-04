# -*- cmake -*-
include(Prebuilt)

set(BUILD_TESTING OFF)
use_prebuilt_binary(abseil-cpp)
set(ABSEIL_SRC_DIR ${LIBS_PREBUILT_DIR}/abseil-cpp)
set(ABSEIL_BIN_DIR ${CMAKE_BINARY_DIR}/abseil-cpp)
if (DARWIN)
    add_definitions(-DTARGET_OS_IPHONE_SIMULATOR=0)
endif (DARWIN)

