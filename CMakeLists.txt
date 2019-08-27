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
     escmonitor.cpp
     palm.cpp
     wingslot.cpp
     escfunctest.cpp
     escrecorder.cpp
     #wingchargeblockmanager.cpp

     widgets/statusbitwidget.cpp
     widgets/stopwatch.cpp
     widgets/toolframe.cpp
     widgets/qstatebutton.cpp
     widgets/qdynamicbar.cpp
     widgets/qcheckview.cpp
     widgets/qcheckdrop.cpp

     GraphWidget/graphaxis.cpp
     GraphWidget/graphdataset.cpp
     GraphWidget/graphstyler.cpp
     GraphWidget/graphwidget.cpp
     GraphWidget/graphwindow.cpp

     GraphWidget/Themes/basictheme.cpp
     GraphWidget/Themes/darktheme.cpp
     GraphWidget/Themes/graphabstracttheme.cpp
)

include_directories(${egc_SOURCE_DIR}/libs/corbautils
                    ${egc_SOURCE_DIR}/libs/ebird_common
                    ${egc_BINARY_DIR}/corba/idl/ebird
                    ${egc_SOURCE_DIR}/../Birds/common
                    ${egc_SOURCE_DIR}/libs/esclib
                    #/home/ebird/cpp/graphwidget/build-GraphWidget-Unnamed-Debug/
                    )

add_executable(esctest ${uis} ${ESCTEST_SRC} ${RES_SOURCES})

target_link_libraries(esctest
                      ebird_common
                      esclib
                      corbautils
                      birds
                      Qt5::Widgets
                      #GraphWidget
                      rt)

install(TARGETS esctest
        RUNTIME
        DESTINATION ebird
        )
