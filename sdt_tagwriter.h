#ifndef SDT_TAGWRITER_H
#define SDT_TAGWRITER_H


#include <iostream>
#include <map>

#include "sdt_global.h"


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

    bool processFile();

protected:
    int         slice;
    int         series;
    std::string seriesUID;
    std::string studyUID;
    std::string accessionNumber;

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

    int seriesOffset;

};


inline void sdtTagWriter::setAccessionNumber(std::string acc)
{
    accessionNumber=acc;
}



#endif // SDT_TAGWRITER_H
