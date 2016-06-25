#include "sdt_tagwriter.h"
#include "sdt_twixreader.h"

#include "dcmtk/dcmdata/dcpath.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmdata/dctk.h"

#include "external/mdfdsman.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/dcmdata/dctk.h"

#include <stdlib.h>
#include <climits>

#include "boost/date_time/posix_time/posix_time.hpp"


#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif


sdtTagWriter::sdtTagWriter()
{
    slice      =0;
    series     =0;
    sliceCount =0;
    seriesCount=0;

    seriesUID="";
    studyUID ="";

    approxCreationTime=true;
    raidDateTime="";

    frameDuration=0;
    dcmRows=0;
    dcmCols=0;

    is3DScan=false;
    sliceArraySize=1;

    inputFilename ="";
    outputFilename="";
    inputPath     ="";
    outputPath    ="";

    mapping   =nullptr;
    options   =nullptr;
    twixReader=nullptr;

    tags.clear();

    seriesOffset=0;

    imagePositionPatient   ="";
    imageOrientationPatient="";
    sliceLocation          ="";
    sliceThickness         ="";
    pixelSpacing           ="";
    slicesSpacing          ="";
}


void sdtTagWriter::setTWIXReader(sdtTWIXReader* instance)
{
    twixReader=instance;
}


void sdtTagWriter::setFolders(std::string inputFolder, std::string outputFolder)
{
    inputPath=inputFolder;
    outputPath=outputFolder;

    if (inputPath[inputPath.length()-1]!='/')
    {
        inputPath.append("/");
    }

    if (outputPath[outputPath.length()-1]!='/')
    {
        outputPath.append("/");
    }
}


void sdtTagWriter::setFile(std::string filename, int currentSlice, int totalSlices, int currentSeries, int totalSeries, std::string currentSeriesUID, std::string currentStudyUID)
{
    inputFilename =inputPath +filename;
    outputFilename=outputPath+filename;

    slice      =currentSlice;
    series     =currentSeries;
    seriesUID  =currentSeriesUID;
    studyUID   =currentStudyUID;
    sliceCount =totalSlices;
    seriesCount=totalSeries;

    dcmRows=0;
    dcmCols=0;
}


void sdtTagWriter::setMapping(stringmap* currentMapping, stringmap* currentOptions)
{
    mapping=currentMapping;
    options=currentOptions;

    if (options->find(SDT_OPT_SERIESOFFSET)!=options->end())
    {
        // Read the series offset from the options. Make sure it's set to 0 if the value is invalid
        seriesOffset=strtol((*options)[SDT_OPT_SERIESOFFSET].c_str(),nullptr,10);
        if ((seriesOffset==LONG_MAX) || (seriesOffset==LONG_MIN))
        {
            seriesOffset=0;
        }
    }
}


bool sdtTagWriter::processFile()
{
    OFCondition result=EC_Normal;
    MdfDatasetManager ds_man;

    // Load file into dataset manager
    result=ds_man.loadFile(inputFilename.c_str());

    if (result.bad())
    {
        LOG("ERROR: Unable to load file " << inputFilename);
        return false;
    }

    // Read the width and height of the current DICOM file, as needed, e.g., for calculating the pixel spacing
    ds_man.getDataset()->findAndGetLongInt(DcmTagKey(0x0028, 0x0010),dcmRows);
    ds_man.getDataset()->findAndGetLongInt(DcmTagKey(0x0028, 0x0011),dcmCols);

    // Now calulate all dynamic variables
    calculateVariables();
    calculateOrientation();

    // Prepare the list of DICOM tags to be written
    tags.clear();
    for (auto& mapEntry : *mapping)
    {
        std::string dcmPath=mapEntry.first;
        std::string value="";

        // Obtain the value to be written into the DICOM tag. Write it into the results
        // array only if indicated by the return value from getTagValue
        if (getTagValue(mapEntry.second, value))
        {
            tags[dcmPath]=value;
        }
    }

    // debug
    /*
    std::cout << "--- Series " << series << ", Slice " << slice << "---" << std::endl;
    for (auto entry : tags)
    {
        std::cout << entry.first << "=" << entry.second << std::endl;
    }
    std::cout << "-----------------------------" << std::endl << std::endl;
    */

    // Modify DICOM tags in loaded file
    for (auto& tag : tags)
    {
        result=ds_man.modifyOrInsertPath(tag.first.c_str(), tag.second.c_str(), OFFalse);

        if (result.bad())
        {
            LOG("ERROR: Unable to set tag " << tag.first << " in " << inputFilename << " (" << result.text() << ")");
        }
    }

    // Save modified file into output folder
    result=ds_man.saveFile(outputFilename.c_str());
    if (result.bad())
    {
        LOG("ERROR: Unable to write file " << outputFilename);
        return false;
    }

    return true;
}


bool sdtTagWriter::getTagValue(std::string mapping, std::string& value, int recurCount)
{
    // If mapped entry is variable
    if (mapping[0]==SDT_TAG_VAR)
    {
        bool writeTag=true;
        std::string variable=mapping.substr(1,std::string::npos);

        if (variable==SDT_VAR_SLICE)
        {
            value=std::to_string(slice);
        }

        if (variable==SDT_VAR_SERIES)
        {
            value=std::to_string(series+seriesOffset);
        }

        if (variable==SDT_VAR_SLICE_COUNT)
        {
            value=std::to_string(sliceCount);
        }

        if (variable==SDT_VAR_SERIES_COUNT)
        {
            value=std::to_string(seriesCount);
        }

        if (variable==SDT_VAR_UID_SERIES)
        {
            value=seriesUID;
        }

        if (variable==SDT_VAR_UID_STUDY)
        {
            value=studyUID;
        }

        if (variable==SDT_VAR_ACC)
        {
            value=accessionNumber;

            if (value.empty())
            {
                writeTag=false;
            }
        }

        if (variable==SDT_VAR_PROC_TIME)
        {
            formatDateTime("%H%M%s", processingTime, value);
        }

        if (variable==SDT_VAR_PROC_DATE)
        {
            formatDateTime("%Y%m%d", processingTime, value);
        }

        if (variable==SDT_VAR_CREA_TIME)
        {
            formatDateTime("%H%M%s", creationTime, value);
        }

        if (variable==SDT_VAR_CREA_DATE)
        {
            formatDateTime("%Y%m%d", creationTime, value);
        }

        if (variable==SDT_VAR_ACQ_TIME)
        {
            formatDateTime("%H%M%s", acquisitionTime, value);
        }

        if (variable==SDT_VAR_ACQ_DATE)
        {
            formatDateTime("%Y%m%d", acquisitionTime, value);
        }

        if (variable==SDT_VAR_PROTNAME_FRAME)
        {
            value=twixReader->getValue("ProtocolName")+", T"+std::to_string(series-1);
        }

        if (variable==SDT_VAR_DURATION_FRAME)
        {
            value=std::to_string(int(frameDuration));
        }

        if (variable==SDT_VAR_IMAGE_POSITION)
        {
            value=imagePositionPatient;
        }

        if (variable==SDT_VAR_IMAGE_ORIENTATION)
        {
            value=imageOrientationPatient;
        }

        if (variable==SDT_VAR_SLICE_LOCATION)
        {
            value=sliceLocation;
        }

        if (variable==SDT_VAR_SLICE_THICKNESS)
        {
            value=sliceThickness;
        }

        if (variable==SDT_VAR_PIXEL_SPACING)
        {
            value=pixelSpacing;
        }

        if (variable==SDT_VAR_SLICES_SPACING)
        {
            value=slicesSpacing;
        }

        if (variable==SDT_VAR_KEEP)
        {
            value="";
            writeTag=false;
        }

        return writeTag;
    }

    // If mapped entry is rawfile entry
    if (mapping[0]==SDT_TAG_RAW)
    {
        std::string rawfileKey=mapping.substr(1,std::string::npos);
        value=twixReader->getValue(rawfileKey);
        return true;
    }

    // If mapped entry is a conversion of a rawfile entry
    if (mapping[0]==SDT_TAG_CNV)
    {
        // Avoid multi-level nested recursion in macros
        if (recurCount>1)
        {
            value="";
        }
        else
        {
            size_t sepPos=mapping.find(",");

            std::string rawfileKey=mapping.substr(5,sepPos-5);
            std::string arg=mapping.substr(sepPos+1,mapping.length()-2-sepPos);
            std::string func=mapping.substr(1,3);

            // Send the first argument to the parser again
            value="";
            getTagValue(rawfileKey, value, recurCount+1);

            if (func=="DIV")
            {
                value=eval_DIV(value, arg);
            }

            if (func=="EXT")
            {
                // Also parse the argument value
                std::string argValue="";
                getTagValue(arg, argValue, recurCount+1);
                value=value+argValue;
            }
        }

        return true;
    }

    // If mapped entry is static entry
    value=mapping;
    return true;
}


std::string sdtTagWriter::eval_DIV(std::string value, std::string arg)
{
    //LOG("DBG: " << value << " " << arg);

    if (value.empty())
    {
        return "0";
    }

    int decimals=-1;

    size_t sepPos=arg.find(",");
    if (sepPos!=std::string::npos)
    {
        decimals=atoi(arg.substr(sepPos+1,std::string::npos).c_str());
        arg=arg.substr(0,sepPos);
    }

    float val=0;
    float div=0;
    try
    {
        val=stof(value);
        div=stof(arg);
    }
    catch (const std::exception&)
    {
        return "0";
    }

    if (div!=0)
    {
        value=std::to_string(val/div);

        // If maximum number of decimals has been specified
        if (decimals>=0)
        {
            sepPos=value.find(".");
            if (sepPos!=std::string::npos)
            {
                // If no decimals are requested, delete the . as well
                if (decimals==0)
                {
                    decimals=-1;
                }

                size_t eraseStart=sepPos+1+decimals;

                if (eraseStart<value.length()-1)
                {
                    value.erase(eraseStart,std::string::npos);
                }
            }
        }

        return value;
    }
    else
    {
        return "0";
    }
}


void sdtTagWriter::calculateVariables()
{
    // Calculate all internal variables that need to be updated for different slices / series

    double frameTime=0;
    bool   timeOffsetFound=false;

    frameDuration=0;
    bool   frameDurationFound=false;

    if (options->find(SDT_OPT_TIMEOFFSET)!=options->end())
    {
        // Read time offset from options and convert into float
        std::string frameTimeStr=(*options)[SDT_OPT_TIMEOFFSET];
        try
        {
            frameTime=stof(frameTimeStr);
        }
        catch (const std::exception&)
        {
            frameTime=0;
        }

        timeOffsetFound=true;
    }

    if (options->find(SDT_OPT_FRAMEDURATION)!=options->end())
    {
        // Read frame duration from options and convert into float
        std::string frameDurationStr=(*options)[SDT_OPT_FRAMEDURATION];
        try
        {
            frameDuration=stof(frameDurationStr);
        }
        catch (const std::exception&)
        {
            frameDuration=0;
        }

        frameDurationFound=true;
    }


    // If dynamic mode has been selected, add the time interval to the base time (creationTime)
    if (options->find(SDT_OPT_SERIESMODE)!=options->end())
    {
        // If the current series should be in color mode
        if (boost::to_upper_copy((*options)[SDT_OPT_SERIESMODE])==SDT_OPT_SERIESMODE_TIME)
        {
            std::string scanTimeStr=twixReader->getValue("TotalScanTimeSec");

            // If not explicit time offset has been given, estimate time point
            // based on total scan duration and number of series
            if (!timeOffsetFound)
            {
                if (!scanTimeStr.empty())
                {
                    frameTime=std::stod(scanTimeStr);
                    frameTime=frameTime/double(seriesCount) * (0.5 + series - 1);
                }

                //LOG("DBG: Total duration " << std::stod(scanTimeStr));
                //LOG("DBG: Time series mode, total duration = " << frameTime);
            }

            if (!frameDurationFound)
            {
                if (!scanTimeStr.empty())
                {
                    // Estimate the frame duration in ms
                    frameDuration=std::stod(scanTimeStr)/double(seriesCount)*1000;
                }
            }
        }
    }

    if (frameTime!=0)
    {
        // Add frametime to creation time

        long frameSec=long(frameTime);
        long frameMSec=long((frameTime-frameSec)*1000);

        acquisitionTime=creationTime + seconds(frameSec) + milliseconds(frameMSec);
    }
    else
    {
        acquisitionTime=creationTime;
    }

    // TODO: Set duration tag

    if (twixReader->getValue("MRAcquisitionType")=="3D")
    {
        is3DScan=true;
    }
    else
    {
        is3DScan=false;
    }

    sliceArraySize=1;
    if (!twixReader->getValue("mrprot.sSliceArray.lSize").empty())
    {
        sliceArraySize=twixReader->getValueInt("mrprot.sSliceArray.lSize");

        if (sliceArraySize==0)
        {
            sliceArraySize=1;
        }
    }
}


void sdtTagWriter::prepareTime()
{
    processingTime=second_clock::local_time();

    if (!raidDateTime.empty())
    {
        // If an exact acquisition time has been provided through the task file
        approxCreationTime=false;

        // TODO: Implement task reader
        creationTime=second_clock::local_time();  // dbg
    }
    else
    {
        // If an exact acquisition time has not been provided, use the time obtained
        // from the frame-of-reference entry
        approxCreationTime=true;
        LOG("WARNING: Using approximative acquisition time based on reference scans.");

        std::string timeString=twixReader->getValue("FrameOfReference_Date")+" "+twixReader->getValue("FrameOfReference_Time");
        try
        {
            creationTime=ptime(time_from_string(timeString));
        }
        catch (const std::exception&)
        {
            creationTime=second_clock::local_time();
        }
    }

    // Unless overwritten via an option, the acquisition time should be identical to the
    acquisitionTime=creationTime;
}


void sdtTagWriter::calculateOrientation()
{
    // Use the 0th slab for 3D sequences for now. Multi-slab 3D scans are not yet
    // properly supported here
    int sliceToUse=0;

    if (!is3DScan)
    {
        // Read the slice information for each slice from the TWIX file
        // TODO: Validate in 2D TWIX file if this makes sense
        sliceToUse=slice;

        if (sliceToUse>=sliceArraySize)
        {
            sliceToUse=sliceArraySize-1;
        }
    }

    std::string pathBase="mrprot.sSliceArray.asSlice["+std::to_string(sliceToUse)+"].";

    arma::rowvec center(3);
    arma::vec    normal(3);
    arma::vec    fov(3);

    center(0)=twixReader->getValueDouble(pathBase+"sPosition.dSag");
    center(1)=twixReader->getValueDouble(pathBase+"sPosition.dCor");
    center(2)=twixReader->getValueDouble(pathBase+"sPosition.dTra");

    normal(0)=twixReader->getValueDouble(pathBase+"sNormal.dSag");
    normal(1)=twixReader->getValueDouble(pathBase+"sNormal.dCor");
    normal(2)=twixReader->getValueDouble(pathBase+"sNormal.dTra");

    fov(0)=twixReader->getValueDouble(pathBase+"dPhaseFOV"  );
    fov(1)=twixReader->getValueDouble(pathBase+"dReadoutFOV");
    fov(2)=0;

    double thickness =twixReader->getValueDouble(pathBase+"dThickness" );
    double inplaneRot=twixReader->getValueDouble(pathBase+"dInPlaneRot");

    if (is3DScan)
    {
        double sliceThickness3D=thickness/sliceCount;
        double sliceShift = sliceThickness3D*(slice-1)-sliceThickness3D*(sliceCount-1)/2 ;
        center(2) += sliceShift;

        sliceThickness=std::to_string(sliceThickness3D);

        // 3D sequences normally don't have slice gaps (multi-slab protocols not covered here)
        slicesSpacing=std::to_string(0.);
    }
    else
    {
        sliceThickness=std::to_string(thickness);

        // TODO: Check how slice spacing is read from TWIX file
    }

    arma::mat ImgOri;
    ImgOri  << 1 << 0 << 0 << arma::endr
            << 0 << 1 << 0 << arma::endr
            << 0 << 0 << 1 << arma::endr;

    //arma::mat normorig = ImgOri.col(2);

    double n = 10000000;
    arma::vec normal2 = arma::round(normal*n) / n;

    double beta = acos(normal(2));
    beta = round(beta * 180 / M_PI) / 180 * M_PI;
    arma::mat Rx;
    Rx << 1 << 0         << 0          << arma::endr
       << 0 << cos(beta) << -sin(beta) << arma::endr
       << 0 << sin(beta) << cos(beta)  << arma::endr;

    arma::mat m_001; m_001 << 0 << 0 << 1;
    arma::mat m_010; m_010 << 0 << 0 << 1;
    auto alpha = -acos(dot(normal2, Rx * m_001.t()));
    auto Rz = rotation_matrix(alpha, Rx * m_010.t());

    auto C = cos(inplaneRot);
    auto S = sin(inplaneRot);
    auto OMC = 1.0-C;
    auto uX = normal(0);
    auto uY = normal(1);
    auto uZ = normal(2);

    arma::mat Rip;
    Rip << C + uX*uX*OMC    << uX*uY*OMC + uZ*S << uX*uZ*OMC - uY*S << arma::endr
        << uX*uY*OMC - uZ*S << C + uY*uY*OMC    << uY*uZ*OMC + uX*S << arma::endr
        << uX*uZ*OMC + uY*S << uY*uZ*OMC - uX*S << C + uZ*uZ*OMC    << arma::endr;

    arma::mat ImgDir = (Rip * Rz * Rx * ImgOri.t()).t();
    arma::mat ImgPos = center - ImgDir.row(0) * fov(0)/2 - ImgDir.row(1) * fov(1)/2;

    std::stringstream ImagePositionPatient;
    ImagePositionPatient << std::fixed << ImgPos(0) << "\\" << ImgPos(1) << "\\" << ImgPos(2);
    imagePositionPatient=ImagePositionPatient.str();

    std::stringstream ImageOrientationPatient;
    ImageOrientationPatient
            << int(round(ImgDir(0 , 0))) << "\\" << int(round(ImgDir(0 , 1))) << "\\" << int(round(ImgDir(0 , 2))) << "\\"
            << int(round(ImgDir(1 , 0))) << "\\" << int(round(ImgDir(1 , 1))) << "\\" << int(round(ImgDir(1 , 2)));
    imageOrientationPatient=ImageOrientationPatient.str();

    sliceLocation=std::to_string(center(2));

    // TODO: Calculate dwelltime, pixelspacing, check slice spacing, acquisition matrix
}


arma::mat sdtTagWriter::rotation_matrix(double theta, const arma::mat & rotationAxis)
{
    auto C = cos(theta);
    auto S = sin(theta);
    auto OMC = 1.0-C;
    auto uX = rotationAxis(0);
    auto uY = rotationAxis(1);
    auto uZ = rotationAxis(2);
    arma::mat R;
    R   << C + uX*uX*OMC    << uX*uY*OMC + uZ*S << uX*uZ*OMC - uY*S << arma::endr
    << uX*uY*OMC - uZ*S << C + uY*uY*OMC    << uY*uZ*OMC + uX*S << arma::endr
    << uX*uZ*OMC + uY*S << uY*uZ*OMC - uX*S << C +uZ*uZ*OMC     << arma::endr;
    return R;
}


/*
// TODO
bool TagsLookupTable::calcImageOrientation(std::ostream & log_stream)
{
    bool success = true;

    int sliceNum = -1;
    try
    {
        sliceNum = stoi(tag_variables[TagsLookupTable::variableSlice]);
    }
    catch (...)
    {
        log_stream << "Cannot read slice number as int: " << tag_variables[TagsLookupTable::variableSlice] << std::endl;
    }
    std::string base_path = "sSliceArray.asSlice[" + std::to_string(sliceNum) + "].";
    std::string base_path_zero = "sSliceArray.asSlice[0].";

//    % slice center
//    sliceobj = mrprot{1}.sSliceArray.asSlice(k);
//
//    try center(1) = sliceobj.sPosition.dSag; catch center(1) = 0; end;
//    try center(2) = sliceobj.sPosition.dCar; catch center(2) = 0; end;
//    try center(3) = sliceobj.sPosition.dTra; catch center(3) = 0; end;
//    try normal(1) = sliceobj.sNormal.dSag; catch normal(1) = 0; end;
//    try normal(2) = sliceobj.sNormal.dCor; catch normal(2) = 0; end;
//    try normal(3) = sliceobj.sNormal.dTra; catch normal(3) = 0; end;

    arma::rowvec center(3);
    success &= getMRprotValue({base_path + "sPosition.dSag", base_path_zero + "sPosition.dSag"}, center(0), log_stream);
    success &= getMRprotValue({base_path + "sPosition.dCor", base_path_zero + "sPosition.dCor"}, center(1), log_stream);
    success &= getMRprotValue({base_path + "sPosition.dTra", base_path_zero + "sPosition.dTra"}, center(2), log_stream);

    arma::vec normal(3);
    success &= getMRprotValue({base_path + "sNormal.dSag", base_path_zero + "sNormal.dSag"}, normal(0), log_stream, 0);
    success &= getMRprotValue({base_path + "sNormal.dCor", base_path_zero + "sNormal.dCor"}, normal(1), log_stream, 0);
    success &= getMRprotValue({base_path + "sNormal.dTra", base_path_zero + "sNormal.dTra"}, normal(2), log_stream, 0);

    std::string SliceThickness;
    success &= getMRprotValue({base_path + "dThickness", base_path_zero + "dThickness"}, SliceThickness, log_stream);
    tag_variables["SliceThickness"] = SliceThickness;

    if (tag_variables["MRAcquisitionType"] == "3D")
    {
        try
        {
            int imageNum = std::stoi(tag_variables["NoImagesPerSlab"]);

            double sliceThick = std::stod(SliceThickness)/imageNum;
            tag_variables["SliceThickness"] = std::to_string(sliceThick);

            double sliceShift = sliceThick*(sliceNum-1) - sliceThick*(imageNum-1)/2 ;
            center(2) += sliceShift;
        }
        catch (...)
        {
            log_stream << "Unable calculate sliceShift for 3D case " << std::endl;
            return false;
        }
    }

//        % this accounts to AP,PA, RL, LR
//        % inplaneRot = sliceobj.dInPlaneRot; %in radians
//        inplaneRot = 0;
    double inplaneRot = 0;
//
//        rot = rotation_matrix(acos(dot([0,0,1],normal)),cross(normal,[0,0,1]));
//        FOV = [sliceobj.dPhaseFOV,sliceobj.dReadoutFOV,0];
    arma::vec FOV(3);
    success &= getMRprotValue({base_path + "dPhaseFOV", base_path_zero + "dPhaseFOV"}, FOV(0), log_stream);
    success &= getMRprotValue({base_path + "dReadoutFOV", base_path_zero + "dReadoutFOpathBaseV"}, FOV(1), log_stream);
    FOV(2) = 0;

//        ImgOri = [1,0,0;0,1,0;0,0,1];
    arma::mat ImgOri;
    ImgOri  << 1 << 0 << 0 << arma::endr
            << 0 << 1 << 0 << arma::endr
            << 0 << 0 << 1 << arma::endr;

//        normorig = ImgOri(:,3);
    arma::mat normorig = ImgOri.col(2);

//        n = 10000000;
//        normal2 = round(normal*n)/n;
    double n = 10000000;
    arma::vec normal2 = arma::round(normal*n) / n;

//        % % option: rotate around X and then around Y
//        % rotate around X
//        beta = acos(normal(3));
//        beta = round(beta*180/pi)/180*pi;
//        %beta = atan(norm(normal2(1:2))/normal2(3));
//        Rx = [1 0 0;0 cos(beta) -sin(beta);0 sin(beta) cos(beta)];
    double beta = acos(normal(2));
    beta = round(beta * 180 / datum::pi) / 180 * datum::pi;
    arma::mat Rx;
    Rx  << 1 << 0         << 0          << arma::endr
        << 0 << cos(beta) << -sin(beta) << arma::endr
        << 0 << sin(beta) << cos(beta)  << arma::endr;

//        % rotate around new Y
//        alpha = -acos(dot(normal2,Rx*[0,0,1]'));
//        %                 alpha = -round(-alpha*180/pi)/180*pi;
//        Rz = rotation_matrix(alpha,Rx*[0,1,0]');

    arma::mat m_001; m_001 << 0 << 0 << 1;
    arma::mat m_010; m_010 << 0 << 0 << 1;
    auto alpha = -acos(dot(normal2, Rx * m_001.t()));
    auto Rz = rotation_matrix(alpha, Rx * m_010.t());

//        %                 % % option: rotate around X and then around Z
//        %                 % rotate around X
//        %                 beta = acos(normal(3)/norm(normal));
//        %                 Rx = [1 0 0;0 cos(beta) -sin(beta);0 sin(beta) cos(beta)];
//        %
//        %                 % rotate around z
//        %                 if (norm(normal(1:2)) > 0)
//        %                     alpha = -acos(normal(2)/norm(normal(1:2)))-pi;
//        %                     Rz = [cos(alpha) -sin(alpha) 0;sin(alpha) cos(alpha) 0; 0 0 1];
//        %                 else
//        %                     Rz = eye(3);
//        %                 end;
//
//        % % option: rotate around Y and then around Z
//        %                 % rotate around Y
//        %                 beta = acos(normal(3)/norm(normal));
//        %                 Rx = [cos(beta) 0 sin(beta);0 1 0;-sin(beta) 0 cos(beta)];
//        %
//        %                 % rotate around z
//                                          %                 if (norm(normal(1:2)) > 0)
//        %                     alpha = -acos(normal(1)/norm(normal(1:2)));
//        %                     Rz = [cos(alpha) -sin(alpha) 0;sin(alpha) cos(alpha) 0; 0 0 1];
//        %                 else
//        %                     Rz = eye(3);
//        %                 end;
//
//        % in plane rotation
//        C = cos(inplaneRot);
//        S = sin(inplaneRot);
//        OMC = 1.0-C;
//        uX = normal(1);
//        uY = normal(2);
//        uZ = normal(3);

    auto C = cos(inplaneRot);
    auto S = sin(inplaneRot);
    auto OMC = 1.0-C;
    auto uX = normal(0);
    auto uY = normal(1);
    auto uZ = normal(2);

//        Rip = [C+uX^2*OMC uX*uY*OMC+uZ*S uX*uZ*OMC-uY*S; ...
//        uX*uY*OMC-uZ*S C+uY^2*OMC uY*uZ*OMC+uX*S; ...
//        uX*uZ*OMC+uY*S uY*uZ*OMC-uX*S C+uZ^2*OMC ];

    arma::mat Rip;
    Rip << C + uX*uX*OMC    << uX*uY*OMC + uZ*S << uX*uZ*OMC - uY*S << arma::endr
        << uX*uY*OMC - uZ*S << C + uY*uY*OMC    << uY*uZ*OMC + uX*S << arma::endr
        << uX*uZ*OMC + uY*S << uY*uZ*OMC - uX*S << C + uZ*uZ*OMC    << arma::endr;

//        ImgDir = (Rip*Rz*Rx*ImgOri')';
//        ImgPos = center - ImgDir(1,:)*FOV(1)/2 - ImgDir(2,:)*FOV(2)/2;
    arma::mat ImgDir = (Rip * Rz * Rx * ImgOri.t()).t();
    arma::mat ImgPos = center - ImgDir.row(0) * FOV(0)/2 - ImgDir.row(1) * FOV(1)/2;

//        %                 % test calculations
//        %                 if (l == 1)
//        %                     v1 = hdrdcmtmp.ImageOrientationPatient(1:3);
//        %                     v2 = hdrdcmtmp.ImageOrientationPatient(4:6);
//        %                     diffangles = acos([dot(v1,ImgDir(1,:)),dot(v2,ImgDir(2,:)),dot(normal,ImgDir(3,:))])*180/pi;
//        %                     if (max(diffangles) > 0.1)
//        %                         display('Discrepancy in ImageOrientationPatient!')
//        %                         diffangles
//        %                     end;
//        %                     if (max(abs(hdrdcmtmp.ImagePositionPatient'-ImgPos)) > 0.1)
//        %                         display('Discrepancy in ImagePositionPatient!')
//        %                         hdrdcmtmp.ImagePositionPatient'
//        %                     end;
//        %                     % make a figure
//        %                     b = normal;
//        %                     figure;axis equal;hold on;plot3([0,b(1)],[0,b(2)],[0,b(3)],'o-k');
//        %                     for ii = 1:3
//        %                         plot3([0,ImgDir(ii,1)],[0,ImgDir(ii,2)],[0,ImgDir(ii,3)],'o-r');
//        %                         plot3([0,ImgOri(ii,1)],[0,ImgOri(ii,2)],[0,ImgOri(ii,3)],'o-b');
//        %                     end;
//        %                     plot3([0,v1(1)],[0,v1(2)],[0,v1(3)],'o-k');
//        %                     plot3([0,v2(1)],[0,v2(2)],[0,v2(3)],'o-k');
//        %                 end;
//
//        headers{i,j,k}.EchoNumber = j;

//        headers{i,j,k}.ImagePositionPatient = ImgPos';
    std::stringstream ImagePositionPatient;

    //-129.340954421\-132\-49.99485386
    ImagePositionPatient << std::fixed << ImgPos(0) << "\\" << ImgPos(1) << "\\" << ImgPos(2);
    tag_variables["ImagePositionPatient"] = ImagePositionPatient.str();

//        headers{i,j,k}.ImageOrientationPatient = [ImgDir(1,:)';ImgDir(2,:)'];
    std::stringstream ImageOrientationPatient;
    ImageOrientationPatient
            << int(round(ImgDir(0 , 0))) << "\\" << int(round(ImgDir(0 , 1))) << "\\" << int(round(ImgDir(0 , 2))) << "\\"
            << int(round(ImgDir(1 , 0))) << "\\" << int(round(ImgDir(1 , 1))) << "\\" << int(round(ImgDir(1 , 2)));
    tag_variables["ImageOrientationPatient"] = ImageOrientationPatient.str();
//    arma::mat(join_vert(ImgDir.row(0).t(), ImgDir.row(1).t())).save(ImageOrientationPatient, arma::raw_ascii);


//        headers{i,j,k}.Private_0019_1015 = headers{i,j,k}.ImagePositionPatient;
//        headers{i,j,k}.SliceLocation = center(3);%headers{i,j,k}.ImagePositionPatient(3);
    tag_variables["SliceLocation"] = std::to_string(center(2));

//        headers{i,j,k}.InstanceNumber = l;l = l+1;
//        headers{i,j,k}.AcquisitionNumber = i;

    return success;
}
*/

