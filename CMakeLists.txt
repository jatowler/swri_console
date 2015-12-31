cmake_minimum_required(VERSION 2.8.3)
project(swri_console)

set(BUILD_DEPS
  rosbag_storage 
  roscpp 
  rosgraph_msgs
  )

set(RUNTIME_DEPS
  rosbag_storage
  roscpp
  rosgraph_msgs
  )
 
find_package(catkin REQUIRED COMPONENTS ${BUILD_DEPS})
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

catkin_package()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(include 
  ${catkin_INCLUDE_DIRS} 
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Widgets_INCLUDE_DIRS}
)
add_definitions(
  ${Qt5Core_DEFINITIONS}
  ${Qt5Gui_DEFINITIONS}
  ${Qt5Widgets_DEFINITIONS}
)

set(QT_USE_QTCORE TRUE)
set(QT_USE_QTGUI TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(src)

# Add Qt Designer UI files to this list.
file (GLOB SRC_FILES 
  ui/console_window.ui
  )

# Add header files containing Q_OBJECT declaration to this list.
file (GLOB SRC_FILES
  include/swri_console/bag_source.h
  include/swri_console/bag_source_backend.h
  include/swri_console/color_widget.h
  include/swri_console/console_master.h
  include/swri_console/console_window.h
  include/swri_console/log_database.h
  include/swri_console/log_filter.h
  include/swri_console/log_list_model.h
  include/swri_console/log_list_widget.h
  include/swri_console/node_list_model.h
  include/swri_console/node_list_widget.h
  include/swri_console/ros_source.h
  include/swri_console/ros_source_backend.h
  include/swri_console/save_file_dialog.h
  include/swri_console/session_list_delegate.h
  include/swri_console/session_list_model.h
  include/swri_console/session_list_widget.h
  )

file (GLOB SRC_FILES 
  src/bag_source.cpp
  src/bag_source_backend.cpp
  src/color_widget.cpp
  src/console_master.cpp
  src/console_window.cpp
  src/log.cpp
  src/log_database.cpp
  src/log_filter.cpp
  src/log_list_model.cpp
  src/log_list_widget.cpp
  src/main.cpp
  src/node_list_model.cpp
  src/node_list_widget.cpp
  src/register_meta_types.cpp 
  src/ros_source.cpp
  src/ros_source_backend.cpp
  src/save_file_dialog.cpp
  src/session_list_delegate.cpp
  src/session_list_model.cpp
  src/session_list_widget.cpp
  src/session.cpp
  src/settings_keys.cpp
  )

qt5_add_resources(RCC_SRCS resources/images.qrc)
qt5_wrap_ui(SRC_FILES ${UI_FILES})
qt5_wrap_cpp(SRC_FILES ${HEADER_FILES})

add_executable(swri_console
  ${HEADER_FILES}
  ${SRC_FILES}
  ${RCC_SRCS}
  src/main.cpp
)
target_link_libraries(swri_console
  ${Qt5Core_LIBRARIES}
  ${Qt5Gui_LIBRARIES}
  ${Qt5Widgets_LIBRARIES}
  ${catkin_LIBRARIES}
)
set_target_properties(swri_console
  PROPERTIES COMPILE_FLAGS "-std=c++0x")

add_executable(collect_rosout_bags src/collect_rosout_bags.cpp)
target_link_libraries(collect_rosout_bags
  ${QT_LIBRARIES}
  ${catkin_LIBRARIES}
  )
set_target_properties(collect_rosout_bags
  PROPERTIES COMPILE_FLAGS "-std=c++0x")

install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  FILES_MATCHING PATTERN "*.h"
)

install(TARGETS swri_console
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
)

