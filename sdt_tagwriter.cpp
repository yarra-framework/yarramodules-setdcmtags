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

    tags.clear();
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
    tags.clear();

    for (auto mapEntry : *mapping)
    {
        std::string dcmPath=mapEntry.first;
        std::string value="";

        tags[dcmPath]=value;
    }

    // TODO: Evaluate and write the results

    // debug
    std::cout << "--- Series " << series << ", Slice " << slice << "---" << std::endl;
    for (auto entry : tags)
    {
        std::cout << entry.first << "=" << entry.second << std::endl;
    }
    std::cout << "-----------------------------" << std::endl << std::endl;

    return true;
}

