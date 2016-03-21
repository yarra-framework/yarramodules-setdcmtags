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

#include "boost/date_time/posix_time/posix_time.hpp"


sdtTagWriter::sdtTagWriter()
{
    slice      =0;
    series     =0;
    sliceCount =0;
    seriesCount=0;

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


void sdtTagWriter::setFile(std::string filename, int currentSlice, int totalSlices, int currentSeries, int totalSeries, std::string currentSeriesUID, std::string currentStudyUID)
{
    inputFilename =inputPath +filename;
    outputFilename=outputPath+filename;

    slice      =currentSlice;
    series     =currentSeries;
    seriesUID  =currentSeriesUID;
    studyUID   =currentStudyUID;
    sliceCount =totalSlices;
    seriesCount=totalSeries;
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

    for (auto& mapEntry : *mapping)
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
    for (auto& tag : tags)
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

        if (variable==SDT_VAR_CREA_TIME)
        {
            formatDateTime("%H%M%s", creationTime, value);
        }

        if (variable==SDT_VAR_CREA_DATE)
        {
            formatDateTime("%Y%m%d", creationTime, value);
        }

        if (variable==SDT_VAR_ACQ_TIME)
        {
            formatDateTime("%H%M%s", acquisitionTime, value);
        }

        if (variable==SDT_VAR_ACQ_DATE)
        {
            formatDateTime("%Y%m%d", acquisitionTime, value);
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
    //LOG("DBG: " << value << " " << arg);

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

    double frameTime=0;
    bool   timeOffsetFound=false;

    double frameDuration=0;
    bool   frameDurationFound=false;

    if (options->find(SDT_OPT_TIMEOFFSET)!=options->end())
    {
        // Read time offset from options and convert into float
        std::string frameTimeStr=(*options)[SDT_OPT_TIMEOFFSET];
        try
        {
            frameTime=stof(frameTimeStr);
        }
        catch (const std::exception&)
        {
            frameTime=0;
        }

        timeOffsetFound=true;
    }

    if (options->find(SDT_OPT_FRAMEDURATION)!=options->end())
    {
        // Read frame duration from options and convert into float
        std::string frameDurationStr=(*options)[SDT_OPT_FRAMEDURATION];
        try
        {
            frameDuration=stof(frameDurationStr);
        }
        catch (const std::exception&)
        {
            frameDuration=0;
        }

        frameDurationFound=true;
    }


    // If dynamic mode has been selected, add the time interval to the base time (creationTime)
    if (options->find(SDT_OPT_SERIESMODE)!=options->end())
    {
        // If the current series should be in color mode
        if (boost::to_upper_copy((*options)[SDT_OPT_SERIESMODE])==SDT_OPT_SERIESMODE_TIME)
        {
            std::string scanTimeStr=twixReader->getValue("TotalScanTimeSec");

            // If not explicit time offset has been given, estimate time point
            // based on total scan duration and number of series
            if (!timeOffsetFound)
            {
                if (!scanTimeStr.empty())
                {
                    frameTime=std::stod(scanTimeStr);
                    frameTime=frameTime/double(seriesCount) * (0.5 + series - 1);
                }

                //LOG("DBG: Total duration " << std::stod(scanTimeStr));
                //LOG("DBG: Time series mode, total duration = " << frameTime);
            }

            if (!frameDurationFound)
            {
                if (!scanTimeStr.empty())
                {
                    frameDuration=std::stod(scanTimeStr)/double(seriesCount);
                }
            }
        }
    }

    if (frameTime!=0)
    {
        // Add frametime to creation time

        long frameSec=long(frameTime);
        long frameMSec=long((frameTime-frameSec)*1000);

        acquisitionTime=creationTime + seconds(frameSec) + milliseconds(frameMSec);
    }
    else
    {
        acquisitionTime=creationTime;
    }

    // TODO: Adapt the duration tag if dynamic mode has been selected (matching the frame time)

    // TODO: Set duration tag

}


void sdtTagWriter::prepareTime()
{
    processingTime=second_clock::local_time();

    if (!raidDateTime.empty())
    {
        // If an exact acquisition time has been provided through the task file
        approxCreationTime=false;

        // TODO: Implement task reader
        creationTime=second_clock::local_time();  // dbg
    }
    else
    {
        // If an exact acquisition time has not been provided, use the time obtained
        // from the frame-of-reference entry
        approxCreationTime=true;
        LOG("WARNING: Using approximative acquisition time based on reference scans.");

        std::string timeString=twixReader->getValue("FrameOfReference_Date")+" "+twixReader->getValue("FrameOfReference_Time");
        try
        {
            creationTime=ptime(time_from_string(timeString));
        }
        catch (const std::exception&)
        {
            creationTime=second_clock::local_time();
        }
    }

    // Unless overwritten via an option, the acquisition time should be identical to the
    acquisitionTime=creationTime;
}


