# -*- cmake -*-
include(Prebuilt)

if (LINUX)
  include(FindPkgConfig)
  pkg_check_modules(OGG REQUIRED ogg)
  pkg_check_modules(VORBIS REQUIRED vorbis)
  pkg_check_modules(VORBISFILE REQUIRED vorbisfile)
else (LINUX)
  use_prebuilt_binary(ogg_vorbis)
  set(VORBIS_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  set(VORBISENC_INCLUDE_DIRS ${VORBIS_INCLUDE_DIRS})
  set(VORBISFILE_INCLUDE_DIRS ${VORBIS_INCLUDE_DIRS})

  if (WINDOWS)
    set(OGG_LIBRARIES
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libogg.lib
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libogg.lib)
    set(VORBIS_LIBRARIES
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libvorbis.lib
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libvorbis.lib)
    set(VORBISFILE_LIBRARIES
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libvorbisfile.lib
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libvorbisfile.lib)
  else (WINDOWS)
    set(OGG_LIBRARIES 
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libogg.a
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libogg.a)
    set(VORBIS_LIBRARIES 
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libvorbis.a
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libvorbis.a)
    set(VORBISENC_LIBRARIES 
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libvorbisenc.a
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libvorbisenc.a)
    set(VORBISFILE_LIBRARIES 
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libvorbisfile.a
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libvorbisfile.a)
  endif (WINDOWS)
endif (LINUX)

link_directories(
    ${VORBIS_LIBRARY_DIRS}
    ${VORBISFILE_LIBRARY_DIRS}
    ${OGG_LIBRARY_DIRS}
    )

set(LLAUDIO_VORBIS_LIBRARIES
    ${VORBISFILE_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
    )

