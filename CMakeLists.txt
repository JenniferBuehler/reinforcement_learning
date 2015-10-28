# CMakeLists files in this project can
# refer to the root source directory of the project as ${RL_SOURCE_DIR} and
# to the root binary directory of the project as ${RL_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (RL)


ADD_DEFINITIONS(
    -std=c++11 -g 
)



add_subdirectory (src)
add_subdirectory (include)

include_directories (include PUBLIC)


# add a target to generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
	${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)


# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found.
add_executable (demoGridWorld src/main.cpp src/Exception.cpp src/RandomNumber.cpp)

