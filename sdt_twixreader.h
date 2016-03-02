#ifndef SDT_TWIXREADER_H
#define SDT_TWIXREADER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "sdt_global.h"


enum twixitemtype
{
    tSTRING=0,
    tBOOL,
    tLONG,
    tDOUBLE
};


class sdtTWIXSearchItem
{
public:

    sdtTWIXSearchItem(std::string newId, std::string newSearchString, twixitemtype newType, bool isMandatory=true)
    {
        id=newId;
        searchString=newSearchString;
        type=newType;
        mandatory=isMandatory;
    }

    std::string  id;
    std::string  searchString;
    twixitemtype type;
    bool         mandatory;
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
    void addSearchEntry(std::string id, std::string searchString, twixitemtype type, bool mandatory=true);

    bool readMRProt(std::ifstream& file);
    void parseXProtLine(std::string& line, std::ifstream& file);
    void parseMRProtLine(std::string line);

    void removeQuotationMarks(std::string& line);
    void removeLeadingWhitespace(std::string& line);
    void removeEnclosingWhitespace(std::string& line);
    void removePrecisionTag(std::string& line);
    void findBraces(std::string& line, std::ifstream& file);

    fileVersionType   fileVersion;
    sdtTwixSearchList searchList;
    stringmap         values;

    uint64_t lastMeasOffset;
    uint32_t headerLength;
    uint64_t headerEnd;

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


inline void sdtTWIXReader::addSearchEntry(std::string id, std::string searchString, twixitemtype type, bool mandatory)
{
    searchList.push_back(sdtTWIXSearchItem(id, searchString, type, mandatory));
}


#endif // SDT_TWIXREADER_H

