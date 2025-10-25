cmake_minimum_required(VERSION 3.20)

project(wlr-layer-shell-unstable-v1
	VERSION 1.0.0
	LANGUAGES C
)


set(HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/include/wlr-layer-shell-unstable-v1/wlr-layer-shell-unstable-v1-protocol.h)
set(INTERMEDIATE_HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-protocol.h)
set(SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/src/wlr-layer-shell-unstable-v1-protocol.c)
set(CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/protocol/wlr-layer-shell-unstable-v1.xml)

make_directory(${CMAKE_CURRENT_BINARY_DIR}/include/wlr-layer-shell-unstable-v1)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/src)

add_custom_command(
	OUTPUT
		${INTERMEDIATE_HEADER_FILE}
	COMMAND
		wayland-scanner client-header ${CONFIG_FILE} ${INTERMEDIATE_HEADER_FILE}
)
add_custom_command(
	OUTPUT
		${HEADER_FILE}
	COMMAND
		${CMAKE_COMMAND}
			-DSOURCE=${INTERMEDIATE_HEADER_FILE}
			-DOUTPUT=${HEADER_FILE}
			-DPATTERN=namespace
			-DREPLACE=namespace_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/replace.cmake
	DEPENDS
		${INTERMEDIATE_HEADER_FILE}
)

add_custom_command(
	OUTPUT
		${SOURCE_FILE}
	COMMAND
		wayland-scanner private-code ${CONFIG_FILE} ${SOURCE_FILE}
)

add_custom_target(wlr-layer-shell-unstable-v1-generator DEPENDS ${HEADER_FILE} ${SOURCE_FILE})

add_library(wlr-layer-shell-unstable-v1 STATIC ${SOURCE_FILE})
target_include_directories(wlr-layer-shell-unstable-v1 PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(wlr-layer-shell-unstable-v1 PUBLIC xdg-shell::xdg-shell)
add_dependencies(wlr-layer-shell-unstable-v1 wlr-layer-shell-unstable-v1-generator)
add_library(wlr-layer-shell-unstable-v1::wlr-layer-shell-unstable-v1 ALIAS wlr-layer-shell-unstable-v1)
