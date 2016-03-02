#ifndef SDT_GLOBAL_H
#define SDT_GLOBAL_H

#include <iostream>

#define SDT_VERSION 0.1

#define SDT_TAG_RAW '@'
#define SDT_TAG_VAR '#'


#define LOG(x) std::cout << x << std::endl;

typedef std::vector<std::string> stringlist;


#endif // SDT_GLOBAL_H


