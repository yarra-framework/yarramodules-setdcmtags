TARGET = GetSeqParams
CONFIG -= qt

# Define identifier for Ubuntu Linux version (UBUNTU_1204 / UBUNTU_1404)
BUILD_OS=UBUNTU_1404

equals( BUILD_OS, "UBUNTU_1404" ) {
    message( "Configuring for Ubuntu 14.04" )
    QMAKE_CXXFLAGS += -DUBUNTU_1404
    ICU_PATH=/usr/lib/x86_64-linux-gnu
    BOOST_PATH=/usr/lib/x86_64-linux-gnu
}

equals( BUILD_OS, "UBUNTU_1204" ) {
    message( "Configuring for Ubuntu 12.04" )
    QMAKE_CXXFLAGS += -DUBUNTU_1204
    ICU_PATH=/usr/lib
    BOOST_PATH=/usr/local/lib
}

QMAKE_CXXFLAGS += -std=c++11 

SOURCES += main.cpp \
    gsp_mainclass.cpp \
    ../sdt_twixreader.cpp 

HEADERS += \
    gsp_mainclass.h \
    ../sdt_twixreader.h

LIBS =  -lpthread -lrt

LIBS += -lz
LIBS += $$BOOST_PATH/libboost_filesystem.a
LIBS += $$BOOST_PATH/libboost_system.a
LIBS += $$BOOST_PATH/libboost_date_time.a

LIBS += $$ICU_PATH/libicui18n.a
LIBS += $$ICU_PATH/libicuuc.a
LIBS += $$ICU_PATH/libicudata.a

LIBS += -ldl
