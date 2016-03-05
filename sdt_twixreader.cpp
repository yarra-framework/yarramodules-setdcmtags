#include "sdt_twixreader.h"
#include "sdt_twixheader.h"

#include <iostream>
#include <fstream>
#include <algorithm>


sdtTWIXReader::sdtTWIXReader()
{
    errorReason="";
    searchList.clear();
    fileVersion=UNKNOWN;

    lastMeasOffset=0;
    headerLength=0;
    headerEnd=0;

    dbgDumpProtocol=false;

    prepareSearchList();
}


bool sdtTWIXReader::readFile(std::string filename)
{
    lastMeasOffset=0;
    headerLength=0;
    headerEnd=0;

    std::ifstream file;
    file.open(filename.c_str());

    if (!file.is_open())
    {
        errorReason="Unable to open raw-data file";
        return false;
    }

    // Determine TWIX file type: VA/VB or VD/VE?

    uint32_t x[2];
    file.read((char*)x, 2*sizeof(uint32_t));

    if ((x[0]==0) && (x[1]<=64))
    {
        fileVersion=VDVE;
    }
    else
    {
        fileVersion=VAVB;
    }
    file.seekg(0);

    if (fileVersion==VDVE)
    {
        uint32_t id=0, ndset=0;
        std::vector<VD::EntryHeader> veh;

        file.read((char*)&id,   sizeof(uint32_t));  // ID
        file.read((char*)&ndset,sizeof(uint32_t));  // # data sets

        if (ndset>30)
        {
            // If there are more than 30 measurements, it's unlikely that the
            // file is a valid TWIX file
            LOG("WARNING: Number of measurements in file " << ndset);

            errorReason="File is invalid (invalid number of measurements)";
            file.close();
            return false;
        }

        veh.resize(ndset);

        for (size_t i=0; i<ndset; ++i)
        {
            file.read((char*)&veh[i], VD::ENTRY_HEADER_LEN);
        }

        lastMeasOffset=veh.back().MeasOffset;

        // Go to last measurement
        file.seekg(lastMeasOffset);
    }

    // Find header length
    file.read((char*)&headerLength, sizeof(uint32_t));

    if ((headerLength<=0) || (headerLength>5000000))
    {
        // File header is invalid

        std::string fileType="VB";
        if (fileVersion==VDVE)
        {
            fileType="VD/VE";
        }
        LOG("WARNING: Unusual header size " << headerLength << " (file type " << fileType << ")");

        errorReason="File is invalid (unusual header size)";
        file.close();
        return false;
    }

    // Jump back to start of measurement block
    file.seekg(lastMeasOffset);

    headerEnd=lastMeasOffset+headerLength;

    // Parse header
    LOG("Header size is " << headerLength);
    LOG("");

    if (dbgDumpProtocol)
    {
        LOG("### Protocol Dump Begin ###");
    }

    bool terminateParsing=false;

    while ((!file.eof()) && (file.tellg()<headerEnd) && (!terminateParsing))
    {
        std::string line="";
        std::getline(file, line);

        if ((dbgDumpProtocol) && (!line.empty()))
        {
            LOG(line);
        }

        parseXProtLine(line, file);

        // When the MRProt section is reached, parse it and terminate
        if (line.find("### ASCCONV BEGIN ###")!=std::string::npos)
        {
            readMRProt(file);
            terminateParsing=true;
        }
    }

    if (dbgDumpProtocol)
    {
        LOG("### Protocol Dump End ###");
    }

    file.close();

    if (!searchList.empty())
    {
        errorReason="Not all raw-data entries found";
        return false;
    }

    calculateAdditionalValues();

    /*
    for(auto value : values)
    {
       std::cout << value.first << " = " << value.second << std::endl;
    }
    */

    return true;
}


void sdtTWIXReader::calculateAdditionalValues()
{
    if (values.find("DeviceSerialNumber")!=values.end())
    {
        values["StationName"]="MRC"+values["DeviceSerialNumber"];
    }

    // TODO: Calculate frequency

    // TODO: Modify units of MR prot entries
}



bool sdtTWIXReader::readMRProt(std::ifstream& file)
{
    while ((!file.eof()) && (file.tellg()<headerEnd))
    {
        std::string line="";
        std::getline(file, line);

        if ((dbgDumpProtocol) && (!line.empty()))
        {
            LOG(line);
        }

        // Terminate once the end of the mrprot section is reached
        if (line.find("### ASCCONV END ###")!=std::string::npos)
        {
            return true;
        }

        parseMRProtLine(line);
    }

    return false;
}

static std::string sdt_wipKey_VB="sWiPMemBlock";
static std::string sdt_wipKey_VD="sWipMemBlock";


void sdtTWIXReader::parseMRProtLine(std::string line)
{
    size_t equalPos=line.find("=");

    if (equalPos==std::string::npos)
    {
        LOG("WARNING: Invalid MR Prot line found: " << line);
        return;
    }

    // Get everything before the "=" separator
    std::string key=line.substr(0,equalPos-1);

    // Remove all whitespace from key
    key.erase(std::remove(key.begin(), key.end(), ' '), key.end());

    // Replace the pre-VD "sWiPMemBlock" key with the VD name
    if (key.find(sdt_wipKey_VB)!=std::string::npos)
    {
        key.replace(0,sdt_wipKey_VB.length(),sdt_wipKey_VD);
    }

    // Add prefix to key
    key="mrprot."+key;

    // Get everything past the "=" character
    line.erase(0,equalPos+1);
    std::string value=line;

    // Remove leading white space from value
    removeLeadingWhitespace(value);

    // Check if the value string is enclosed by quotation marks
    removeQuotationMarks(value);

    //LOG("<" << key << "> = <" << value << ">");

    // Store in results table
    values[key]=value;
}


void sdtTWIXReader::parseXProtLine(std::string& line, std::ifstream& file)
{
    int indexFound=-1;
    size_t searchPos=std::string::npos;

    for (size_t i=0; i<searchList.size(); i++)
    {
        searchPos=line.find(searchList.at(i).searchString);

        if (searchPos!=std::string::npos)
        {
            // Get value from line and write into result array
            std::string key=searchList.at(i).id;
            std::string value=line;

            value.erase(0,searchPos+searchList.at(i).searchString.length());

            // Search for enclosing {}
            findBraces(value, file);

            switch (searchList.at(i).type)
            {
            case tSTRING:
            default:
                removeEnclosingWhitespace(value);
                removeQuotationMarks(value);
                break;

            case tBOOL:
                if (value.find("true")!=std::string::npos)
                {
                    value="true";
                }
                else
                {
                    value="false";
                }
                break;

            case tLONG:
                removeEnclosingWhitespace(value);
                break;

            case tDOUBLE:
                removePrecisionTag(value);
                break;
            }

            values[key]=value;

            indexFound=i;
            break;
        }
    }

    // Remove entry from search list
    if (indexFound>=0)
    {
        searchList.erase(searchList.begin()+indexFound);
    }
}


void sdtTWIXReader::removeQuotationMarks(std::string& line)
{
    // Check if the string has a quotation mark at the start
    if (line[0]=='"')
    {
        line.erase(0,1);

        // Check if there is also a trailing quotation mark
        if (line[line.length()-1]=='"')
        {
            line.erase(line.length()-1,1);
        }
    }
}


void sdtTWIXReader::removeLeadingWhitespace(std::string& line)
{
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::bind1st(std::not_equal_to<char>(), ' ')));
}


void sdtTWIXReader::removeEnclosingWhitespace(std::string& line)
{
    removeLeadingWhitespace(line);
    line.erase(std::find_if(line.rbegin(), line.rend(), std::bind1st(std::not_equal_to<char>(), ' ')).base(), line.end());
}


void sdtTWIXReader::removePrecisionTag(std::string& line)
{
    std::string tag="<Precision> ";

    // First search and remove the tag
    size_t tagPos=line.find(tag);
    if (line.find(tag)!=std::string::npos)
    {
        line.erase(0,tagPos+tag.length());
    }

    // Now find the space separator and remove the precision number
    size_t sepPos=line.find(" ");
    if (sepPos!=std::string::npos)
    {
        line.erase(0,sepPos);
    }

    // Remove the whitespace around the actual value
    removeEnclosingWhitespace(line);
}


void sdtTWIXReader::findBraces(std::string& line, std::ifstream& file)
{
    // Continue reading lines until the closing brace is found
    while ((!file.eof()) && (file.tellg()<headerEnd) && (line.find("}")==std::string::npos))
    {
        std::string nextLine;
        std::getline(file, nextLine);

        if ((dbgDumpProtocol) && (!nextLine.empty()))
        {
            LOG(nextLine);
        }

        line += nextLine;
    }

    size_t openBracePos=line.find("{");

    if (openBracePos!=std::string::npos)
    {
        // Delete the quotation marks including preceeding white space
        line.erase(0,openBracePos+1);
    } else
    {
        LOG("WARNING: Incorrect format " << line);
    }

    size_t trailingBracePos=line.find("}");
    if (trailingBracePos!=std::string::npos)
    {
        line.erase(trailingBracePos);
    }
}


void sdtTWIXReader::prepareSearchList()
{
    addSearchEntry("PatientName",                "<ParamString.\"tPatientName\">"           , tSTRING);
    addSearchEntry("PatientID",                  "<ParamString.\"PatientID\">"              , tSTRING);
    addSearchEntry("ProtocolName",               "<ParamString.\"tProtocolName\">"          , tSTRING);
    addSearchEntry("SequenceString",             "<ParamString.\"SequenceString\">"         , tSTRING);

    addSearchEntry("SoftwareVersions",           "<ParamString.\"SoftwareVersions\">"       , tSTRING);
    addSearchEntry("Manufacturer",               "<ParamString.\"Manufacturer\">"           , tSTRING);
    addSearchEntry("ManufacturersModelName",     "<ParamString.\"ManufacturersModelName\">" , tSTRING);
    addSearchEntry("LongModelName",              "<ParamString.\"LongModelName\">"          , tSTRING);
    addSearchEntry("MagneticFieldStrength",      "<ParamDouble.\"flMagneticFieldStrength\">", tDOUBLE);

    addSearchEntry("DeviceSerialNumber",         "<ParamString.\"DeviceSerialNumber\">"     , tSTRING);
    addSearchEntry("InstitutionAddress",         "<ParamString.\"InstitutionAddress\">"     , tSTRING);
    addSearchEntry("InstitutionName",            "<ParamString.\"InstitutionName\">"        , tSTRING);
    addSearchEntry("Modality",                   "<ParamString.\"Modality\">"               , tSTRING);

    addSearchEntry("SequenceVariant",            "<ParamString.\"tSequenceVariant\">"       , tSTRING);
    addSearchEntry("ScanningSequence",           "<ParamString.\"tScanningSequence\">"      , tSTRING);
    addSearchEntry("ScanOptions",                "<ParamString.\"tScanOptions\">"           , tSTRING);
    addSearchEntry("MRAcquisitionType",          "<ParamString.\"tMRAcquisitionType\">"     , tSTRING);

    addSearchEntry("BolusAgent",                 "<ParamString.\"BolusAgent\">"             , tSTRING);
    addSearchEntry("ContrastBolusVolume",        "<ParamDouble.\"ContrastBolusVolume\">"    , tDOUBLE);

    addSearchEntry("UsedPatientWeight",          "<ParamDouble.\"flUsedPatientWeight\">"    , tDOUBLE);
    addSearchEntry("Frequency",                   "<ParamLong.\"lFrequency\">"              , tLONG  );

}

