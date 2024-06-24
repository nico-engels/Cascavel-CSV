#~~~ Cascavel_CSV - Nicolas Engels

cmake_minimum_required(VERSION 3.16)
project(cascavel_csv VERSION 1)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unqualified-std-cast-call)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

set(csc_csv_srcs
  include/console.h    src/console.cpp
  include/rol.h        src/rol.cpp
  include/sqlite_api.h src/sqlite_api.cpp
  include/str_util.h   src/str_util.cpp
  src/main.cpp
)

#link_libraries("-static")

add_executable(cascavel_csv ${csc_csv_srcs})

find_package(SQLite3 REQUIRED)
target_link_libraries(cascavel_csv PUBLIC SQLite::SQLite3)

target_include_directories(cascavel_csv PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_property(TARGET cascavel_csv PROPERTY CXX_STANDARD 23)
