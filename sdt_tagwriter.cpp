#include "sdt_tagwriter.h"
#include "sdt_twixreader.h"

#include "dcmtk/dcmdata/dcpath.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmdata/dctk.h"

#include "external/mdfdsman.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/dcmdata/dctk.h"

#include <stdlib.h>
#include <climits>


sdtTagWriter::sdtTagWriter()
{
    slice    =0;
    series   =0;
    seriesUID="";
    studyUID ="";

    approxCreationTime=true;
    raidDateTime="";

    inputFilename ="";
    outputFilename="";
    inputPath     ="";
    outputPath    ="";

    mapping   =nullptr;
    options   =nullptr;
    twixReader=nullptr;

    tags.clear();

    seriesOffset=0;
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

    if (options->find(SDT_OPT_SERIESOFFSET)!=options->end())
    {
        // Read the series offset from the options. Make sure it's set to 0 if the value is invalid
        seriesOffset=strtol((*options)[SDT_OPT_SERIESOFFSET].c_str(),nullptr,10);
        if ((seriesOffset==LONG_MAX) || (seriesOffset==LONG_MIN))
        {
            seriesOffset=0;
        }
    }
}


bool sdtTagWriter::processFile()
{
    calculateVariables();

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
            LOG("ERROR: Unable to set tag " << tag.first << " in " << inputFilename << " (" << result.text() << ")");
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
            value=std::to_string(series+seriesOffset);
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

        if (variable==SDT_VAR_PROC_TIME)
        {
            formatDateTime("%H%M%s", processingTime, value);
        }

        if (variable==SDT_VAR_PROC_DATE)
        {
            formatDateTime("%Y%m%d", processingTime, value);
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

    // If mapped entry is a conversion of a rawfile entry
    if (mapping[0]==SDT_TAG_CNV)
    {
        size_t sepPos=mapping.find(",");

        std::string rawfileKey=mapping.substr(5,sepPos-5);
        std::string arg=mapping.substr(sepPos+1,mapping.length()-2-sepPos);
        std::string func=mapping.substr(1,3);

        value=twixReader->getValue(rawfileKey);

        if (func=="DIV")
        {
            value=eval_DIV(value, arg);
        }

        return true;
    }

    // If mapped entry is static entry
    value=mapping;
    return true;
}


std::string sdtTagWriter::eval_DIV(std::string value, std::string arg)
{
    if (value.empty())
    {
        return "0";
    }

    int decimals=-1;

    size_t sepPos=arg.find(",");
    if (sepPos!=std::string::npos)
    {
        decimals=atoi(arg.substr(sepPos+1,std::string::npos).c_str());
        arg=arg.substr(0,sepPos);
    }

    float val=0;
    float div=0;
    try
    {
        val=stof(value);
        div=stof(arg);
    }
    catch (const std::exception&)
    {
        return "0";
    }

    if (div!=0)
    {
        value=std::to_string(val/div);

        // If maximum number of decimals has been specified
        if (decimals>=0)
        {
            sepPos=value.find(".");
            if (sepPos!=std::string::npos)
            {
                // If no decimals are requested, delete the . as well
                if (decimals==0)
                {
                    decimals=-1;
                }

                size_t eraseStart=sepPos+1+decimals;

                if (eraseStart<value.length()-1)
                {
                    value.erase(eraseStart,std::string::npos);
                }
            }
        }

        return value;
    }
    else
    {
        return "0";
    }
}


void sdtTagWriter::calculateVariables()
{
    // Calculate all internal variables that need to be updated for different slices / series

    // TODO
}


void sdtTagWriter::prepareTime()
{
    if (!raidDateTime.empty())
    {
        // If an exact acquisition time has been provided through the task file

        // TODO

        approxCreationTime=false;
    }
    else
    {
        // If an exact acquisition time has not been provided, use the time obtained
        // from the frame-of-reference entry
        LOG("WARNING: Using approximative acquisition time based on reference scans.");

        // TODO

        approxCreationTime=true;
    }

    processingTime=second_clock::local_time();

    // TODO

}

