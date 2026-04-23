cmake_minimum_required(VERSION 3.16)

project(lvgl_runtime_demo LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(PkgConfig REQUIRED)

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(LVGL_DIR "${REPO_ROOT}/lv_port_linux_test/lvgl")
set(LVGL_GENERATED_REGISTRY_C "${CMAKE_BINARY_DIR}/generated_page_registry.c")
set(LVGL_GENERATED_REGISTRY_H "${CMAKE_BINARY_DIR}/generated_page_registry.h")
set(LVGL_GENERATED_SOURCES_CMAKE "${CMAKE_BINARY_DIR}/generated_page_sources.cmake")
set(LVGL_TASK_JSON "" CACHE FILEPATH "Optional task.json for isolated single-page builds")
set(M1_TASK_JSON "" CACHE FILEPATH "Legacy alias for LVGL_TASK_JSON")

if(NOT LVGL_TASK_JSON AND M1_TASK_JSON)
  set(LVGL_TASK_JSON "${M1_TASK_JSON}")
endif()

set(LV_CONF_DEFAULTS_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.defaults")
set(LV_BUILD_CONF_PATH "${CMAKE_BINARY_DIR}/lv_conf.h" CACHE PATH "Path to generated lv_conf.h" FORCE)
set(LV_BUILD_SET_CONFIG_OPTS ON CACHE BOOL "Expose LVGL config as CMake variables" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Disable LVGL demos" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Disable LVGL examples" FORCE)

execute_process(
  COMMAND
    ${Python3_EXECUTABLE} ${LVGL_DIR}/scripts/generate_lv_conf.py
    --template ${LVGL_DIR}/lv_conf_template.h
    --defaults ${LV_CONF_DEFAULTS_PATH}
    --config ${LV_BUILD_CONF_PATH}
  RESULT_VARIABLE config_result
  OUTPUT_VARIABLE config_output
  ERROR_VARIABLE config_output)

if(NOT config_result EQUAL 0)
  message(FATAL_ERROR "Failed to generate lv_conf.h: ${config_output}")
endif()

if(LVGL_TASK_JSON)
  execute_process(
    COMMAND
      ${Python3_EXECUTABLE} ${REPO_ROOT}/tools/sync-generated-pages.py
      --task-json ${LVGL_TASK_JSON}
      --registry-c ${LVGL_GENERATED_REGISTRY_C}
      --registry-h ${LVGL_GENERATED_REGISTRY_H}
      --cmake-out ${LVGL_GENERATED_SOURCES_CMAKE}
    RESULT_VARIABLE generated_pages_result
    OUTPUT_VARIABLE generated_pages_output
    ERROR_VARIABLE generated_pages_output)
else()
  execute_process(
    COMMAND
      ${Python3_EXECUTABLE} ${REPO_ROOT}/tools/sync-generated-pages.py
      --tasks-root ${REPO_ROOT}/workspace/tasks
      --registry-c ${LVGL_GENERATED_REGISTRY_C}
      --registry-h ${LVGL_GENERATED_REGISTRY_H}
      --cmake-out ${LVGL_GENERATED_SOURCES_CMAKE}
    RESULT_VARIABLE generated_pages_result
    OUTPUT_VARIABLE generated_pages_output
    ERROR_VARIABLE generated_pages_output)
endif()

if(NOT generated_pages_result EQUAL 0)
  message(FATAL_ERROR "Failed to sync generated pages: ${generated_pages_output}")
endif()

include(${LVGL_GENERATED_SOURCES_CMAKE} OPTIONAL)

add_subdirectory("${LVGL_DIR}" lvgl)

pkg_check_modules(FREETYPE QUIET freetype2)
if(FREETYPE_FOUND)
  target_include_directories(lvgl PRIVATE ${FREETYPE_INCLUDE_DIRS})
  target_link_directories(lvgl PRIVATE ${FREETYPE_LIBRARY_DIRS})
  target_link_libraries(lvgl PRIVATE ${FREETYPE_LIBRARIES})
else()
  message(WARNING "FreeType not found; CJK font rendering will use built-in fallback fonts")
endif()

add_executable(lvgl_runtime_demo
  ${LVGL_GENERATED_REGISTRY_C}
  ${LVGL_GENERATED_PAGE_SOURCES}
  src/home_page.c
  src/main.c
  src/page_registry.c
  src/token_page.c
  src/stitch_smart_home_panel_page.c
  src/image_converter_page.c
  src/copy_scan_print_setup_page.c
  src/ui_font.c
)

target_include_directories(lvgl_runtime_demo PRIVATE
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_definitions(lvgl_runtime_demo PRIVATE
  TOKEN_PROVIDER_ROW_IMAGE_PATH=\"A:${CMAKE_CURRENT_LIST_DIR}/assets/provider_icons_live.png\"
)

target_link_libraries(lvgl_runtime_demo PRIVATE
  lvgl
  m
  pthread
)

if(UNIX AND NOT APPLE)
  target_link_libraries(lvgl_runtime_demo PRIVATE rt)
endif()
