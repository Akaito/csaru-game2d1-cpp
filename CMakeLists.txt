cmake_minimum_required(VERSION 3.2)

set(src_files
	src/main.cpp
	)

set(header_files
	)

# CSaruDir must be set in your environment variables!  It should point to a
#	directory with src/bin/pkg directories (or at least src; others can be
#	created later).  Open CSaru.cmake in a text editor to see what it's doing.
#	CSaru_Lib() has been written with the intent of being broken out in this
#	file if you need finer control over your project.
include($ENV{CSaruDir}/cmake/CSaru.cmake)
# Pass the version of your project; it's not related to the CSaruEnviron.
CSaru_Bin_Console(1.5 "${src_files}" "${header_files}")

# Custom stuff hand-edited into this CMakeLists ...

set(CXX_STANDARD_REQUIRED true)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

CSaru_UseVendor("CML")

CSaru_Depends("csaru-game2dlib-cpp")
CSaru_Depends("csaru-json-cpp")
CSaru_Depends("csaru-container-cpp")
CSaru_Depends("csaru-core-cpp")

if (UNIX)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
endif()

set(CMAKE_BUILD_TYPE "Debug")


# Find PhysicsFS (by Icculus)
if (UNIX)
	include(FindPhysFS REQUIRED)
	target_link_libraries(${PROJECT_NAME} ${PHYSFS_LIBRARY})
else()
	message(SEND_ERROR "Non-*nix platform; PhysicsFS-finding style unknown.")
endif()


# The SDL2-finding code below is from
# http://stackoverflow.com/questions/23850472/how-to-use-sdl2-and-sdl-image-with-cmake
if (UNIX)
	include(FindPkgConfig)
	pkg_search_module(SDL2 REQUIRED sdl2)
	pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
	pkg_search_module(SDL2TTF REQUIRED SDL2_ttf>=2.0.0)
else()
	message(SEND_ERROR "Non-*nix platform; SDL2-finding style unknown.")
endif()

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES} ${SDL2TTF_LIBRARIES})

