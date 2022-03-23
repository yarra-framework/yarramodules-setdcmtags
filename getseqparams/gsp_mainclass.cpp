#include "gsp_mainclass.h"
#include "../sdt_global.h"

#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace fs = boost::filesystem;

#define GSP_VER "0.1b4"


std::string const gspMainclass::summaryItems[] = {"PatientName", "PatientID", "ProtocolName", "HasAdjustments",
                                                  "ContainedMeasurements", "BodyPartExamined", "TotalScanTimeSec",
                                                  "mrprot.sKSpace.lBaseResolution", "mrprot.sKSpace.lRadialViews",
                                                  "mrprot.sKSpace.lImagesPerSlab", "mrprot.sKSpace.lPartitions",
                                                  "mrprot.tSequenceFileName",
                                                  "SoftwareVersions", "mrprot.sProtConsistencyInfo.tBaselineString",
                                                  "ManufacturersModelName", "MagneticFieldStrength", "DeviceSerialNumber"};


gspMainclass::gspMainclass()
{
    mode=INVALID;
    returnValue=0;
}


gspMainclass::~gspMainclass()
{
}


void gspMainclass::perform(int argc, char *argv[])
{
    if (argc<3)
    {
        #if (BUILD_OS==WINDOWS)
            std::string targetname="yct_getseqparams";
        #else
            std::string targetname="GetSeqParams";
        #endif

        LOG("");
        LOG("Yarra Client Tools - GetSeqParams " << GSP_VER);
        LOG("---------------------------------------");
        LOG("");        
        LOG("Usage: " << targetname << " [filename/path] [command] [options]");
        LOG("");
        LOG("Available commands:");
        LOG("");
        LOG("    show                               --  Shows relevant parameters from Twix file");
        LOG("    show  [parameter]                  --  Shows specific parameter from Twix file");
        LOG("    show  all                          --  Shows all parameters from Twix file");
        LOG("    write [filename]                   --  Writes parameter summary into ini file");
        LOG("    index [csv filename] [parameters]  --  Reads parameters from all Twix files in the path (and subfolders) and creates CSV file");
        LOG("                                           CSV columns specified with param_1#param_2#param_3 (see available parameters with \"show all\")");
        LOG("");

        returnValue=0;
        return;
    }

    std::string cmd(argv[2]);

    if (cmd=="show")
    {
        mode=SHOW;
    }

    if (cmd=="write")
    {
        mode=WRITE;

        if (argc!=4)
        {
            mode=INVALID;
        }
    }

    if (cmd=="index")
    {
        mode=INDEX;

        if (argc!=5)
        {
            mode=INVALID;
        }
    }

    if (mode==INVALID)
    {
        LOG("ERROR: Invalid parameters. Call without arguments for usage information");

        returnValue=1;
        return;
    }

    std::string filename(argv[1]);
    fs::path    filepath(argv[1]);

    // Separate handling for the INDEX mode
    if (mode==INDEX)
    {
        std::string csvFilename(argv[3]);
        std::string csvCols(argv[4]);
        if (generateCSV(filename, csvFilename, csvCols))
        {
            returnValue=0;
        }
        else
        {
            returnValue=1;
        }
        return;
    }

    // Handling of modes SHOW and WRITE

    if (!fs::exists(filepath) || !fs::is_regular_file(filepath))
    {
        LOG("ERROR: Twix file does not exist " << filename);
        returnValue=1;
        return;
    }

    twixReader.setDebugOptions(false);

    // Now parse the raw-data file and extract all needed information
    if (!twixReader.readFile(filename))
    {
        LOG("ERROR: Unable to parse raw-data file " << filename);
        LOG("CAUSE: " << twixReader.errorReason);

        returnValue=1;
        return;
    }

    if (mode==SHOW)
    {
        std::string param="summary";

        if (argc==4)
        {
            param=std::string(argv[3]);
        }

        if (param=="summary")
        {
            for (auto const& x : summaryItems)
            {
                LOG(x << "=" << twixReader.values[x]);
            }
        }
        else if (param=="all")
        {
            for (auto const& x : twixReader.values)
            {
                LOG(x.first << "=" << x.second);
            }
        }
        else
        {
            // Don't add endl here in case the results is piped into a shell variable
            std::cout << twixReader.values[param];
        }
    }

    if (mode==WRITE)
    {
        std::string outname(argv[3]);
        std::ofstream out(outname, std::ios_base::trunc);

        out << "[ScanInformation]" << std::endl;
        out << "Filename=" << filepath.filename() << std::endl;

        for (auto const& x : summaryItems)
        {
            out << x << "=" << twixReader.values[x] << std::endl;
        }

        out.close();
    }

    returnValue=0;
    return;
}


bool gspMainclass::generateCSV(std::string searchPath, std::string csvFilename, std::string csvCols)
{
    fs::path dirPath(searchPath);
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
    {
        LOG("ERROR: Search path does not exist " << searchPath);
        return false;
    }

    std::vector<std::string> listOfFiles;
    try
    {
        // Create a Recursive Directory Iterator object and points to the starting of directory
        fs::recursive_directory_iterator iter(dirPath);
        fs::recursive_directory_iterator end;

        // Iterate until end is reached
        while (iter != end)
        {
            if ((fs::is_regular_file(iter->path())) && (iter->path().extension() == ".dat"))
            {
                // Add the name in vector
                listOfFiles.push_back(iter->path().string());
            }

            // Increment the iterator to point to next entry in recursive iteration
            boost::system::error_code ec;
            iter.increment(ec);
            if (ec)
            {
                LOG("Error accessing " << iter->path().string() << " : " << ec.message());
            }
        }
    }
    catch (std::system_error & e)
    {
        LOG("Exception: " << e.what());
        return false;
    }

    fs::path csvPath(csvFilename);
    if (fs::exists(csvPath))
    {
        LOG("ERROR: CSV file already exists " << csvFilename);
        return false;
    }

    // Create CSV file
    std::ofstream csvFile;
    csvFile.open(csvPath.string());
    csvFile << "sep=,\n";

    std::string headerLine = "\"File\"";

    // Parse parameter list
    std::vector<std::string> columns;
    boost::split(columns, csvCols, boost::is_any_of(GSP_COLS_SEPARATOR), boost::token_compress_on);

    for (size_t i=0; i<columns.size(); i++)
    {
        headerLine += ",\""+columns.at(i)+"\"";
    }
    headerLine += "\n";
    csvFile << headerLine;

    // Now loop over the files
    LOG("Indexing files...");

    for (size_t i=0; i<listOfFiles.size(); i++)
    {
        LOG("  " << listOfFiles.at(i));

        twixReader = sdtTWIXReader();
        twixReader.setDebugOptions(false);

        if (!twixReader.readFile(listOfFiles.at(i)))
        {
            LOG("ERROR: Unable to parse raw-data file " << listOfFiles.at(i));
            LOG("CAUSE: " << twixReader.errorReason);
            continue;
        }

        // Compose CSV line
        std::string entryLine = "\""+listOfFiles.at(i)+"\"";

        for (size_t i=0; i<columns.size(); i++)
        {
            std::string value="";
            if (twixReader.values.count(columns[i]))
            {
                value=twixReader.values[columns[i]];
            }
            entryLine += ",\""+value+"\"";
        }

        // Write into CSV file
        entryLine += "\n";
        csvFile << entryLine;
    }

    csvFile.close();
    LOG("Done");

    return true;
}
