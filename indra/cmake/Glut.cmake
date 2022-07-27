# -*- cmake -*-
include(Linking)
include(Prebuilt)

if(WINDOWS)
    use_prebuilt_binary(freeglut)
    set(GLUT_LIBRARY
        debug freeglut_static.lib
        optimized freeglut_static.lib)
elseif(LINUX)
  FIND_LIBRARY(GLUT_LIBRARY glut)
elseif(DARWIN)
  include(CMakeFindFrameworks)
  find_library(GLUT_LIBRARY GLUT)
endif ()
