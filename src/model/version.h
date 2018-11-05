#ifndef VERSION_H
#define VERSION_H

//Compatibility layer for windows resource file.

//int to String converter
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAJOR_VERSION 0
#define MINOR_VERSION 46
#define VERSION_CHARS "v" STR(MAJOR_VERSION) "." STR(MINOR_VERSION) "rc"

#endif // VERSION_H
