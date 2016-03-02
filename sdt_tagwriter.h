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

    void setFile(std::string filename, int currentSlice, int currentSeries, std::string currentSeriesUID);
    void setMapping(stringmap* currentMapping, stringmap* currentOptions);

    bool processFile();

protected:
    int         slice;
    int         series;
    std::string seriesUID;

    std::string inputFilename;
    std::string outputFilename;

    std::string inputPath;
    std::string outputPath;

    stringmap* mapping;
    stringmap* options;

    sdtTWIXReader* twixReader;
};


#endif // SDT_TAGWRITER_H
