cmake_minimum_required(VERSION 3.21)
project(md VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network Test)
qt_standard_project_setup()

if(APPLE)
  enable_language(OBJCXX)
endif()

include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_MD2HTML_EXECUTABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(md4c
  GIT_REPOSITORY https://github.com/mity/md4c.git
  GIT_TAG release-0.5.2
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(md4c)

add_library(md_core STATIC
  src/core/Title.cpp
  src/core/BufferStore.cpp
  src/core/Markdown.cpp
  src/core/PdfExporter.cpp
  src/core/RelativeTime.cpp)
target_include_directories(md_core PUBLIC src)
target_include_directories(md_core PRIVATE ${md4c_SOURCE_DIR}/src)
target_link_libraries(md_core PUBLIC Qt6::Core Qt6::Gui md4c-html)
qt_add_resources(md_core md_resources PREFIX "/" BASE resources
  FILES resources/preview.css resources/print.css
        resources/icons/tray-mac.png resources/icons/tray-mac@2x.png
        resources/icons/tray-linux.png)

if(APPLE)
  set(MD_DOCK_SOURCES src/platform/mac/Dock.mm)
else()
  set(MD_DOCK_SOURCES src/platform/Dock.cpp)
endif()

add_library(md_app STATIC
  ${MD_DOCK_SOURCES}
  src/platform/Reveal.cpp
  src/platform/SingleInstance.cpp
  src/platform/Tray.cpp
  src/ui/AppMenu.cpp
  src/ui/MainWindow.cpp
  src/ui/BufferListDelegate.cpp
  src/ui/EditorView.cpp
  src/ui/PreviewView.cpp
  src/ui/OverlayBar.cpp)
target_link_libraries(md_app PUBLIC md_core Qt6::Widgets Qt6::Network)
if(APPLE)
  target_link_libraries(md_app PUBLIC "-framework AppKit")
endif()

qt_add_executable(md MACOSX_BUNDLE src/main.cpp)
target_link_libraries(md PRIVATE md_app)
set_target_properties(md PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER dev.yourname.md
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
if(APPLE)
  set_target_properties(md PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in)
  target_sources(md PRIVATE resources/icons/md.icns)
  set_source_files_properties(resources/icons/md.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
endif()

enable_testing()

qt_add_executable(tst_window tests/tst_window.cpp)
target_link_libraries(tst_window PRIVATE md_app Qt6::Test)
add_test(NAME window COMMAND tst_window)

foreach(t title bufferstore markdown pdf relativetime lifecycle)
  qt_add_executable(tst_${t} tests/tst_${t}.cpp)
  target_link_libraries(tst_${t} PRIVATE md_core Qt6::Test)
  add_test(NAME ${t} COMMAND tst_${t})
endforeach()

qt_add_executable(tst_singleinstance tests/tst_singleinstance.cpp)
target_link_libraries(tst_singleinstance PRIVATE md_app Qt6::Test)
add_test(NAME singleinstance COMMAND tst_singleinstance)

qt_add_executable(tst_packaging tests/tst_packaging.cpp)
target_link_libraries(tst_packaging PRIVATE Qt6::Core Qt6::Test)
target_compile_definitions(tst_packaging PRIVATE MD_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME packaging COMMAND tst_packaging)

set_tests_properties(window title bufferstore markdown pdf relativetime lifecycle singleinstance packaging
  PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
