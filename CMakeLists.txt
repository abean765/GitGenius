cmake_minimum_required(VERSION 3.16)
project(GitGenius LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QML_DEBUG TRUE)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR OFF CACHE BOOL "" FORCE) # libgit2-spezifische Tests
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

add_subdirectory(3rdparty/libgit2)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup()

qt_add_executable(appGitGenius
    src/main.cpp
)

qt_add_qml_module(appGitGenius
    URI GitGenius
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/RepositoryHeader.qml
        qml/StatusList.qml
        qml/SubmoduleList.qml
        qml/RepositoryTreeNode.qml
        qml/RepositoryTreeView.qml
        qml/GitCommandDialog.qml
        qml/CommitHistoryView.qml
    RESOURCES
        assets/icons/repository.svg
        assets/icons/branch.svg
        assets/icons/submodule.svg
    SOURCES
        src/gitclientbackend.h src/gitclientbackend.cpp
        src/commithistorymodel.h src/commithistorymodel.cpp
        SOURCES
        SOURCES src/backend.h src/backend.cpp
)

set_target_properties(appGitGenius PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)


target_link_libraries(appGitGenius
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        libgit2package
)

target_include_directories(appGitGenius PRIVATE 3rdparty/libgit2/include)

install(TARGETS appGitGenius)
