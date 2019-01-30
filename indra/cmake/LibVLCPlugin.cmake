# -*- cmake -*-
include(Linking)
include(Prebuilt)
include(Variables)

if (LIBVLCPLUGIN)
if (STANDALONE)
else (STANDALONE)
    use_prebuilt_binary(vlc-bin)
    set(VLC_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/vlc)
endif (STANDALONE)

if (WINDOWS)
    set(VLC_PLUGIN_LIBRARIES
        libvlc.lib
        libvlccore.lib
    )
elseif (DARWIN)
elseif (LINUX)
    # Specify a full path to make sure we get a static link
    set(VLC_PLUGIN_LIBRARIES
        ${LIBS_PREBUILT_DIR}/lib/libvlc.a
        ${LIBS_PREBUILT_DIR}/lib/libvlccore.a
    )
endif (WINDOWS)
endif (LIBVLCPLUGIN)
