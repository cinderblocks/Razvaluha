# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindPkgConfig)
  pkg_check_modules(GSTREAMER010 REQUIRED gstreamer-0.10)
  pkg_check_modules(GSTREAMER010_PLUGINS_BASE REQUIRED gstreamer-plugins-base-0.10)
  pkg_check_modules(GLIB REQUIRED glib-2.0)
elseif (LINUX)
  use_prebuilt_binary(gstreamer)
  # possible libxml2 should have its own .cmake file instead. yah boiiiiii
  use_prebuilt_binary(libxml2)

  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLIB REQUIRED glib-2.0)

  set(GSTREAMER010_FOUND ON FORCE BOOL)
  set(GSTREAMER010_PLUGINS_BASE_FOUND ON FORCE BOOL)
  set(GSTREAMER010_INCLUDE_DIRS
      ${LIBS_PREBUILT_DIR}/include/gstreamer-0.10
      ${LIBS_PREBUILT_DIR}/include/libxml2
      ${GLIB_INCLUDE_DIRS}
      )
  # We don't need to explicitly link against gstreamer itself, because
  # LLMediaImplGStreamer probes for the system's copy at runtime.
  set(GSTREAMER010_LIBRARIES
      ${GLIB_LDFLAGS}
      )
endif (STANDALONE)

if (GSTREAMER010_FOUND AND GSTREAMER010_PLUGINS_BASE_FOUND)
  set(GSTREAMER010 ON CACHE BOOL "Build with GStreamer-0.10 streaming media support.")
endif (GSTREAMER010_FOUND AND GSTREAMER010_PLUGINS_BASE_FOUND)

if (GSTREAMER010)
  add_definitions(-DLL_GSTREAMER010_ENABLED=1)
endif (GSTREAMER010)

