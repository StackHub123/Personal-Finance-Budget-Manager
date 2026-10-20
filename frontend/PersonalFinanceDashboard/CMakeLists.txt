cmake_minimum_required(VERSION 3.21)
project(PersonalFinanceFrontend VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

qt_add_executable(PersonalFinanceFrontend
    main.cpp
    mainwindow.h
    mainwindow.cpp
)

target_link_libraries(PersonalFinanceFrontend PRIVATE Qt6::Widgets)
