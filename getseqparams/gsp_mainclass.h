#ifndef GSP_MAINCLASS_H
#define GSP_MAINCLASS_H

#include "../sdt_twixreader.h"


class gspMainclass
{
public:

    enum Modes {
        INVALID=0,
        SHOW,
        WRITE
    };

    gspMainclass();
    ~gspMainclass();

    void perform(int argc, char *argv[]);
    int getReturnValue();

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

