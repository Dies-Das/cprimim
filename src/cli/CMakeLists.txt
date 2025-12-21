add_executable(cprimim
    cli.c
    video_cli.c
    image_cli.c
    flag_impl.c
    stb_impl.c
)

target_include_directories(cprimim
	PRIVATE
	${CMAKE_SOURCE_DIR}/include

	${CMAKE_SOURCE_DIR}/src/cli/third_party/stb

	${CMAKE_SOURCE_DIR}/src/cli/third_party/flag

)


target_link_libraries(cprimim
  PRIVATE
    cprimim-lib
    m
    avcodec
    avformat
    avutil
    swscale

)
set_target_properties(cprimim PROPERTIES
  COMPILE_FLAGS "${CMAKE_C_FLAGS_PROFILE}"
  LINK_FLAGS    "-pg"
)
