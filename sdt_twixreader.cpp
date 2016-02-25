#include "sdt_twixreader.h"
#include "sdt_twixheader.h"

#include <iostream>
#include <fstream>


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

        // Terminate the parsing once the acquisiton protocol is reached
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

        //TODO
        //evaluateLine(line, file);

        // Terminate once the end of the mrprot section is reached
        if (line.find("### ASCCONV END ###")!=std::string::npos)
        {
            return true;
        }

        if (!line.empty())
        {
            std::cout << line << std::endl;
        }

    }

    return false;
}


void sdtTWIXReader::prepareSearchList()
{
    // TODO
}
