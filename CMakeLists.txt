find_package(Qt5Widgets)
cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

qt5_wrap_ui(uis
    #mainwindow.ui
    )

set (RESOURCES
    widgets/icons.qrc
)

qt5_add_resources(RES_SOURCES ${RESOURCES})

set (ESCTEST_SRC
     main.cpp
     mainwindow.cpp
     functiontest.cpp
     wingslot.cpp
     widgets/statusbitwidget.cpp
     widgets/stopwatch.cpp
     widgets/toolframe.cpp
     widgets/qstatebutton.cpp
     widgets/qdynamicbar.cpp
)

include_directories(${egc_SOURCE_DIR}/libs/corbautils
                    ${egc_SOURCE_DIR}/libs/ebird_common
                    ${egc_BINARY_DIR}/corba/idl/ebird
                    ${egc_SOURCE_DIR}/../Birds/common
                    ${egc_SOURCE_DIR}/libs/esclib
                    )

add_executable(esctest ${uis} ${ESCTEST_SRC} ${RES_SOURCES})

target_link_libraries(esctest
                      ebird_common
                      esclib
                      corbautils
                      birds
                      Qt5::Widgets
                      rt)

install(TARGETS esctest
        RUNTIME
        DESTINATION ebird
        )
