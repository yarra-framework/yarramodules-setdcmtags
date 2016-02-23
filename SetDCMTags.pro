TARGET = SetDCMTags

CONFIG -= qt

SOURCES += main.cpp \
    sdt_mainclass.cpp \
    external/mdfdsman.cc \
    external/dcdictbi.cc

HEADERS += \
    sdt_mainclass.h \
    sdt_global.h \
    external/mdfdsman.h


DEFINES += HAVE_CONFIG_H
DEFINES += USE_NULL_SAFE_OFSTRING

INCLUDEPATH += /usr/local/include/dcmtk/dcmnet/
INCLUDEPATH += /usr/local/include/dcmtk/config/

LIBS =  -ldcmdata -loflog -lofstd -lz -lpthread -lrt

#LIBS += -loflog -pthread -lofstd -ldcmdata -ldcmnet
#LIBS += -lrt -lnsl -lm -lz
#LIBS += -L/usr/local/lib/
