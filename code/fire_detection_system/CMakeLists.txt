cmake_minimum_required(VERSION 3.4.1)
project(FireDetectionSystem)

# 编译选项
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

# 查找 OpenCV
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")

# 查找 OpenMP
find_package(OpenMP REQUIRED)
if(OPENMP_FOUND)
    message(STATUS "OpenMP found: ${OpenMP_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# 查找 ncnn
find_package(ncnn REQUIRED)
if(NOT TARGET ncnn)
    message(WARNING "ncnn NOT FOUND! 请设置 ncnn_DIR 环境变量")
else()
    message(STATUS "ncnn found")
endif()

# 包含目录
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/get_heat/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/object_detection
)

# 源文件列表
set(SOURCES
    fire_detection_system.cpp
    thermal_module.cpp
    detection_module.cpp
    get_heat/functions/MLX90640_API.cpp
    get_heat/functions/MLX90640_I2C_Driver.cpp
    get_heat/functions/pseudo_color.cpp
    object_detection/nanodet.cpp
)

# 最终可执行文件
add_executable(FireDetectionSystem ${SOURCES})

# 链接库
target_link_libraries(FireDetectionSystem
    PUBLIC
        ${OpenCV_LIBS}
        ncnn
        OpenMP::OpenMP_CXX
)

