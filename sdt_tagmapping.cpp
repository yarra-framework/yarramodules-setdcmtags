#include "sdt_tagmapping.h"

#include <boost/foreach.hpp>


sdtTagMapping::sdtTagMapping()
{
    globalTags.clear();
    globalOptions.clear();

    currentTags.clear();
    currentOptions.clear();

    setupDefaultMapping();
}


void sdtTagMapping::setupDefaultMapping()
{
    addTag("0010", "0010", "@PatientName"               );
    addTag("0010", "0020", "@PatientID"                 );

    addTag("0018", "0010", "@BolusAgent"                );
    addTag("0018", "1041", "@ContrastBolusVolume"       );

    addTag("0018", "0020", "@ScanningSequence"          );
    addTag("0018", "0021", "@SequenceVariant"           );
    addTag("0018", "0022", "@ScanOptions"               );
    addTag("0018", "0023", "@MRAcquisitionType"         );
    addTag("0018", "0024", "@SequenceString"            );

    addTag("0018", "0087", "@MagneticFieldStrength"     );
    addTag("0018", "0100", "@DeviceSerialNumber"        );
    addTag("0018", "1030", "@ProtocolName"              );
    addTag("0018", "1020", "@SoftwareVersions"          );

    addTag("0018", "1016", "Yarra Framework"             );

    std::string setDCMTagInfo=std::string("SetDCMTags Ver ")+SDT_VERSION;
    addTag("0018", "1018", setDCMTagInfo                );

    addTag("0008", "0060", "@Modality"                  );
    addTag("0008", "0070", "@Manufacturer"              );
    addTag("0008", "1090", "@ManufacturersModelName"    );
    addTag("0008", "0080", "@InstitutionName"           );
    addTag("0008", "0081", "@InstitutionAddress"        );
    addTag("0008", "1010", "@StationName"               );

    addTag("0008", "0050", "#acc"                       );
    addTag("0020", "000D", "#uid_study"                 );
    addTag("0020", "000E", "#uid_series"                );
    addTag("0020", "0011", "#series"                    );
    addTag("0020", "0013", "#slice"                     );
    addTag("0020", "0012", "1"                          );

    addTag("0028", "0004", "MONOCHROME2"                );
    addTag("0008", "0005", "ISO_IR 100"                 );

    addTag("0018", "0084", "$DIV(Frequency,1000000)"    );

    addTag("0018", "0080", "$DIV(mrprot.alTR[0],1000,0)");
    addTag("0018", "0081", "$DIV(mrprot.alTE[0],1000,2)");


/*
    common.ImageType = 'ORIGINAL\PRIMARY\M\1\NORM';

    common.Private_0019_10xx_Creator = 'SIEMENS MR HEADER';
    common.Private_0019_1008 = 'IMAGE NUM 4';
    common.Private_0051_10xx_Creator = 'SIEMENS MR HEADER';
    common.Private_0051_1008 = 'IMAGE NUM 4';
    common.Private_0029_10xx_Creator = 'SIEMENS CSA HEADER';
    common.Private_0029_11xx_Creator = 'SIEMENS MEDCOM HEADER2';
    common.CSAImageHeaderType = 'IMAGE NUM 4';
    common.CSASeriesHeaderType = 'MR';

*/

}


void sdtTagMapping::readConfiguration(std::string modeFilename, std::string dynamicFilename)
{
    // If a mode file has been provided, read the content into a property tree
    if (!modeFilename.empty())
    {
        try
        {
            pt::read_ini(modeFilename, modeFile);
        }
        catch(const pt::ptree_error &e)
        {
            LOG("ERROR: Unable to read mode file -- " << e.what());
        }
    }

    // If a dynamic-settings file has been provided, read the content into a property tree
    if (!dynamicFilename.empty())
    {
        try
        {
            pt::read_ini(dynamicFilename, dynamicFile);
        }
        catch(const pt::ptree_error &e)
        {
            LOG("ERROR: Unable to read dynamic settings file -- " << e.what());
        }
    }
}


void sdtTagMapping::setupGlobalConfiguration()
{
    // Evaluate global configuration read from mode file. This will add or overwrite the default mapping
    try
    {
        BOOST_FOREACH(pt::ptree::value_type &v, modeFile.get_child("SetDCMTags"))
        {
            std::string key=v.first.data();
            std::string value=v.second.data();

            // Check if the entry is DICOM mapping. If so add to mapping table, otherwise add to option table
            if (key[0]=='(')
            {
                globalTags[key]=value;
            }
            else
            {
                globalOptions[key]=value;
            }
        }
    }
    catch (const std::exception&)
    {
        // Ini section has not been defined / no entries
    }

    // Read dynamic configuration from dynamic file. This will add or overwrite entries from the mode file
    try
    {
        BOOST_FOREACH(pt::ptree::value_type &v, dynamicFile.get_child("SetDCMTags"))
        {
            std::string key=v.first.data();
            std::string value=v.second.data();

            // Check if the entry is DICOM mapping. If so add to mapping table, otherwise add to option table
            if (key[0]=='(')
            {
                globalTags[key]=value;
            }
            else
            {
                globalOptions[key]=value;
            }
        }
    }
    catch (const std::exception&)
    {
    }

    //std::cout << key << "=" << value << std::endl;  //debug
}


void sdtTagMapping::setupSeriesConfiguration(int series)
{
    // First, copy the global configuration
    currentTags   =globalTags;
    currentOptions=globalOptions;

    std::string sectionName="SetDCMTags_Series"+std::to_string(series);

    // Read series configuration from mode file (adding to or overwriting global configuration)
    try
    {
        BOOST_FOREACH(pt::ptree::value_type &v, modeFile.get_child(sectionName))
        {
            std::string key=v.first.data();
            std::string value=v.second.data();

            // Check if the entry is DICOM mapping. If so add to mapping table, otherwise add to option table
            if (key[0]=='(')
            {
                currentTags[key]=value;
            }
            else
            {
                currentOptions[key]=value;
            }
        }
    }
    catch (const std::exception&)
    {
        // Ini section has not been defined / no entries
    }

    // Read series configuration from dynamic file
    try
    {
        BOOST_FOREACH(pt::ptree::value_type &v, dynamicFile.get_child(sectionName))
        {
            std::string key=v.first.data();
            std::string value=v.second.data();

            // Check if the entry is DICOM mapping. If so add to mapping table, otherwise add to option table
            if (key[0]=='(')
            {
                currentTags[key]=value;
            }
            else
            {
                currentOptions[key]=value;
            }
        }
    }
    catch (const std::exception&)
    {
    }

    evaluateSeriesOptions(series);
}


void sdtTagMapping::evaluateSeriesOptions(int series)
{
    // TODO: Implement processing options / macros, e.g. color=true
}
