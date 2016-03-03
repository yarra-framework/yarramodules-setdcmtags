#include "sdt_mainclass.h"
#include "sdt_global.h"

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace fs = boost::filesystem;



sdtMainclass::sdtMainclass()
    : app("SetDCMTags", "Set DICOM tags to values read from Siemens raw-data file")
{
    inputDir="";
    outputDir="";
    rawFile="";

    accessionNumber="";
    modeFile="";
    dynamicSettingsFile="";
    extendedLog=false;

    seriesMap.clear();

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


void sdtMainclass::perform(int argc, char *argv[])
{
    cmdLine.addParam ("input",     "Folder with DICOM files to process",                OFCmdParam::PM_Mandatory);
    cmdLine.addParam ("output",    "Folder where modified DICOM files will be written", OFCmdParam::PM_Mandatory);
    cmdLine.addParam ("rawfile",   "Path and name of raw-data file",                    OFCmdParam::PM_Mandatory);

    cmdLine.addGroup("parameters:");
    cmdLine.addOption(SDT_PARAM_ACC, "", 1, "", "Accession number");
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

    // Now parse the raw-data file and extract all needed information
    if (!twixReader.readFile(std::string(rawFile.c_str())))
    {
        LOG("Error parsing raw-data file " << rawFile);
        LOG("Reason: " << twixReader.errorReason);

        returnValue=1;
        return;
    }

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

    // Read the settings from the mode file and/or dynamic-settings file (if provided)
    tagMapping.readConfiguration(std::string(modeFile.c_str()),std::string(dynamicSettingsFile.c_str()));
    tagMapping.setupGlobalConfiguration();

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
    // TODO

    return true;
}


bool sdtMainclass::processSeries()
{
    // Give tagWriter access to the results from TWIX reader
    tagWriter.setTWIXReader(&twixReader);

    // Set folder for reading and writing the DICOMs
    tagWriter.setFolders(std::string(inputDir.c_str()), std::string(outputDir.c_str()));

    // Loop over all series
    for (auto series : seriesMap)
    {
        int seriesID=series.first;
        tagMapping.setupSeriesConfiguration(seriesID);

        // Loop over all slices of series
        for (auto slice : series.second.sliceMap)
        {
            // Inform helper class about current file name and slice/series counters
            tagWriter.setFile(slice.second, slice.first, seriesID, series.second.uid);
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
    bool success   =true;
    int series     =0;
    int slice      =0;
    seriesmode mode=NOT_DEFINED;

    fs::path inputPath(std::string(inputDir.c_str()));

    for(const auto &dir_entry : boost::make_iterator_range(fs::directory_iterator(inputPath), {}))
    {
        if (dir_entry.path().extension() == ".dcm")
        {
            // Extract the slice and series number from the filename
            success=parseFilename(dir_entry.path().stem().string(), mode, series, slice);

            //std::cout << "File: " << dir_entry.path().string() << "  Series: " << series << "  Slice: " << slice << std::endl;

            if (!success)
            {
                break;
            }
            else
            {
                // Store the filename in the series and slice mapping
                seriesMap[series].sliceMap[slice]=dir_entry.path().filename().string();
            }
        }
    }

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


bool sdtMainclass::parseFilename(std::string filename, seriesmode& mode, int& series, int& slice)
{
    series=0;
    slice=0;

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

    if (mode==MULTI_SERIES)
    {
        std::string tmpSeries=filename;

        // Split into series and slice at the dot
        tmpSeries.erase(dotPos, std::string::npos);
        tmpSlice.erase(0,dotPos+1);

        // Remove all characters and keep only numbers
        tmpSeries.erase(std::remove_if(tmpSeries.begin(),tmpSeries.end(),isalpha),tmpSeries.end());
        series=atoi(tmpSeries.c_str());
    }

    // Remove all characters and keep only numbers
    tmpSlice.erase(std::remove_if(tmpSlice.begin(),tmpSlice.end(),isalpha),tmpSlice.end());
    slice=atoi(tmpSlice.c_str());

    return true;
}

