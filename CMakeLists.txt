cmake_minimum_required(VERSION 3.20)

project(birthday_countdown CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define standard GNU installation path variables (CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_DATADIR, etc.)
include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(ADWAITA REQUIRED IMPORTED_TARGET libadwaita-1)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        PkgConfig::GTK4
        PkgConfig::ADWAITA
        PkgConfig::GLIB
)

# -----------------------------------------------------------------------------
# Local Schema Compilation
# -----------------------------------------------------------------------------
find_program(GLIB_COMPILE_SCHEMAS glib-compile-schemas REQUIRED)

add_custom_target(compile_schemas ALL
    COMMAND ${GLIB_COMPILE_SCHEMAS} ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling local GSettings schema..."
)

add_dependencies(${PROJECT_NAME} compile_schemas)

# -----------------------------------------------------------------------------
# Installation Rules
# -----------------------------------------------------------------------------

# 1. Install Executable Binary -> /usr/local/bin
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 2. Install GSettings Schema -> /usr/local/share/glib-2.0/schemas
install(FILES org.gnome.BirthdayCountdown.gschema.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas
)

# 3. Install SVG Icon -> /usr/local/share/icons/hicolor/scalable/apps/
install(FILES bdaycountdown.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME org.gnome.BirthdayCountdown.svg
)

# 4. Install Desktop Launcher -> /usr/local/share/applications/
install(FILES org.gnome.BirthdayCountdown.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

# -----------------------------------------------------------------------------
# Post-Install System Cache Updates
# -----------------------------------------------------------------------------
find_program(GTK_UPDATE_ICON_CACHE gtk-update-icon-cache)
find_program(UPDATE_DESKTOP_DATABASE update-desktop-database)

install(CODE "
    message(STATUS \"Updating GSettings schemas...\")
    execute_process(
        COMMAND ${GLIB_COMPILE_SCHEMAS} \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas
        OUTPUT_QUIET ERROR_QUIET
    )
")

if(GTK_UPDATE_ICON_CACHE)
    install(CODE "
        message(STATUS \"Updating GTK icon cache...\")
        execute_process(
            COMMAND ${GTK_UPDATE_ICON_CACHE} -f -t \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons/hicolor
            OUTPUT_QUIET ERROR_QUIET
        )
    ")
endif()

if(UPDATE_DESKTOP_DATABASE)
    install(CODE "
        message(STATUS \"Updating desktop application database...\")
        execute_process(
            COMMAND ${UPDATE_DESKTOP_DATABASE} \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/applications
            OUTPUT_QUIET ERROR_QUIET
        )
    ")
endif()
