#include "sdt_mainclass.h"
#include "sdt_global.h"

#include <iostream>


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

    returnValue=0;
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
            LOG("");
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
            LOG("  Dynamic Settings = " << dynamicSettingsFile);
            LOG("");

            // TODO: Output configuration


        }
    }
    else
    {
        LOG("ERROR: Unable to parse command line.");
    }
}

