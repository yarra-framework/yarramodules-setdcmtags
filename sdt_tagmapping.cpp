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
