cmake_minimum_required(VERSION 3.10)

project(Robinwaita LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK REQUIRED gtk4)
pkg_check_modules(ADWAITA REQUIRED libadwaita-1)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIOUNIX REQUIRED gio-unix-2.0)
pkg_check_modules(GRAPHENE REQUIRED graphene-1.0)

include_directories(
        ${Python3_INCLUDE_DIRS}
        ${GIOUNIX_INCLUDE_DIRS}
        ${GTK_INCLUDE_DIRS}
        ${ADWAITA_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
        ${GRAPHENE_INCLUDE_DIRS}
)

add_executable(robinwaita
        main.c
        process.c
        process.h
        scheduler.c
        scheduler.h
        main.h
)

target_link_libraries(robinwaita
        ${Python3_LIBRARIES}
        ${GTK_LIBRARIES}
        ${ADWAITA_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${GIOUNIX_LIBRARIES}
        ${GRAPHENE_LIBRARIES}
)

link_directories(
        ${GTK_LIBRARY_DIRS}
        ${ADWAITA_LIBRARY_DIRS}
        ${GLIB_LIBRARY_DIRS}
        ${GRAPHENE_LIBRARY_DIRS}
        ${GIOUNIX_LIBRARY_DIRS}
)

target_compile_options(robinwaita PRIVATE ${GTK_CFLAGS_OTHER})