#ifndef GSP_MAINCLASS_H
#define GSP_MAINCLASS_H

#include "../sdt_twixreader.h"

#define GSP_COLS_SEPARATOR "#"


class gspMainclass
{
public:

    enum Modes {
        INVALID=0,
        SHOW,
        WRITE,
        INDEX
    };

    gspMainclass();
    ~gspMainclass();

    void perform(int argc, char *argv[]);
    int getReturnValue();

    bool generateCSV(std::string searchPath, std::string csvFilename, std::string csvCols);


    // Helper class to parse TWIX files
    sdtTWIXReader twixReader;

    Modes mode;
    int   returnValue;

private:
    static std::string const summaryItems[];
};


inline int gspMainclass::getReturnValue()
{
    return returnValue;
}


#endif // GSP_MAINCLASS_H

