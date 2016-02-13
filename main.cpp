#include <sdt_mainclass.h>


int main(int argc, char *argv[])
{
    sdtMainclass instance;
    instance.perform(argc, argv);
    return instance.getReturnValue();
}




