# -*- cmake -*-
include(Prebuilt)
include(Linking)

set(OpenSSL_FIND_QUIETLY ON)
set(OpenSSL_FIND_REQUIRED ON)

if (STANDALONE OR USE_SYSTEM_OPENSSL)
  include(FindOpenSSL)
else (STANDALONE OR USE_SYSTEM_OPENSSL)
  use_prebuilt_binary(openssl)
  if (WINDOWS)
    set(SSL_LIBRARY
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libssl.lib
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libssl.lib)
    set(CRYPTO_LIBRARY
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libcrypto.lib
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libcrypto.lib)
    set(OPENSSL_LIBRARIES ${SSL_LIBRARY} ${CRYPTO_LIBRARY} Crypt32.lib)
  else (WINDOWS)
    set(OPENSSL_LIBRARIES ssl crypto)
  endif (WINDOWS)
  set(OPENSSL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE OR USE_SYSTEM_OPENSSL)

if (LINUX)
  set(CRYPTO_LIBRARIES crypto dl)
elseif (DARWIN)
  set(CRYPTO_LIBRARIES crypto)
endif (LINUX)
