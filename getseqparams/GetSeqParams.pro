TARGET = GetSeqParams
CONFIG -= qt

# Define identifier for Ubuntu Linux version (UBUNTU_1204 / UBUNTU_1604 / WINDOWS)
BUILD_OS=WINDOWS

equals( BUILD_OS, "UBUNTU_1604" ) {
    message( "Configuring for Ubuntu 16.04" )
    QMAKE_CXXFLAGS += -DUBUNTU_1604
    ICU_PATH=/usr/lib/x86_64-linux-gnu
    BOOST_PATH=/usr/lib/x86_64-linux-gnu
}

equals( BUILD_OS, "UBUNTU_1204" ) {
    message( "Configuring for Ubuntu 12.04" )
    QMAKE_CXXFLAGS += -DUBUNTU_1204
    ICU_PATH=/usr/lib
    BOOST_PATH=/usr/local/lib
}

equals( BUILD_OS, "WINDOWS" ) {
    message( "Configuring for Windows" )
    INCLUDEPATH += C:\NMRProjects\boost\include\boost-1_70
    BOOST_PATH=C:\NMRProjects\boost\lib
    CONFIG += console
    TARGET = yct_getseqparams
    QMAKE_CXXFLAGS += -DTARGET="yct_getseqparams"
}

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp \
    gsp_mainclass.cpp \
    ../sdt_twixreader.cpp 

HEADERS += \
    gsp_mainclass.h \
    ../sdt_twixreader.h

LIBS =  -lpthread

!equals( BUILD_OS, "WINDOWS" ) {
    LIBS += -lrt
}

LIBS += -lz

equals( BUILD_OS, "WINDOWS" ) {
    LIBS += $$BOOST_PATH/libboost_system-mgw49-mt-x32-1_70.a
    LIBS += $$BOOST_PATH/libboost_filesystem-mgw49-mt-x32-1_70.a
    LIBS += $$BOOST_PATH/libboost_date_time-mgw49-mt-x32-1_70.a
} else {
    LIBS += $$BOOST_PATH/libboost_filesystem.a
    LIBS += $$BOOST_PATH/libboost_system.a
    LIBS += $$BOOST_PATH/libboost_date_time.a
}

!equals( BUILD_OS, "WINDOWS" ) {
    LIBS += $$ICU_PATH/libicui18n.a
    LIBS += $$ICU_PATH/libicuuc.a
    LIBS += $$ICU_PATH/libicudata.a
    LIBS += -ldl
}
