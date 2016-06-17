#ifndef SDT_TAGWRITER_H
#define SDT_TAGWRITER_H


#include <iostream>
#include <map>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <armadillo>

#include "sdt_global.h"

using namespace boost::posix_time;


class sdtTWIXReader;

class sdtTagWriter
{
public:
    sdtTagWriter();

    void setTWIXReader(sdtTWIXReader* instance);
    void setFolders(std::string inputFolder, std::string outputFolder);
    void setAccessionNumber(std::string acc);

    void setFile(std::string filename, int currentSlice, int totalSlices, int currentSeries, int totalSeries, std::string currentSeriesUID, std::string currentStudyUID);
    void setMapping(stringmap* currentMapping, stringmap* currentOptions);

    void setRAIDCreationTime(std::string datetimeString);
    void prepareTime();

    bool processFile();

protected:
    int         slice;
    int         series;
    std::string seriesUID;
    std::string studyUID;
    std::string accessionNumber;
    int         sliceCount;
    int         seriesCount;    

    bool        is3DScan;
    int         sliceArraySize;

    bool        approxCreationTime;
    std::string raidDateTime;

    ptime       creationTime;
    ptime       processingTime;
    ptime       acquisitionTime;

    double      frameDuration;

    std::string imagePositionPatient;
    std::string imageOrientationPatient;
    std::string sliceLocation;
    std::string sliceThickness;
    std::string pixelSpacing;
    std::string slicesSpacing;

    std::string inputFilename;
    std::string outputFilename;

    std::string inputPath;
    std::string outputPath;

    stringmap*  mapping;
    stringmap*  options;

    stringmap   tags;

    sdtTWIXReader* twixReader;

    bool getTagValue(std::string mapping, std::string& value, int recurCount=0);
    bool writeFile();

    void calculateVariables();
    void calculateOrientation();

    int seriesOffset;

    std::string eval_DIV(std::string value, std::string arg);

    void formatDateTime(std::string const& format, ptime const& date_time, std::string& result);

    arma::mat rotation_matrix(double theta, const arma::mat & rotationAxis);

};


inline void sdtTagWriter::setAccessionNumber(std::string acc)
{
    accessionNumber=acc;
}


inline void sdtTagWriter::setRAIDCreationTime(std::string datetimeString)
{
    raidDateTime=datetimeString;
}


inline void sdtTagWriter::formatDateTime(std::string const& format, ptime const& date_time, std::string& result)
{
    try
    {
        time_facet* facet=new time_facet(format.c_str());
        std::ostringstream stream;
        stream.imbue(std::locale(stream.getloc(), facet));
        stream << date_time;
        result=stream.str();
    }
    catch (const std::exception&)
    {
        LOG("ERROR: Conversion error for " << date_time);
        result="";
    }
}


#endif // SDT_TAGWRITER_H
