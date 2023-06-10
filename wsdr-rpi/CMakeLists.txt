# global settings
cmake_minimum_required(VERSION 3.12)
project(wsdr VERSION 2.6.0 LANGUAGES C)

# default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# prevent agressive and unsafe optimizations
# (by default CMake uses -O3 level)
#set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set(CMAKE_C_FLAGS "-O3 -ffast-math -funroll-loops")

set(LWS_WITH_SSL 0)
set(LWS_WITH_TLS 0)
set(LWS_WITH_STATIC 1)
set(LWS_WITH_SHARED 0)
# set(LWS_PLAT_BAREMETAL 1)
# set(LWS_WITH_SECURE_STREAMS 0)
# set(LWS_WITHOUT_EXTENSIONS 1)
# set(LWS_WITHOUT_CLIENT 1)
# set(LWS_WITH_LHP 0)
# set(LWS_WITHOUT_CLIENT 1)
set(LWS_WITHOUT_TESTAPPS 1)

# set(LWS_WITH_NETWORK 0)
# set(LWS_ONLY_SSPC 0) # remove everything except SSPC pieces from library build

add_subdirectory(libwebsockets)
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
  src/web/web_handler.c
)

# make sure we can include libwebsockets.h from the lws build
target_include_directories(wsdr PRIVATE
        # lws_config.h generated into here
        ${PROJECT_BINARY_DIR}/libwebsockets
        # source includes will do for everything else
        ${PROJECT_SOURCE_DIR}/libwebsockets/include )

# target_link_libraries(wsdr
#                         websockets)

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
        COMMAND /bin/ls -l wsdr >> ../build.txt
        )
