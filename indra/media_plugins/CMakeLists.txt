# -*- cmake -*-

include(Variables)

add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

add_subdirectory(libvlc)
add_subdirectory(cef)

### add_subdirectory(example)
