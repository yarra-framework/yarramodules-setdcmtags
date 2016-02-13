#ifndef SDT_MAINCLASS_H
#define SDT_MAINCLASS_H

#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcpath.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/ofstd/ofstd.h"


class sdtMainclass
{
public:
    sdtMainclass();

    void perform(int argc, char *argv[]);
    int getReturnValue();

    // Helper class for commandline parsing
    OFCommandLine cmdLine;
    OFConsoleApplication app;

    int returnValue;
};


inline int sdtMainclass::getReturnValue()
{
    return returnValue;
}


#endif // SDT_MAINCLASS_H
