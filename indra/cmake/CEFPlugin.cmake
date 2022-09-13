# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (STANDALONE)
    set(CEFPLUGIN OFF CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
else (STANDALONE)
    use_prebuilt_binary(dullahan)
    set(CEFPLUGIN ON CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
        set(CEF_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/cef)
endif (STANDALONE)

if (WINDOWS)
    set(CEF_PLUGIN_LIBRARIES
        ${ARCH_PREBUILT_DIRS_RELEASE}/libcef.lib
        ${ARCH_PREBUILT_DIRS_RELEASE}/libcef_dll_wrapper.lib
        ${ARCH_PREBUILT_DIRS_RELEASE}/dullahan.lib
    )
elseif (DARWIN)
    FIND_LIBRARY(APPKIT_LIBRARY AppKit)
    if (NOT APPKIT_LIBRARY)
        message(FATAL_ERROR "AppKit not found")
    endif()

    set(CEF_PLUGIN_LIBRARIES
        ${ARCH_PREBUILT_DIRS_RELEASE}/libcef_dll_wrapper.a
        ${ARCH_PREBUILT_DIRS_RELEASE}/libdullahan.a
        ${APPKIT_LIBRARY}
       )

elseif (LINUX)
    set(CEF_PLUGIN_LIBRARIES
        dullahan
        cef
        cef_dll_wrapper.a
    )
endif (WINDOWS)
