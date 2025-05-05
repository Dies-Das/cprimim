add_library(cprimim-lib
    image.c

    processing.c
    line.c

    utils.c
    point.c
)


target_include_directories(cprimim-lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE

    ${CMAKE_SOURCE_DIR}/src/processing/internal
)


set_target_properties(cprimim-lib PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

