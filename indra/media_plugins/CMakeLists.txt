# -*- cmake -*-

include(Variables)

add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
    add_subdirectory(libvlc)
endif (LINUX)

if (DARWIN)
    add_subdirectory(libvlc)
endif (DARWIN)

if (WINDOWS)
    add_subdirectory(libvlc)
endif (WINDOWS)
add_subdirectory(cef)

### add_subdirectory(example)
