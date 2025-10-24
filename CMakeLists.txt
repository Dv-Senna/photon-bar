cmake_minimum_required(VERSION 3.30)

project(photon-bar
	VERSION 0.0.1
	LANGUAGES CXX
)


add_subdirectory(vendors/xdg-shell)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(photon-bar ${SOURCE_FILES})
target_compile_features(photon-bar PRIVATE cxx_std_26)
target_link_libraries(photon-bar PRIVATE
	wayland-client
	xdg-shell::xdg-shell
)
target_include_directories(photon-bar PRIVATE src)
