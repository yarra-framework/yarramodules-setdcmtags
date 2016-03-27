#include "sdt_tagmapping.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>


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
    addTag("0010", "0010", "@PatientName"               ); // Patient's Name
    addTag("0010", "0020", "@PatientID"                 ); // Patient ID
    addTag("0010", "0030", "@PatientBirthDay"           ); // Patient's Birth Date
    addTag("0010", "1010", "@PatientAge_DCM"            ); // Patient's Age
    addTag("0010", "0040", "@PatientSex_DCM"            ); // Patient's Sex

    addTag("0018", "0010", "@BolusAgent"                ); // Contrast/Bolus Agent
    addTag("0018", "1041", "@ContrastBolusVolume"       ); // Contrast/Bolus Volume

    addTag("0018", "0020", "@ScanningSequence"          ); // Scanning Sequence
    addTag("0018", "0021", "@SequenceVariant"           ); // Sequence Variant
    addTag("0018", "0022", "@ScanOptions"               ); // Scan Options
    addTag("0018", "0023", "@MRAcquisitionType"         ); // MR Acquisition Type
    addTag("0018", "0024", "@SequenceString"            ); // Sequence Name

    addTag("0018", "0087", "@MagneticFieldStrength"     ); // Magnetic Field Strength
    addTag("0018", "0100", "@DeviceSerialNumber"        ); // Device Serial Number
    addTag("0018", "1030", "@ProtocolName"              ); // Protocol Name
    addTag("0008", "103E", "@ProtocolName"              ); // Series Description

    addTag("0018", "1020", "@SoftwareVersions"          ); // Software Versions
    addTag("0018", "1016", "Yarra Framework"            ); // Secondary Capture Device Manufacturer

    std::string setDCMTagInfo=std::string("SetDCMTags Ver ")+SDT_VERSION;
    addTag("0018", "1018", setDCMTagInfo                ); // Secondary Capture Device Manufacturer's Model Name

    addTag("0008", "0060", "@Modality"                  ); // Modality
    addTag("0008", "0070", "@Manufacturer"              ); // Manufacturer
    addTag("0008", "1090", "@ManufacturersModelName"    ); // Manufacturer's Model Name
    addTag("0008", "0080", "@InstitutionName"           ); // Institution Name
    addTag("0008", "0081", "@InstitutionAddress"        ); // Institution Address
    addTag("0008", "1010", "@StationName"               ); // Station Name

    addTag("0008", "0050", "#acc"                       ); // Accession Number
    addTag("0020", "0010", "#uid_study"                 ); // Study ID
    addTag("0020", "0052", "#uid_study"                 ); // Frame of Reference UID
    addTag("0020", "000D", "#uid_study"                 ); // Study Instance UID
    addTag("0020", "000E", "#uid_series"                ); // Series Instance UID
    addTag("0020", "0011", "#series"                    ); // Series Number
    addTag("0020", "0013", "#slice"                     ); // Instance Number
    addTag("0020", "0012", "1"                          ); // Acquisition Number

    addTag("0028", "0004", "MONOCHROME2"                ); // Photometric Interpretation
    addTag("0008", "0005", "ISO_IR 100"                 ); // Specific Character Set

    addTag("0018", "0084", "$DIV(Frequency,1000000)"    ); // Imaging Frequency

    addTag("0018", "0080", "$DIV(mrprot.alTR[0],1000,0)"); // Repetition Time
    addTag("0018", "0081", "$DIV(mrprot.alTE[0],1000,2)"); // Echo Time

    addTag("0028", "0002", "1"                          ); // Samples per Pixel
    addTag("0028", "0100", "16"                         ); // Bits Allocated
    addTag("0028", "0103", "0"                          ); // Pixel Representation

    addTag("0008", "0008", "ORIGINAL\\PRIMARY"          ); // Image Type
    addTag("0019", "0010", "SIEMENS MR HEADER"          ); // Siemens Private Tag
    addTag("0051", "0010", "SIEMENS MR HEADER"          ); // Siemens Private Tag
    addTag("0019", "1008", "IMAGE NUM 4"                ); // Siemens Private Tag
    addTag("0029", "1008", "IMAGE NUM 4"                ); // Siemens Private Tag
    addTag("0051", "1008", "IMAGE NUM 4"                ); // Siemens Private Tag
    addTag("0029", "0010", "SIEMENS CSA HEADER"         ); // Siemens Private Tag
    addTag("0029", "0011", "SIEMENS MEDCOM HEADER2"     ); // Siemens Private Tag
    addTag("0029", "1018", "MR"                         ); // Siemens Private Tag

    addTag("0008", "0020", "#create_date"               ); // Study Date
    addTag("0008", "0030", "#create_time"               ); // Study Time
    addTag("0008", "0021", "#acq_date"                  ); // Series Date
    addTag("0008", "0031", "#acq_time"                  ); // Series Time
    addTag("0008", "0022", "#acq_date"                  ); // Acquisition Date
    addTag("0008", "0032", "#acq_time"                  ); // Acquisition Time
    addTag("0008", "0023", "#proc_date"                 ); // Content/Image Date
    addTag("0008", "0033", "#proc_time"                 ); // Content/Image Time
    addTag("0008", "0012", "#proc_date"                 ); // Instance Creation Date
    addTag("0008", "0013", "#proc_time"                 ); // Instance Creation Time

    addTag("0018", "5100", "@PatientPosition"           ); // Patient Position

    // ImagePositionPatient
    // ImageOrientationPatient
    // SliceLocation

    // TODO: Set slice orientation / location: DCM_ImagePositionPatient, DCM_ImageOrientationPatient, DCM_SliceLocation, DCM_SliceThickness, DCM_PixelSpacing, DCM_PositionReferenceIndicator
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
                // Clear all default mappings if ClearDefaults=true is found
                if ((key==SDT_OPT_CLEARDEFAULTS) && (value==SDT_TRUE))
                {
                    currentTags.clear();
                }
                else
                {
                    currentOptions[key]=value;
                }
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
                // Clear all default mappings if ClearDefaults=true is found
                if ((key==SDT_OPT_CLEARDEFAULTS) && (value==SDT_TRUE))
                {
                    currentTags.clear();
                }
                else
                {
                    currentOptions[key]=value;
                }
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
    // Implements processing options / macros, e.g. Color=true

    if (currentOptions.find(SDT_OPT_COLOR)!=currentOptions.end())
    {
        // If the current series should be in color mode
        if (boost::to_upper_copy(currentOptions[SDT_OPT_COLOR])==SDT_TRUE)
        {
            addTag("0028", "0004", "RGB"                ); // Photometric Interpretation
            addTag("0008", "0064", "WSD"                ); // Conversion Type
            addTag("0028", "1055", "Algo1"              ); // Window Center & Width Explanation
            addTag("0028", "0002", "3"                  ); // Samples per Pixel
            addTag("0028", "0100", "8"                  ); // Bits Allocated
            addTag("0028", "0101", "8"                  ); // Bits Allocated
            addTag("0028", "0102", "7"                  ); // High Bit
            addTag("0028", "0106", "0"                  ); // Smallest Image Pixel Value
            addTag("0028", "0107", "255"                ); // Largest Image Pixel Value
            addTag("0028", "1050", "128"                ); // Window Center
            addTag("0028", "1051", "256"                ); // Window Width
        }
    }

    if (currentOptions.find(SDT_OPT_SERIESMODE)!=currentOptions.end())
    {
        // For the time series mode, append the time point number to the series description
        if (boost::to_upper_copy(currentOptions[SDT_OPT_SERIESMODE])==SDT_OPT_SERIESMODE_TIME)
        {
            addTag("0054", "1000", "DYNAMIC"            ); // Series Type
            addTag("0008", "103E", "#protname_frame"    ); // Series Description
            addTag("0054", "0101", "#series_count"      ); // Number of Time Slices
            addTag("0018", "1242", "#duration_frame"    ); // Actual Frame Duration
        }
    }
}

