#include <gsp_mainclass.h>

int main(int argc, char *argv[])
{
    gspMainclass instance;
    instance.perform(argc, argv);
    return instance.getReturnValue();
}


