cmake_minimum_required(VERSION 3.16)

project(FatigueDetectionSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Multimedia Network)
find_package(OpenCV REQUIRED)
# find_package(SQLite3 REQUIRED)

# 在 edit-preferences-编译套件-cmake configuration 中添加opencv的路径：
# -DOpenCV_DIR:PATH=D:/develop_software/opencv-4.12.0/opencv/build/x64/vc16/lib
# 也可在project中配置
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




# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ONNXRUNTIME_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
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
