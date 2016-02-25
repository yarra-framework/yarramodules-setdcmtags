#ifndef SDT_TWIXREADER_H
#define SDT_TWIXREADER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "sdt_global.h"


class sdtTWIXSearchItem
{
public:
    sdtTWIXSearchItem(std::string newId, std::string newSearchString)
    {
        id=newId;
        searchString=newSearchString;
    }

    std::string id;
    std::string searchString;
};

typedef std::vector<sdtTWIXSearchItem> sdtTwixSearchList;


class sdtTWIXReader
{
public:

    enum fileVersionType
    {
        UNKNOWN=0,
        VAVB,
        VDVE
    };

    sdtTWIXReader();

    bool readFile(std::string filename);
    std::string getErrorReason();
    std::string getValue(std::string id);

    void prepareSearchList();
    void addSearchEntry(std::string id, std::string searchString);

    bool readMRProt(std::ifstream& file);

    fileVersionType   fileVersion;
    sdtTwixSearchList searchList;
    std::map<std::string, std::string> values;

    uint64_t         lastMeasOffset;
    uint32_t         headerLength;
    uint64_t         headerEnd;

    std::string errorReason;
};


inline std::string sdtTWIXReader::getErrorReason()
{
    return errorReason;
}


inline std::string sdtTWIXReader::getValue(std::string id)
{
    // Check if the key exists
    if (values.find(id)==values.end())
    {
        return "";
    }

    // If so, return the value
    return values[id];
}


inline void sdtTWIXReader::addSearchEntry(std::string id, std::string searchString)
{
    searchList.push_back(sdtTWIXSearchItem(id, searchString));
}


#endif // SDT_TWIXREADER_H

