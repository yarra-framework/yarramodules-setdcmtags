#include "sdt_mainclass.h"
#include "sdt_global.h"

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace fs = boost::filesystem;



sdtMainclass::sdtMainclass()
    : app("SetDCMTags", "Set DICOM tags to values read from Siemens raw-data file")
{
    inputDir ="";
    outputDir="";
    rawFile  ="";

    accessionNumber    ="";
    taskFile           ="";
    modeFile           ="";
    dynamicSettingsFile="";
    extendedLog        =false;

    seriesMap.clear();
    studyUID="";

    returnValue=0;
}


sdtMainclass::~sdtMainclass()
{
    LOG("");
}


#define SDT_PARAM_ACC "-a"
#define SDT_PARAM_MOD "-m"
#define SDT_PARAM_DYN "-d"
#define SDT_PARAM_LOG "-l"
#define SDT_PARAM_VER "-v"
#define SDT_PARAM_TSK "-t"


void sdtMainclass::perform(int argc, char *argv[])
{
    OFLog::configure(OFLogger::ERROR_LOG_LEVEL);

    cmdLine.addParam ("input",     "Folder with DICOM files to process",                OFCmdParam::PM_Mandatory);
    cmdLine.addParam ("output",    "Folder where modified DICOM files will be written", OFCmdParam::PM_Mandatory);
    cmdLine.addParam ("rawfile",   "Path and name of raw-data file",                    OFCmdParam::PM_Mandatory);

    cmdLine.addGroup("parameters:");
    cmdLine.addOption(SDT_PARAM_ACC, "", 1, "", "Accession number");
    cmdLine.addOption(SDT_PARAM_TSK, "", 1, "", "Path and name of task file");
    cmdLine.addOption(SDT_PARAM_MOD, "", 1, "", "Path and name of mode file");
    cmdLine.addOption(SDT_PARAM_DYN, "", 1, "", "Path and name of dynamic settings");
    cmdLine.addOption(SDT_PARAM_LOG, "", 0, "", "Extended log output for debugging");

    cmdLine.addGroup ("other options:");
    cmdLine.addOption(SDT_PARAM_VER, "Show version information and exit", OFCommandLine::AF_Exclusive);

    LOG("");

    prepareCmdLineArgs(argc, argv);

    if (app.parseCommandLine(cmdLine, argc, argv))
    {
        // Output the version number without any other text, so that it can be parsed
        if ((cmdLine.hasExclusiveOption()) && (cmdLine.findOption(SDT_PARAM_VER)))
        {
            LOG("Yarra SetDCMTags");
            LOG("");
            LOG("Version:  "  << SDT_VERSION);
            LOG("Build on: " << __DATE__ << " " << __TIME__);
            return;
        }

        LOG("Yarra SetDCMTags -- Version " << SDT_VERSION);
        LOG("");
        LOG("WARNING: This module is still in development and might not be completely functional yet.");
        LOG("WARNING: Calculation of image orientation and position is not working yet.");
        LOG("WARNING: USE WITH CARE!");
        LOG("");

        // ## First get the mandatory parameters
        cmdLine.getParam(1, inputDir );
        cmdLine.getParam(2, outputDir);
        cmdLine.getParam(3, rawFile  );

        // ## Now read the optional parameters
        if (cmdLine.findOption(SDT_PARAM_ACC))
        {
            if (cmdLine.getValue(accessionNumber) != OFCommandLine::VS_Normal)
            {
                LOG("ERROR: Unable to read ACC number.");
                return;
            }
        }

        if (cmdLine.findOption(SDT_PARAM_TSK))
        {
            if (cmdLine.getValue(taskFile) != OFCommandLine::VS_Normal)
            {
                LOG("ERROR: Unable to read task-file path.");
                return;
            }
        }

        if (cmdLine.findOption(SDT_PARAM_MOD))
        {
            if (cmdLine.getValue(modeFile) != OFCommandLine::VS_Normal)
            {
                LOG("ERROR: Unable to read mode-file path.");
                return;
            }
        }

        if (cmdLine.findOption(SDT_PARAM_DYN))
        {
            if (cmdLine.getValue(dynamicSettingsFile) != OFCommandLine::VS_Normal)
            {
                LOG("ERROR: Unable to read dynamic settings file path.");
                return;
            }
        }

        if (cmdLine.findOption(SDT_PARAM_LOG))
        {
            extendedLog=true;
            LOG("Extended logging is ON.");
            LOG("");
            LOG("Configuration:");
            LOG("  Input folder     = " << inputDir           );
            LOG("  Output folder    = " << outputDir          );
            LOG("  Raw-data file    = " << rawFile            );
            LOG("  Accession number = " << accessionNumber    );
            LOG("  Mode file        = " << modeFile           );
            LOG("  Dynamic settings = " << dynamicSettingsFile);
            LOG("");
        }
    }
    else
    {
        LOG("ERROR: Unable to parse command line.");
        return;
    }

    // Test is given directories and filenames exist
    if (!checkFolderExistence())
    {
        LOG("Check if parameters are correct");

        returnValue=1;
        return;
    }

    twixReader.setDebugOptions(extendedLog);

    // Now parse the raw-data file and extract all needed information
    if (!twixReader.readFile(std::string(rawFile.c_str())))
    {
        LOG("Error parsing raw-data file " << rawFile);
        LOG("Reason: " << twixReader.errorReason);

        returnValue=1;
        return;
    }

    // Read the settings from the mode file and/or dynamic-settings file (if provided)
    tagMapping.readConfiguration(std::string(modeFile.c_str()),std::string(dynamicSettingsFile.c_str()));
    tagMapping.setupGlobalConfiguration();

    if (!generateFileList())
    {
        LOG("Error while parsing input folder");

        returnValue=1;
        return;
    }

    // Generate UIDs for all series
    if (!generateUIDs())
    {
        LOG("Error while generating UIDs");

        returnValue=1;
        return;
    }

    // Loop over all series and process DICOM files
    if (!processSeries())
    {
        LOG("Error while processing series");

        returnValue=1;
        return;
    }

    LOG("Done.");
}


bool sdtMainclass::generateUIDs()
{
    // Loop over all series to generate a different UID for each series
    for (auto& series : seriesMap)
    {
        char uid[100];
        dcmGenerateUniqueIdentifier(uid, SITE_SERIES_UID_ROOT);
        series.second.uid=std::string(uid);
    }

    // Generate a study UID
    char uid[100];
    dcmGenerateUniqueIdentifier(uid, SITE_STUDY_UID_ROOT);
    studyUID=std::string(uid);

    return true;
}


bool sdtMainclass::processSeries()
{
    // Give tagWriter access to the results from TWIX reader
    tagWriter.setTWIXReader(&twixReader);

    // Set folder for reading and writing the DICOMs
    tagWriter.setFolders(std::string(inputDir.c_str()), std::string(outputDir.c_str()));

    // Forward the ACC number
    tagWriter.setAccessionNumber(std::string(accessionNumber.c_str()));

    // Define the creation and processing
    tagWriter.prepareTime();

    // Loop over all series
    for (auto series : seriesMap)
    {
        int seriesID=series.first;
        tagMapping.setupSeriesConfiguration(seriesID);        

        // Loop over all slices of series
        for (auto& slice : series.second.sliceMap)
        {
            // Inform helper class about current file name and slice/series counters
            tagWriter.setFile(slice.second,                                // filename
                              slice.first, series.second.sliceMap.size(),  // current slice, total slices
                              seriesID, seriesMap.size(),                  // current series, total series
                              series.second.uid, studyUID);                // series UID, study UID
            tagWriter.setMapping(&tagMapping.currentTags, &tagMapping.currentOptions);

            if (!tagWriter.processFile())
            {
                LOG("ERROR: Unable to process file " << slice.second);
                return false;
            }
        }
    }

    return true;
}


bool sdtMainclass::checkFolderExistence()
{
    bool foldersExist=true;

    if (!fs::is_directory(std::string(inputDir.c_str())))
    {
        LOG("ERROR: Unable to find input folder " << inputDir);
        foldersExist=false;
    }

    if (!fs::is_directory(std::string(outputDir.c_str())))
    {
        LOG("ERROR: Unable to find output folder " << outputDir);
        foldersExist=false;
    }

    if (!fs::exists(std::string(rawFile.c_str())))
    {
        LOG("ERROR: Unable to find raw file " << rawFile);
        foldersExist=false;
    }

    if ((!modeFile.empty()) && (!fs::exists(std::string(modeFile.c_str()))))
    {
        LOG("ERROR: Unable to find mode file " << modeFile);
        foldersExist=false;
    }

    if ((!dynamicSettingsFile.empty()) && (!fs::exists(std::string(dynamicSettingsFile.c_str()))))
    {
        LOG("ERROR: Unable to find dynamic settings file " << dynamicSettingsFile);
        foldersExist=false;
    }

    return foldersExist;
}


bool sdtMainclass::generateFileList()
{
    bool success         =true;
    int  series          =1;
    int  slice           =1;
    seriesmode mode      =NOT_DEFINED;
    int  fileCount       =0;
    bool interleaveSeries=false;

    // Check if the interleaved series mode has been selected
    if (tagMapping.isGlobalOptionSet(SDT_OPT_INTERLEAVE_SERIES))
    {
        interleaveSeries=true;
        LOG("Interleaving series (series in slices).");
    }

    fs::path inputPath(std::string(inputDir.c_str()));

    for (const auto& dir_entry : boost::make_iterator_range(fs::directory_iterator(inputPath), {}))
    {
        if (dir_entry.path().extension()==".dcm")
        {
            // Extract the slice and series number from the filename
            success=parseFilename(dir_entry.path().stem().string(), mode, series, slice, interleaveSeries);

            //std::cout << "File: " << dir_entry.path().string() << "  Series: " << series << "  Slice: " << slice << std::endl;

            if (!success)
            {
                // TODO: Output error message
                break;
            }
            else
            {
                // Store the filename in the series and slice mapping
                seriesMap[series].sliceMap[slice]=dir_entry.path().filename().string();
                fileCount++;
            }
        }
    }

    // Check if the series should be stacked into a single series
    if (tagMapping.isGlobalOptionSet(SDT_OPT_STACK_SERIES))
    {
        LOG("Stacking series.");

        // Create temporary copy of seriesMap
        std::map<int, sdtSeriesInfo> bufferMap=seriesMap;
        seriesMap.clear();

        // Resort all image into a single series
        int imageCount=0;

        for(auto entry : bufferMap)
        {
            for(auto file : entry.second.sliceMap)
            {
                seriesMap[0].sliceMap[imageCount]=file.second;
                imageCount++;
            }
        }
    }

    LOG("Processing " << fileCount << " files in " << seriesMap.size() << " series.");

    /* // DEBUG: For outputting file list
    for(auto entry : seriesMap)
    {
        std::cout << "Series " << entry.first << " ## " << std::endl;

        for(auto file : entry.second.sliceMap)
        {
            std::cout << "  " << file.first << " = " << file.second << std::endl;
        }
    }
    */

    return success;
}


int sdtMainclass::getAppendedNumber(std::string input)
{
    // TODO
    int pos=0;

    for (int i=input.length()-1; i>=0; i--)
    {
        if (!isdigit(input.at(i)))
        {
            break;
        }
        pos=i;
    }

    input.erase(0,pos);

    return atoi(input.c_str());
}


bool sdtMainclass::parseFilename(std::string filename, seriesmode& mode, int& series, int& slice, bool interleaveSeries)
{
    series=1;
    slice=1;

    // Check if filename starts with "reco.", as created by the GRASP C++ module.
    // If so, remove it.
    if (filename.find("reco.")==0)
    {
        filename.erase(0,5);
    }

    // Detect if the filename is of from slice[no].dcm or series[no].slice[no].dcm
    seriesmode neededMode=SINGLE_SERIES;

    size_t dotPos=filename.find('.');

    if (dotPos!=std::string::npos)
    {
        neededMode=MULTI_SERIES;
    }

    if (mode==NOT_DEFINED)
    {
        mode=neededMode;
    }
    else
    {
        if (mode!=neededMode)
        {
            std::cout << "ERROR: Inconsistent naming of DICOM files detected." << std::endl;
            std::cout << "ERROR: All DICOMs need to be names either slice[#].dcm or series[#].slice[#].dcm." << std::endl;
            return false;
        }
    }

    std::string tmpSlice=filename;

    struct isnotdigit { bool operator()(char c) { return !isdigit(c); } };

    if (mode==MULTI_SERIES)
    {
        std::string tmpSeries=filename;

        // Split into series and slice at the dot
        tmpSeries.erase(dotPos, std::string::npos);
        tmpSlice.erase(0,dotPos+1);

        // Keep only the number at the end of the string and remove any preceeding characters
        series=getAppendedNumber(tmpSeries);
    }

    // Keep only the number at the end of the string and remove any preceeding characters
    slice=getAppendedNumber(tmpSlice);

    // Error checking for reasonable value range of slice and series number
    if ((slice<0) || (series<0))
    {
        LOG("ERROR: Invalid series or slice number (series " << series << ", slice " << slice << ")");
        return false;
    }

    if (mode==MULTI_SERIES)
    {
        if (interleaveSeries)
        {
            // For interleaved series mode, swap the slice and series number
            int tmpSeriesValue=series;
            int tmpSliceValue=slice;
            series=tmpSliceValue;
            slice=tmpSeriesValue;
        }
    }

    return true;
}


