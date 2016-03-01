#ifndef SDT_MAINCLASS_H
#define SDT_MAINCLASS_H

#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcpath.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/ofstd/ofstd.h"

#include "sdt_twixreader.h"


class sdtSeriesInfo
{
    int                        number;
    std::string                uid;
    std::map<int, std::string> files;
};


class sdtMainclass
{
public:

    enum seriesmode {
        NOT_DEFINED  =-1,
        SINGLE_SERIES= 0,
        MULTI_SERIES = 1
    };

    sdtMainclass();

    void perform(int argc, char *argv[]);
    int getReturnValue();

    bool checkFolderExistence();
    bool generateFileList();
    bool parseFilename(std::string filename, seriesmode& mode, int& series, int& slice);

    // Helper class for commandline parsing
    OFCommandLine        cmdLine;
    OFConsoleApplication app;

    // Settings from commandline parameters
    OFCmdString          inputDir;
    OFCmdString          outputDir;
    OFCmdString          rawFile;

    OFCmdString          accessionNumber;
    OFCmdString          modeFile;
    OFCmdString          dynamicSettingsFile;
    bool                 extendedLog;

    // Helper class to parse TWIX files
    sdtTWIXReader        twixReader;

    std::vector<sdtSeriesInfo> series;

    int returnValue;
};


inline int sdtMainclass::getReturnValue()
{
    return returnValue;
}


#endif // SDT_MAINCLASS_H
