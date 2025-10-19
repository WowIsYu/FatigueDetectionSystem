cmake_minimum_required(VERSION 3.16)

project(FatigueDetectionSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Multimedia Network)

# 1. 手动设置 OpenCV 路径
set(OPENCV_ROOT_PATH "D:/dependency/opencv-4.12.0/opencv/build")

# set(OpenCV_DIR "D:/dependency/opencv-4.12.0/opencv/build/x64/vc16/lib"
#     CACHE PATH "Path to OpenCVConfig.cmake")
# include路径：D:\dependency\opencv-4.12.0\opencv\build\include\opencv2

# 自动区分 Debug / Release
# 分别找到 Debug / Release 的 OpenCV 库
find_library(OpenCV_LIB_DEBUG
    NAMES opencv_world4120d
    PATHS ${OPENCV_ROOT_PATH}/x64/vc16/lib
)

find_library(OpenCV_LIB_RELEASE
    NAMES opencv_world4120
    PATHS ${OPENCV_ROOT_PATH}/x64/vc16/lib
)

# 指定多配置链接规则
set(OpenCV_LIBS
    debug ${OpenCV_LIB_DEBUG}
    optimized ${OpenCV_LIB_RELEASE}
)

message(STATUS "OpenCV Debug lib: ${OpenCV_LIB_DEBUG}")
message(STATUS "OpenCV Release lib: ${OpenCV_LIB_RELEASE}")

find_path(OPENCV_INCLUDE_DIR opencv2/core.hpp
    PATHS ${OPENCV_ROOT_PATH}/include)


# 2. 在 edit-preferences-编译套件-cmake configuration 中添加opencv的路径：
# -DOpenCV_DIR:PATH=D:\dependency\opencv-4.12.0\opencv\build\x64\vc16\lib
# 也可在project中配置


# 手动设置 ONNX Runtime 根目录
set(ONNXRUNTIME_ROOT_PATH "D:/dependency/onnxruntime-win-x64-1.23.0")

message(STATUS "ONNXRUNTIME_INCLUDE_DIR = ${ONNXRUNTIME_INCLUDE_DIR}")

find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
    PATHS ${ONNXRUNTIME_ROOT_PATH}/include)

find_library(ONNXRUNTIME_LIB onnxruntime
    PATHS ${ONNXRUNTIME_ROOT_PATH}/lib)




set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(FatigueDetectionSystem
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/core/DatabaseManager.h src/core/DatabaseManager.cpp
        src/core/DetectionEngine.h src/core/DetectionEngine.cpp
        src/core/VideoProcessor.h src/core/VideoProcessor.cpp
        src/utils/Config.h src/utils/Config.cpp
        src/ui/SettingsDialog.h src/ui/SettingsDialog.cpp
        src/ui/DetectionRecordDialog.h src/ui/DetectionRecordDialog.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET FatigueDetectionSystem APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(FatigueDetectionSystem SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(FatigueDetectionSystem
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ONNXRUNTIME_INCLUDE_DIR}
    ${OPENCV_INCLUDE_DIR}
)


target_link_libraries(FatigueDetectionSystem PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
  Qt6::Core
  Qt6::Widgets
  Qt6::Sql
  Qt6::Multimedia
  Qt6::Network
  ${OpenCV_LIBS}
  ${ONNXRUNTIME_LIB}

  # SQLite::SQLite3
)


message(STATUS "源码根 = ${CMAKE_SOURCE_DIR}")
message(STATUS "构建根 = ${CMAKE_BINARY_DIR}")
# 复制 models 目录到 build 树
# file(COPY "${CMAKE_SOURCE_DIR}/models"
#      DESTINATION "${CMAKE_BINARY_DIR}")

# 构建结束后复制 models 目录到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/models"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/models"
    COMMENT "Copying models directory to build tree ..."
)

# 复制dll文件到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_ROOT_PATH}/lib/onnxruntime.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENCV_ROOT_PATH}/x64/vc16/bin/opencv_world4120d.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)



# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.FatigueDetectionSystem)
endif()
set_target_properties(FatigueDetectionSystem PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS FatigueDetectionSystem
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(FatigueDetectionSystem)
endif()
