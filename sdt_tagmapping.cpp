#include "sdt_tagmapping.h"
#include "sdt_global.h"

#include <boost/foreach.hpp>


sdtTagMapping::sdtTagMapping()
{
    global.clear();
    current.clear();

    setupDefaultMapping();
}


void sdtTagMapping::setupDefaultMapping()
{
    // TODO
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
        catch(const boost::property_tree::ptree_error &e)
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
        catch(const boost::property_tree::ptree_error &e)
        {
            LOG("ERROR: Unable to read dynamic settings file -- " << e.what());
        }
    }
}


void sdtTagMapping::setupGlobalConfiguration()
{
    // Evaluate global configuration read from mode file

    BOOST_FOREACH(boost::property_tree::ptree::value_type &v, modeFile.get_child("SetDCMTags"))
    {
        std::string key=v.first.data();
        std::string value=v.second.data();

        // Check if the entry if a DICOM mapping. If so, add to mapping table
        if (key[0]=='(')
        {
            global[key]=value;
        }

        std::cout << key << "=" << value << std::endl;  //debug
    }

    // TODO: Read dynamic configuration from dynamic file
}


void sdtTagMapping::setupSeriesConfiguration(int series)
{
    current=global;

    // TODO: Read series configuration from mode file

    // TODO: Read series configuration from dynamic file

}
