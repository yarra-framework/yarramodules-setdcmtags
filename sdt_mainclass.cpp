#include "sdt_mainclass.h"
#include "sdt_global.h"

#include <iostream>


sdtMainclass::sdtMainclass()
    : app("SetDCMTags", "Set DICOM tags to values read from Siemens raw-data file")
{
    returnValue=0;
}


void sdtMainclass::perform(int argc, char *argv[])
{
    cmdLine.addParam ("input",     "Folder with DICOM files to process",                OFCmdParam::PM_Mandatory);
    cmdLine.addParam ("output",    "Folder where modified DICOM files will be written", OFCmdParam::PM_Mandatory);
    cmdLine.addParam ("rawfile",   "Path and name of raw-data file",                    OFCmdParam::PM_Mandatory);

    cmdLine.addGroup("parameters:");
    cmdLine.addOption("-a", "", 1, "", "Accession number");
    cmdLine.addOption("-m", "", 1, "", "Path and name of mode file");
    cmdLine.addOption("-d", "", 1, "", "Path and name of dynamic settings");
    cmdLine.addOption("-l", "", 0, "", "Extended log output for debugging");

    cmdLine.addGroup ("other options:");
    cmdLine.addOption("-v", "Show version information and exit", OFCommandLine::AF_Exclusive);

    LOG("");

    prepareCmdLineArgs(argc, argv);

    if (app.parseCommandLine(cmdLine, argc, argv))
    {
        // Output the version number without any other text, so that it can be parsed
        if ((cmdLine.hasExclusiveOption()) && (cmdLine.findOption("-v")))
        {
            LOG("Yarra SetDCMTags");
            LOG("");
            LOG("Version:  "  << SDT_VERSION);
            LOG("Build on: " << __DATE__ << " " << __TIME__);
            LOG("");
            return;
        }

        LOG("Yarra SetDCMTags -- Version " << SDT_VERSION);
        LOG("");
    }
    else
    {
        LOG("ERROR: Unable to parse command line.")
    }
}

