#ifndef SDT_MAINCLASS_H
#define SDT_MAINCLASS_H

#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcpath.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/ofstd/ofstd.h"

#include "sdt_twixreader.h"
#include "sdt_tagmapping.h"


class sdtSeriesInfo
{
public:
    std::string                uid;
    std::map<int, std::string> sliceMap;
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
    ~sdtMainclass();

    void perform(int argc, char *argv[]);
    int getReturnValue();

    bool checkFolderExistence();
    bool generateFileList();
    bool parseFilename(std::string filename, seriesmode& mode, int& series, int& slice);

    bool generateUIDs();


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

    // Helper class for assinging the DICOM tags
    sdtTagMapping        tagMapping;

    // Map to store all series information
    std::map<int, sdtSeriesInfo> seriesMap;

    int returnValue;
};


inline int sdtMainclass::getReturnValue()
{
    return returnValue;
}


#endif // SDT_MAINCLASS_H
