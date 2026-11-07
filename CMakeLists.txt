cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED ANKH_VERSION)
  message(FATAL_ERROR "ANKH_VERSION is required, for example 2026.11.07.1530")
endif()
if(NOT DEFINED ANKH_INNO_VERSION)
  message(FATAL_ERROR "ANKH_INNO_VERSION is required, for example 6.7.3")
endif()
if(NOT ANKH_VERSION MATCHES "^([0-9][0-9][0-9][0-9])\\.([0-9][0-9])\\.([0-9][0-9])\\.([0-9][0-9][0-9][0-9])$")
  message(FATAL_ERROR "ANKH_VERSION must be YYYY.MM.DD.HHMM, got ${ANKH_VERSION}")
endif()
set(ankhVersionParts "${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}" "${CMAKE_MATCH_3}" "${CMAKE_MATCH_4}")
set(partIndex 1)
foreach(part IN LISTS ankhVersionParts)
  string(REGEX REPLACE "^0+([0-9])" "\\1" ANKH_V${partIndex} "${part}")
  math(EXPR partIndex "${partIndex} + 1")
endforeach()

project(ankh VERSION ${ANKH_V1}.${ANKH_V2}.${ANKH_V3}.${ANKH_V4} LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg SvgWidgets Network)
qt_standard_project_setup()

configure_file(src/appVersion.h.in appVersion.h @ONLY)
configure_file(src/thirdParty.json.in thirdParty.json @ONLY)
configure_file(src/ankh.rc.in ankh.rc @ONLY)

qt_add_executable(ankh WIN32
  src/main.cpp
  src/fatal.cpp
  src/calendarVersion.cpp
  src/captionButton.cpp
  src/windowControls.cpp
  src/icons.cpp
  src/edgeGrain.cpp
  src/palette.cpp
  src/gameCatalog.cpp
  src/libraryPane.cpp
  src/gamePage.cpp
  src/settingsPane.cpp
  src/settingsPage.cpp
  src/attributionsView.cpp
  src/updateChecker.cpp
  src/updateSheet.cpp
  src/mainWindow.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/ankh.rc
)

target_include_directories(ankh PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(ankh PRIVATE Qt6::Widgets Qt6::Svg Qt6::SvgWidgets Qt6::Network dwmapi)
target_compile_definitions(ankh PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)

file(GLOB glyphFiles CONFIGURE_DEPENDS assets/glyphs/*.svg)
file(GLOB textureFiles CONFIGURE_DEPENDS assets/textures/*.png)
set(resourceFiles assets/ankhWordmarkGold.svg assets/ankh.ico ${glyphFiles} ${textureFiles} licenses/lgpl-3.0.txt licenses/gpl-3.0.txt licenses/innoSetup.txt)
foreach(resourceFile IN LISTS resourceFiles)
  get_filename_component(resourceName "${resourceFile}" NAME)
  get_filename_component(resourceFolder "${resourceFile}" DIRECTORY)
  get_filename_component(resourceFolder "${resourceFolder}" NAME)
  if(resourceFolder STREQUAL "glyphs" OR resourceFolder STREQUAL "textures" OR resourceFolder STREQUAL "licenses")
    set_source_files_properties(${resourceFile} PROPERTIES QT_RESOURCE_ALIAS ${resourceFolder}/${resourceName})
  else()
    set_source_files_properties(${resourceFile} PROPERTIES QT_RESOURCE_ALIAS ${resourceName})
  endif()
endforeach()
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/thirdParty.json PROPERTIES QT_RESOURCE_ALIAS thirdParty.json)
qt_add_resources(ankh resources PREFIX "/" FILES ${resourceFiles} ${CMAKE_CURRENT_BINARY_DIR}/thirdParty.json)