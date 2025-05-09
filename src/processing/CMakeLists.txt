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
target_link_libraries(cprimim-lib
   PRIVATE OpenMP::OpenMP_C)

set_target_properties(cprimim-lib PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
set_target_properties(cprimim-lib PROPERTIES
  COMPILE_FLAGS "${CMAKE_C_FLAGS_PROFILE}"
  LINK_FLAGS    "-pg"
)
