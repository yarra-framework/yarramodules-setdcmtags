#include "gsp_mainclass.h"
#include "../sdt_global.h"

#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace fs = boost::filesystem;

#define GSP_VER "0.1a"


std::string const gspMainclass::summaryItems[] = {"PatientName", "PatientID", "TotalScanTimeSec", "mrprot.sKSpace.lBaseResolution", "mrprot.sKSpace.lRadialViews"};


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
        LOG("Yarra GetSeqParams v" << GSP_VER);
        LOG("------------------------");
        LOG("");        
        LOG("Usage: " << targetname << " [TWIX file] [command] [options]");
        LOG("");
        LOG("Available commands:");
        LOG("");
        LOG("    show              -- Shows relevant parameters from TWIX file");
        LOG("    show  [parameter] -- Shows specific parameter from TWIX file");
        LOG("    show  all         -- Shows all parameters from TWIX file");
        LOG("    write [filename]  -- Writes parameter summary into ini file");
        LOG("");

        returnValue=0;
        return;
    }

    std::string filename(argv[1]);
    fs::path    filepath(argv[1]);

    if (!fs::exists(filepath) || !fs::is_regular_file(filepath))
    {
        LOG("ERROR: Twix file does not exist " << filename);
        returnValue=1;
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

    if (mode==INVALID)
    {
        LOG("ERROR: Invalid parameters. Call without arguments for usage information.");

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





