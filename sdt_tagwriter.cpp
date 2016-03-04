#include "sdt_tagwriter.h"
#include "sdt_twixreader.h"


#include "dcmtk/dcmdata/dcpath.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmdata/dctk.h"

#include "external/mdfdsman.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/dcmdata/dctk.h"


sdtTagWriter::sdtTagWriter()
{
    slice    =0;
    series   =0;
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


void sdtTagWriter::setFile(std::string filename, int currentSlice, int currentSeries, std::string currentSeriesUID, std::string currentStudyUID)
{
    inputFilename =inputPath +filename;
    outputFilename=outputPath+filename;

    slice    =currentSlice;
    series   =currentSeries;
    seriesUID=currentSeriesUID;
    studyUID =currentStudyUID;
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

        // Obtain the value to be written into the DICOM tag. Write it into the results
        // array only if indicated by the return value from getTagValue
        if (getTagValue(mapEntry.second, value))
        {
            tags[dcmPath]=value;
        }
    }

    // debug
    /*
    std::cout << "--- Series " << series << ", Slice " << slice << "---" << std::endl;
    for (auto entry : tags)
    {
        std::cout << entry.first << "=" << entry.second << std::endl;
    }
    std::cout << "-----------------------------" << std::endl << std::endl;
    */

    // Evaluate and write the results
    return writeFile();
}


bool sdtTagWriter::writeFile()
{
    OFCondition result=EC_Normal;

    MdfDatasetManager ds_man;

    // Load file into dataset manager
    result=ds_man.loadFile(inputFilename.c_str());

    if (result.bad())
    {
        LOG("ERROR: Unable to load file " << inputFilename);
        return false;
    }

    // Modify DICOM tags in loaded file
    for (auto tag : tags)
    {
        result=ds_man.modifyOrInsertPath(tag.first.c_str(), tag.second.c_str(), OFFalse);

        if (result.bad())
        {
            LOG("ERROR: Unable to set tag " << tag.first << " in " << inputFilename);
        }
    }

    // Save modified file into output folder
    result=ds_man.saveFile(outputFilename.c_str());

    if (result.bad())
    {
        LOG("ERROR: Unable to write file " << outputFilename);
        return false;
    }

    return true;
}


bool sdtTagWriter::getTagValue(std::string mapping, std::string& value)
{
    // If mapped entry is variable
    if (mapping[0]==SDT_TAG_VAR)
    {
        bool writeTag=true;
        std::string variable=mapping.substr(1,std::string::npos);

        if (variable==SDT_VAR_SLICE)
        {
            value=std::to_string(slice);
        }

        if (variable==SDT_VAR_SERIES)
        {
            value=std::to_string(series);
        }

        if (variable==SDT_VAR_UID_SERIES)
        {
            // TODO: Integrate slice offset

            value=seriesUID;
        }

        if (variable==SDT_VAR_UID_STUDY)
        {
            value=studyUID;
        }

        if (variable==SDT_VAR_ACC)
        {
            value=accessionNumber;

            if (value.empty())
            {
                writeTag=false;
            }
        }

        if (variable==SDT_VAR_KEEP)
        {
            value="";
            writeTag=false;
        }

        return writeTag;
    }

    // If mapped entry is rawfile entry
    if (mapping[0]==SDT_TAG_RAW)
    {
        std::string rawfileKey=mapping.substr(1,std::string::npos);

        value=twixReader->getValue(rawfileKey);

        return true;
    }

    // If mapped entry is static entry
    value=mapping;

    return true;
}

