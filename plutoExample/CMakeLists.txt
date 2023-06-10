cmake_minimum_required(VERSION 3.12)

project(ad9361-iiostream C)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIIO_CHECK_RET")
endif ()

# set sysroot
# SET(CMAKE_SYSROOT files/sysroot)

add_executable(
    ad9361-iiostream src/ad9361-iiostream.c ${GETOPT_C_FILE} ${LIBIIO_RC}
)

target_link_libraries(ad9361-iiostream iio)

find_library(PTHREAD_LIBRARIES pthread)
find_library(CURSES_LIBRARY curses)
find_library(CDK_LIBRARY cdk)

include(CheckSymbolExists)

if (PTHREAD_LIBRARIES AND CURSES_LIBRARY AND CDK_LIBRARY)
    find_path(LIBCKD_INCLUDE_DIR cdk.h PATH_SUFFIXES cdk)

    set(TEMP ${CMAKE_REQUIRED_LIBRARIES})
    set(TEMP1 ${CMAKE_REQUIRED_INCLUDES})
    list(APPEND CMAKE_REQUIRED_LIBRARIES ${CURSES_LIBRARY} ${CDK_LIBRARY})
    list(APPEND CMAKE_REQUIRED_INCLUDES ${LIBCKD_INCLUDE_DIR})
    set(TMP_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS "")
    set(CMAKE_C_FLAGS "${TMP_FLAGS}")
    set(CMAKE_REQUIRED_LIBRARIES ${TEMP})
    set(CMAKE_REQUIRED_INCLUDES ${TEMP1})
endif()

set_target_properties(
    ${IIO_TESTS_TARGETS} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON
                                    C_EXTENSIONS OFF
)

# We don't install any of these, they are examples, not tools. We build them to
# make sure they build/work, not to deploy them.
