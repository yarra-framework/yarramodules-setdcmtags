TARGET = SetDCMTags
CONFIG -= qt

# Define identifier for Ubuntu Linux version (UBUNTU_1204 / UBUNTU_1404)
BUILD_OS=UBUNTU_1204

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
    sdt_mainclass.cpp \
    external/mdfdsman.cc \
    external/dcdictbi.cc \
    sdt_twixreader.cpp \
    sdt_tagmapping.cpp \
    sdt_tagwriter.cpp

HEADERS += \
    sdt_mainclass.h \
    sdt_global.h \
    external/mdfdsman.h \
    sdt_twixreader.h \
    sdt_twixheader.h \
    sdt_tagmapping.h \
    sdt_tagwriter.h


DEFINES += HAVE_CONFIG_H
DEFINES += USE_NULL_SAFE_OFSTRING

INCLUDEPATH += /usr/local/include/dcmtk/dcmnet/
INCLUDEPATH += /usr/local/include/dcmtk/config/

LIBS =  -ldcmdata -loflog -lofstd -lz -lpthread -lrt

LIBS += $$BOOST_PATH/libboost_filesystem.a
LIBS += $$BOOST_PATH/libboost_system.a


#LIBS += -loflog -pthread -lofstd -ldcmdata -ldcmnet
#LIBS += -lrt -lnsl -lm -lz
#LIBS += -L/usr/local/lib/
