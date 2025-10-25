cmake_minimum_required(VERSION 3.20)

project(glad
	LANGUAGES C
)

add_library(glad ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(glad::glad ALIAS glad)
