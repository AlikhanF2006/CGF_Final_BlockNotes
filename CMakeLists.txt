cmake_minimum_required(VERSION 3.16)
project(BlockNotes VERSION 0.7.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BLOCKNOTES_SYSTEM_GLFW "Use installed GLFW instead of bundled source" OFF)
if(BLOCKNOTES_SYSTEM_GLFW)
  find_package(glfw3 3.3 REQUIRED)
else()
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
  add_subdirectory(third_party/glfw)
endif()
add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)
add_executable(BlockNotes
  src/main.cpp            # window, input, interface (app.hpp)
  src/camera_scene.cpp    # camera, scene, particles, light, picking, selection
  src/page_curl.cpp       # page turn, curl mesh, book model
  src/dialogs.cpp         # sign, cover, load/delete dialogs
  src/writing_desk.cpp    # right panel: ink, font, size, scene, light
  src/smoke_test.cpp      # automated screenshot / interaction test
  src/graphics.cpp        # renderer, shadow mapping
  src/font.cpp            # text rendering, procedural page/cover textures
  src/document.cpp)       # text layout, styles, JSON book format
target_include_directories(BlockNotes PRIVATE src third_party third_party/glm)
target_link_libraries(BlockNotes PRIVATE glfw glad ${CMAKE_DL_LIBS})
if(MSVC)
  target_compile_options(BlockNotes PRIVATE /W4 /utf-8)
else()
  target_compile_options(BlockNotes PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_custom_command(TARGET BlockNotes POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:BlockNotes>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:BlockNotes>/shaders")
include(CTest)
if(BUILD_TESTING)
  add_executable(document_tests tests/document_tests.cpp src/document.cpp)
  target_include_directories(document_tests PRIVATE src third_party)
  add_test(NAME document_tests COMMAND document_tests)
endif()
