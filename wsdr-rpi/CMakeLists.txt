# global settings
cmake_minimum_required(VERSION 3.12)
project(wsdr VERSION 2.6.0 LANGUAGES C)

# default build type
#if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
#endif()

# prevent agressive and unsafe optimizations
# (by default CMake uses -O3 level)
#set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

SET(CMAKE_C_FLAGS "-O3 -ffast-math -funroll-loops")

add_executable(wsdr
  src/main.c
  src/spectrum.c
  src/audio_main.c
  src/dsp/cbb_main.c
  src/dsp/resample.c
  src/dsp/rf_decimator.c
  src/signal/rtl_sensor.c
  src/signal/signal_source.c
  src/tools/common.c
  src/tools/list.c
  src/tools/rate_logger.c
  src/web/http_handler.c
  src/web/ws_handler.c
)

target_link_libraries(wsdr PRIVATE rtlsdr)
target_link_libraries(wsdr PRIVATE pthread)
target_link_libraries(wsdr PRIVATE websockets)
target_link_libraries(wsdr PRIVATE rt)
target_link_libraries(wsdr PRIVATE m)
target_link_libraries(wsdr PRIVATE fftw3)

#common warnings to help encourage good coding practices
add_compile_options(-Wall)
add_compile_options(-Wextra)

# build project
#add_subdirectory(src)
#add_subdirectory(share)

# size
add_custom_command(TARGET wsdr
        POST_BUILD
        COMMAND /bin/ls -lh wsdr >> ../build.txt
        )
