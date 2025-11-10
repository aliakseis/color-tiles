cmake_minimum_required(VERSION 3.8.2)

project(Tiles)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if(WIN32)
  set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# Prefer Qt6 if available, otherwise fall back to Qt5
find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
  find_package(Qt5 COMPONENTS Widgets REQUIRED)
  set(USE_QT5 TRUE)
endif()

# Resources
if(DEFINED Qt6_FOUND)
  qt_add_resources(QRCS tiles.qrc)
else()
  qt5_add_resources(QRCS tiles.qrc)
endif()

add_executable(Tiles
  main.cpp
  tiles.cpp
  solver.cpp
  TilesArea.cpp
  tiles.ui
  ${QRCS}
)

if(USE_QT5)
  target_link_libraries(Tiles PRIVATE Qt5::Widgets)
else()
  target_link_libraries(Tiles PRIVATE Qt6::Widgets)
endif()

# Windows packaging left mostly unchanged
if(MSVC)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/bin")
  set(BINARY_INSTALL_DIR .)
  install(TARGETS ${PROJECT_NAME} DESTINATION ${BINARY_INSTALL_DIR})
  set(BINARY_FILE "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe")
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
  set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
  set(CMAKE_INSTALL_OPENMP_LIBRARIES TRUE)
  include (InstallRequiredSystemLibraries)

  if (QT_QMAKE_EXECUTABLE)
    get_filename_component(QT_BIN_DIRECTORY ${QT_QMAKE_EXECUTABLE} DIRECTORY)
    install(CODE "execute_process(COMMAND \"${QT_BIN_DIRECTORY}/windeployqt\"  \"${BINARY_FILE}\" WORKING_DIRECTORY \"${QT_BIN_DIRECTORY}\")")
  else()
    install(CODE "execute_process(COMMAND \"windeployqt\"  \"${BINARY_FILE}\" )")
  endif()
endif()
