#ifndef SDT_TAGMAPPING_H
#define SDT_TAGMAPPING_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <iostream>
#include <map>

namespace pt = boost::property_tree;


// Assignment format: (group,element) = fixed value
//                                    = @entry from rawfile
//                                    = #variable


class sdtTagMapping
{
public:
    sdtTagMapping();

    void readConfiguration(std::string modeFilename, std::string dynamicFilename);

    void setupDefaultMapping();
    void setupGlobalConfiguration();
    void setupSeriesConfiguration(int series);

    std::map<std::string,std::string> global;
    std::map<std::string,std::string> current;

    pt::ptree modeFile;
    pt::ptree dynamicFile;

    std::string makeTag(std::string group, std::string element);

};


inline std::string sdtTagMapping::makeTag(std::string group, std::string element)
{
    return "("+group+","+element+")";
}


#endif // SDT_TAGMAPPING_H

