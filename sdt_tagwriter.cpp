#include "sdt_tagwriter.h"


sdtTagWriter::sdtTagWriter()
{
    slice =0;
    series=0;
    seriesUID="";

    inputFilename ="";
    outputFilename="";
    inputPath     ="";
    outputPath    ="";

    mapping   =nullptr;
    options   =nullptr;
    twixReader=nullptr;
}


void sdtTagWriter::setTWIXReader(sdtTWIXReader* instance)
{
    twixReader=instance;
}


void sdtTagWriter::setFolders(std::string inputFolder, std::string outputFolder)
{
    inputPath=inputFolder;
    outputPath=outputFolder;

    if (inputPath[inputPath.length()-1]!='/')
    {
        inputPath.append("/");
    }

    if (outputPath[outputPath.length()-1]!='/')
    {
        outputPath.append("/");
    }
}


void sdtTagWriter::setFile(std::string filename, int currentSlice, int currentSeries, std::string currentSeriesUID)
{
    inputFilename =inputPath +filename;
    outputFilename=outputPath+filename;

    slice=currentSlice;
    series=currentSeries;
    seriesUID=currentSeriesUID;
}


void sdtTagWriter::setMapping(stringmap* currentMapping, stringmap* currentOptions)
{
    mapping=currentMapping;
    options=currentOptions;
}


bool sdtTagWriter::processFile()
{
    // TODO: Evaluate and write the results

    return true;
}

