# ==============================================================================
#  UserProject.cmake
# ==============================================================================

# ------------------------------------------------------------------------------
# 源文件 (.c)
# ------------------------------------------------------------------------------
file(GLOB_RECURSE USER_RECURSE_SOURCES 
    CONFIGURE_DEPENDS 
    "App/*.c" 
    "Bsp/*.c"
    "Middlewares/Third_Party/MultiButton/*.c"
)

# ------------------------------------------------------------------------------
# 头文件 (.h)
# ------------------------------------------------------------------------------
file(GLOB_RECURSE ALL_USER_HEADERS "App/*.h" "Bsp/*.h" "Middlewares/Third_Party/MultiButton/*.h")
set(USER_INCLUDE_DIRS "")

foreach(_header_file ${ALL_USER_HEADERS})
    get_filename_component(_dir ${_header_file} DIRECTORY)
    list(APPEND USER_INCLUDE_DIRS ${_dir})
endforeach()

if(USER_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES USER_INCLUDE_DIRS)
endif()


# ------------------------------------------------------------------------------
# 第三方库：LVGL
# ------------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/Middlewares/lvgl/CMakeLists.txt")
    add_subdirectory(Middlewares/lvgl)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE lvgl)
endif()

# ------------------------------------------------------------------------------
# 应用配置
# ------------------------------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${USER_RECURSE_SOURCES})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${USER_INCLUDE_DIRS})

# 启用 FPU 硬件加速
add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)