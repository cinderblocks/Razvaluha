# -*- cmake -*-
include(Prebuilt)
include(Variables)

if (NVAPI)
  if (WINDOWS)
    use_prebuilt_binary(nvapi)
    if (ADDRESS_SIZE EQUAL 32)
      set(NVAPI_LIBRARY nvapi)
    elseif (ADDRESS_SIZE EQUAL 64)
      set(NVAPI_LIBRARY nvapi64)
    endif (ADDRESS_SIZE EQUAL 32)	
  else (WINDOWS)
    set(NVAPI_LIBRARY "")
  endif (WINDOWS)
else (NVAPI)
  set(NVAPI_LIBRARY "")
endif (NVAPI)

