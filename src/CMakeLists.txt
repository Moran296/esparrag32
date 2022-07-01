# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/common/*.cpp ${CMAKE_SOURCE_DIR}/network/*.cpp ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/drivers/*.cpp ${CMAKE_SOURCE_DIR}/modules/*.cpp)
set(include_dirs ${CMAKE_SOURCE_DIR}/network/ ${CMAKE_SOURCE_DIR}/src/ ${CMAKE_SOURCE_DIR}/common/ ${CMAKE_SOURCE_DIR}/drivers/ ${CMAKE_SOURCE_DIR}/modules/)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS ${include_dirs})

