# -*- cmake -*-
include(Linking)
include(Prebuilt)
include(Variables)

if (LIBVLCPLUGIN)
  if(NOT STANDALONE)
    use_prebuilt_binary(vlc-bin)
    set(VLC_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/vlc)
  endif(NOT STANDALONE)

  if(WINDOWS)
    set(VLC_PLUGIN_LIBRARIES
        libvlc.lib
        libvlccore.lib
    )
  elseif(DARWIN)
    set(VLC_PLUGIN_LIBRARIES
        libvlc.dylib
        libvlccore.dylib
    )
  elseif(LINUX)
    # Specify a full path to make sure we get a static link
    set(VLC_PLUGIN_LIBRARIES
        ${LIBS_PREBUILT_DIR}/lib/libvlc.a
        ${LIBS_PREBUILT_DIR}/lib/libvlccore.a
    )
  endif(WINDOWS)
endif(LIBVLCPLUGIN)
