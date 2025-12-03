cmake_minimum_required(VERSION 3.5)

project(SkyLinkPro LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 匹配你的Qt6版本
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)

# 直接写根目录的文件名（因为文件都在根目录，不用加任何文件夹）
add_executable(SkyLinkPro
  main.cpp
  mainwindow.cpp
  mainwindow.h
  mainwindow.ui
  db_conn.h
  theme.h
  city_selector.h
  city_selector.cpp
  seat_selection_page.h
  seat_selection_page.cpp
  app_style.h
  login_dialog.h
  login_dialog.cpp
)

# 链接Qt模块
target_link_libraries(SkyLinkPro PRIVATE Qt6::Core Qt6::Widgets Qt6::Sql)

# 头文件在根目录，所以包含当前目录
target_include_directories(SkyLinkPro PRIVATE ".")
