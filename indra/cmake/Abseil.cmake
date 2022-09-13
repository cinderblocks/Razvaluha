# -*- cmake -*-
include(Linking)
include(Prebuilt)

option(USE_BINARY_ABSL "Use binary package for abseil" ON)

if(USE_BINARY_ABSL)
  if(STANDALONE)
    find_package(absl REQUIRED)
  else(STANDALONE)
    use_prebuilt_binary(abseil-cpp)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      find_package(absl REQUIRED PATHS "${AUTOBUILD_INSTALL_DIR}/absl/debug" NO_DEFAULT_PATH)
    else()
      find_package(absl REQUIRED PATHS "${AUTOBUILD_INSTALL_DIR}/absl/release" NO_DEFAULT_PATH) 
    endif()
  endif(STANDALONE)
endif(USE_BINARY_ABSL)