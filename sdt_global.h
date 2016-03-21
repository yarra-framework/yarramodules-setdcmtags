#ifndef SDT_GLOBAL_H
#define SDT_GLOBAL_H

#include <iostream>
#include <map>
#include <vector>

#define SDT_VERSION "0.1b1"

#define SDT_TAG_RAW '@'
#define SDT_TAG_VAR '#'
#define SDT_TAG_CNV '$'

#define SDT_VAR_KEEP       "keep"
#define SDT_VAR_SLICE      "slice"
#define SDT_VAR_SERIES     "series"
#define SDT_VAR_UID_SERIES "uid_series"
#define SDT_VAR_UID_STUDY  "uid_study"
#define SDT_VAR_ACC        "acc"
#define SDT_VAR_PROC_TIME  "proc_time"
#define SDT_VAR_PROC_DATE  "proc_date"
#define SDT_VAR_CREA_TIME  "create_time"
#define SDT_VAR_CREA_DATE  "create_date"
#define SDT_VAR_ACQ_TIME   "acq_time"
#define SDT_VAR_ACQ_DATE   "acq_date"

#define SDT_OPT_SERIESOFFSET  "SeriesOffset"
#define SDT_OPT_COLOR         "Color"
#define SDT_OPT_CLEARDEFAULTS "ClearDefaults"

#define SDT_OPT_SERIESMODE               "SeriesMode"
#define SDT_OPT_SERIESMODE_TIME          "TIME"
#define SDT_OPT_FRAMEDURATION            "FrameDuration"
#define SDT_OPT_TIMEOFFSET               "TimeOffset"

#define SDT_TRUE                         "TRUE"


#define LOG(x) std::cout << x << std::endl;

typedef std::vector<std::string> stringlist;
typedef std::map<std::string,std::string> stringmap;


#endif // SDT_GLOBAL_H
