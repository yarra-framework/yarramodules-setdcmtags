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

        if ((ndset>30) || (ndset<1))
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
        //LOG("WARNING: Unusual header size " << headerLength << " (file type " << fileType << ")");

        errorReason="File is invalid (unusual header size)";
        file.close();
        return false;
    }

    // Jump back to start of measurement block
    file.seekg(lastMeasOffset);

    headerEnd=lastMeasOffset+headerLength;

    // Parse header
    //LOG("Header size is " << headerLength << " bytes.");
    //LOG("");

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
        if ((line.find("### ASCCONV BEGIN ###")!=std::string::npos) ||
            (line.find("### ASCCONV BEGIN object=MrProtDataImpl")!=std::string::npos))
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
        bool missingMandatoryEntry=false;

        // Check if any of the remaining entries is a mandatory entry
        for (auto& entry : searchList)
        {
            if (entry.mandatory)
            {
                missingMandatoryEntry=true;
                break;
            }
        }

        if (missingMandatoryEntry)
        {
            LOG("Missing raw-data entries:");
            for (auto& entry : searchList)
            {
                LOG(entry.id);
            }
            LOG("");

            errorReason="Not all raw-data entries found";
            return false;
        }
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
    // Created modified tags as needed by the DICOM format

    if (values.find("DeviceSerialNumber")!=values.end())
    {
        values["StationName"]="MRC"+values["DeviceSerialNumber"];
    }

    if (values.find("PatientAge")!=values.end())
    {
        std::string agestr=values["PatientAge"];

        // Remove the decimals
        size_t dotPos=agestr.find(".");
        if (dotPos!=std::string::npos)
        {
            agestr.erase(dotPos,std::string::npos);
        }

        values["PatientAge_DCM"]=agestr+"Y";
    }

    if (values.find("PatientSex")!=values.end())
    {
        std::string patientSex="O";

        if (values["PatientSex"]=="1")
        {
            patientSex="F";
        }
        if (values["PatientSex"]=="2")
        {
            patientSex="M";
        }

        values["PatientSex_DCM"]=patientSex;
    }

    if (values.find("FrameOfReference")!=values.end())
    {
        // Extract the time stamp from the frame of reference entry. This will be used as approximate
        // acquisition time of no exact time point has been specified via the task file
        std::string dateString="";
        std::string timeString="";
        if (splitFrameOfReferenceTime(values["FrameOfReference"], timeString, dateString))
        {
            values["FrameOfReference_Date"]=dateString;
            values["FrameOfReference_Time"]=timeString;
        }
    }
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

        if (!parseMRProtLine(line))
        {
            return false;
        }
    }

    return false;
}


static std::string sdt_wipKey_VB="sWiPMemBlock";
static std::string sdt_wipKey_VD="sWipMemBlock";


bool sdtTWIXReader::parseMRProtLine(std::string line)
{
    // Remove all tabs from the line (as introduced in VD)
    line.erase(std::remove(line.begin(), line.end(), '\t'), line.end());

    size_t equalPos=line.find("=");

    if (equalPos==std::string::npos)
    {
        LOG("WARNING: Invalid MR Prot line found: " << line);
        return false;
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

    return true;
}


bool sdtTWIXReader::parseXProtLine(std::string& line, std::ifstream& file)
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
            if (!findBraces(value, file))
            {
                return false;
            }

            switch (searchList.at(i).type)
            {
            default:
            case tSTRING:
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

            case tARRAY:
                // TODO
                value="";
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

    return true;
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


bool sdtTWIXReader::findBraces(std::string& line, std::ifstream& file)
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
        return false;
    }

    size_t trailingBracePos=line.find("}");
    if (trailingBracePos!=std::string::npos)
    {
        line.erase(trailingBracePos);
    }

    return true;
}


bool sdtTWIXReader::splitFrameOfReferenceTime(std::string input, std::string& timeString, std::string& dateString)
{
    // Format is 1.3.12.2.1107.5.2.30.25654.1.20130506212248515.0.0.0
    timeString="";
    dateString="";

    // Remove the numbers and dots in front of the date/time section
    int dotPos=0;

    for (int i=0; i<10; i++)
    {
        dotPos=input.find(".",dotPos+1);

        if (dotPos==std::string::npos)
        {
            // String does not match the usual format
            return false;
        }
    }
    input.erase(0,dotPos+1);

    dateString=input.substr(0,4)+"-"+input.substr(4,2)+"-"+input.substr(6,2);
    timeString=input.substr(8,2)+":"+input.substr(10,2)+":"+input.substr(12,2);

    return true;
}


void sdtTWIXReader::prepareSearchList()
{
    addSearchEntry("PatientName",                "<ParamString.\"tPatientName\">"           , tSTRING);
    addSearchEntry("PatientID",                  "<ParamString.\"PatientID\">"              , tSTRING);
    addSearchEntry("PatientBirthDay",            "<ParamString.\"PatientBirthDay\">"        , tSTRING);
    addSearchEntry("PatientSex",                 "<ParamLong.\"PatientSex\">"               , tLONG  );
    addSearchEntry("PatientAge",                 "<ParamDouble.\"flPatientAge\">"           , tDOUBLE);
    addSearchEntry("UsedPatientWeight",          "<ParamDouble.\"flUsedPatientWeight\">"    , tDOUBLE);

    addSearchEntry("ProtocolName",               "<ParamString.\"tProtocolName\">"          , tSTRING);
    addSearchEntry("SequenceString",             "<ParamString.\"SequenceString\">"         , tSTRING);
    addSearchEntry("SequenceVariant",            "<ParamString.\"tSequenceVariant\">"       , tSTRING);
    addSearchEntry("ScanningSequence",           "<ParamString.\"tScanningSequence\">"      , tSTRING);
    addSearchEntry("ScanOptions",                "<ParamString.\"tScanOptions\">"           , tSTRING);
    addSearchEntry("MRAcquisitionType",          "<ParamString.\"tMRAcquisitionType\">"     , tSTRING);

    addSearchEntry("Modality",                   "<ParamString.\"Modality\">"               , tSTRING);
    addSearchEntry("Manufacturer",               "<ParamString.\"Manufacturer\">"           , tSTRING);
    addSearchEntry("ManufacturersModelName",     "<ParamString.\"ManufacturersModelName\">" , tSTRING);
    addSearchEntry("LongModelName",              "<ParamString.\"LongModelName\">"          , tSTRING);
    addSearchEntry("SoftwareVersions",           "<ParamString.\"SoftwareVersions\">"       , tSTRING);
    addSearchEntry("DeviceSerialNumber",         "<ParamString.\"DeviceSerialNumber\">"     , tSTRING);
    addSearchEntry("InstitutionAddress",         "<ParamString.\"InstitutionAddress\">"     , tSTRING);
    addSearchEntry("InstitutionName",            "<ParamString.\"InstitutionName\">"        , tSTRING);
    addSearchEntry("MagneticFieldStrength",      "<ParamDouble.\"flMagneticFieldStrength\">", tDOUBLE);
    addSearchEntry("Frequency",                  "<ParamLong.\"lFrequency\">"               , tLONG  );
    addSearchEntry("ResonantNucleus",            "<ParamString.\"ResonantNucleus\">"        , tSTRING, false);

    addSearchEntry("BolusAgent",                 "<ParamString.\"BolusAgent\">"             , tSTRING);
    addSearchEntry("ContrastBolusVolume",        "<ParamDouble.\"ContrastBolusVolume\">"    , tDOUBLE);
    addSearchEntry("AngioFlag",                  "<ParamString.\"tAngioFlag\">"             , tSTRING);
    addSearchEntry("NumberOfAverages",           "<ParamLong.\"NAveMeas\">"                 , tLONG  );

    addSearchEntry("FrameOfReference",           "<ParamString.\"FrameOfReference\">"       , tSTRING);
    addSearchEntry("PatientPosition",            "<ParamString.\"tPatientPosition\">"       , tSTRING);
    addSearchEntry("BodyPartExamined",           "<ParamString.\"tBodyPartExamined\">"      , tSTRING);
    addSearchEntry("Laterality",                 "<ParamString.\"tLaterality\">"            , tSTRING, false);

    addSearchEntry("GradientCoil",               "<ParamString.\"tGradientCoil\">"          , tSTRING);
    addSearchEntry("TransmittingCoil",           "<ParamString.\"TransmittingCoil\">"       , tSTRING);

    addSearchEntry("ReadoutOSFactor",            "<ParamDouble.\"flReadoutOSFactor\">"      , tDOUBLE);
    addSearchEntry("SpacingBetweenSlices",       "<ParamArray.\"SpacingBetweenSlices\">"    , tARRAY );

    addSearchEntry("ScanTimeSec",                "<ParamLong.\"lScanTimeSec\">"             , tLONG  );
    addSearchEntry("TotalScanTimeSec",           "<ParamLong.\"lTotalScanTimeSec\">"        , tLONG  );

    addSearchEntry("TablePosition",              "<ParamLong.\"SBCSOriginPositionZ\">"      , tLONG  );    
}

