#ifndef SDT_TAGWRITER_H
#define SDT_TAGWRITER_H


#include <iostream>
#include <map>
#include "boost/date_time/posix_time/posix_time.hpp"

#include "sdt_global.h"


using namespace boost::posix_time;


class sdtTWIXReader;

class sdtTagWriter
{
public:
    sdtTagWriter();

    void setTWIXReader(sdtTWIXReader* instance);
    void setFolders(std::string inputFolder, std::string outputFolder);
    void setAccessionNumber(std::string acc);

    void setFile(std::string filename, int currentSlice, int currentSeries, std::string currentSeriesUID, std::string currentStudyUID); 
    void setMapping(stringmap* currentMapping, stringmap* currentOptions);

    void setRAIDCreationTime(std::string datetimeString);
    void prepareTime();

    bool processFile();

protected:
    int         slice;
    int         series;
    std::string seriesUID;
    std::string studyUID;
    std::string accessionNumber;

    bool        approxCreationTime;
    std::string raidDateTime;

    ptime       creationTime;
    ptime       processingTime;
    ptime       acquisitionTime;

    std::string inputFilename;
    std::string outputFilename;

    std::string inputPath;
    std::string outputPath;

    stringmap*  mapping;
    stringmap*  options;

    stringmap   tags;

    sdtTWIXReader* twixReader;

    bool getTagValue(std::string mapping, std::string& value);
    bool writeFile();
    void calculateVariables();

    int seriesOffset;

    std::string eval_DIV(std::string value, std::string arg);
};


inline void sdtTagWriter::setAccessionNumber(std::string acc)
{
    accessionNumber=acc;
}


inline void sdtTagWriter::setRAIDCreationTime(std::string datetimeString)
{
    raidDateTime=datetimeString;
}



#endif // SDT_TAGWRITER_H
