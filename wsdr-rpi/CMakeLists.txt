cmake_minimum_required(VERSION 3.0.0)
project(wsdr VERSION 0.1.0)

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


