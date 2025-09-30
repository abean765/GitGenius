cmake_minimum_required(VERSION 3.16)
project(GitGenius LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    RESOURCES
        assets/icons/repository.svg
        assets/icons/branch.svg
        assets/icons/submodule.svg
    SOURCES
        src/gitclientbackend.h src/gitclientbackend.cpp
)

set_target_properties(appGitGenius PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appGitGenius
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

install(TARGETS appGitGenius)
