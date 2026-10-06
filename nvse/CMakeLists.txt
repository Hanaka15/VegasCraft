cmake_minimum_required(VERSION 3.25)

project(VegasCraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fallout New Vegas plugins are 32-bit.
if(MSVC)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		message(FATAL_ERROR "VegasCraft NVSE plugin must be built as Win32 (32-bit). Use -A Win32.")
	endif()
	# Static CRT: MSVCP140/filesystem under Wine/Proton crashes during NVSE Load.
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

file(GLOB VEGASCRAFT_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)

add_library(vegascraft SHARED ${VEGASCRAFT_SOURCES} exports.def)
target_include_directories(vegascraft PRIVATE
	src
	include
	${CMAKE_CURRENT_SOURCE_DIR}/../protocol
)
target_compile_definitions(vegascraft PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
if(MSVC)
	target_compile_options(vegascraft PRIVATE /W4 /permissive- /Zc:preprocessor /utf-8 /MP)
	target_link_libraries(vegascraft PRIVATE Shell32 d3d9)
	set_target_properties(vegascraft PROPERTIES OUTPUT_NAME "vegascraft")
endif()

# Optional: copy into an FNV Data folder after build (-DVEGASCRAFT_DEPLOY_DIR=...)
set(VEGASCRAFT_DEPLOY_DIR "" CACHE PATH "FNV Data folder (plugin goes to NVSE/Plugins)")
if(VEGASCRAFT_DEPLOY_DIR AND EXISTS "${VEGASCRAFT_DEPLOY_DIR}")
	add_custom_command(TARGET vegascraft POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${VEGASCRAFT_DEPLOY_DIR}/NVSE/Plugins"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:vegascraft>"
			"${VEGASCRAFT_DEPLOY_DIR}/NVSE/Plugins/vegascraft.dll"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/VegasCraft.ini"
			"${VEGASCRAFT_DEPLOY_DIR}/NVSE/Plugins/VegasCraft.ini"
		VERBATIM)
endif()
