cmake_minimum_required(VERSION 3.16)

project(d2rreg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(MSVC_TOOLSET_VERSION 145)

add_executable(d2rreg main.cpp)

target_link_libraries(d2rreg PRIVATE crypt32)

install(TARGETS d2rreg
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
