add_library(cprimim-lib
    triangulation.c
    image.c
    bezier.c
    # bezier_cast.c
    processing.c
    line.c
    triangle.c
    delaunay.c
    utils.c
    third_party/inih/ini.c
)


target_include_directories(cprimim-lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE

    ${CMAKE_SOURCE_DIR}/src/processing/internal
    ${CMAKE_SOURCE_DIR}/src/processing/third_party
)
target_link_libraries(cprimim-lib
   PRIVATE cairo)

set_target_properties(cprimim-lib PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
set_target_properties(cprimim-lib PROPERTIES
  COMPILE_FLAGS "${CMAKE_C_FLAGS_PROFILE}"
  LINK_FLAGS    "-pg"
)
