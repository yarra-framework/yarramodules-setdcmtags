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

    bool terminateParsing=false;

    while ((!file.eof()) && (file.tellg()<headerEnd) && (!terminateParsing))
    {
        std::string line="";
        std::getline(file, line);

        if (!line.empty())
        {
            //std::cout << line << std::endl;
        }

        //evaluateLine(line, file);

        //if (searchList.empty())
        //{
        //    terminateParsing=true;
        //}

        // When the MRProt section is reached, parse it and terminate
        if (line.find("### ASCCONV BEGIN ###")!=std::string::npos)
        {
            readMRProt(file);
            terminateParsing=true;
        }
    }

    file.close();

    if (!searchList.empty())
    {
        errorReason="Not all raw-data entries found";
        return false;
    }


    return true;
}


bool sdtTWIXReader::readMRProt(std::ifstream& file)
{
    while ((!file.eof()) && (file.tellg()<headerEnd))
    {
        std::string line="";
        std::getline(file, line);

        // Terminate once the end of the mrprot section is reached
        if (line.find("### ASCCONV END ###")!=std::string::npos)
        {
            return true;
        }

        parseMRProtLine(line);

        //std::cout << line << std::endl;
    }

    return false;
}


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

    line.erase(0,equalPos+1);
    std::string value=line;

    // Remove leading white space from value
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), std::bind1st(std::not_equal_to<char>(), ' ')));

    // Check if the value string is enclosed by quotation marks
    if (value[0]=='"')
    {
        value.erase(0,1);

        // Check for trailing quotation mark
        if (value[value.length()-1]=='"')
        {
            value.erase(value.length()-1,1);
        }
    }

    // Add prefix to key
    key="mrprot."+key;

    LOG("<" << key << "> = <" << value << ">");

    // Store in results table
    values[key]=value;
}


void sdtTWIXReader::prepareSearchList()
{
    // TODO
}
