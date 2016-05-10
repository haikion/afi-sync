#include <ctime>

unsigned runningTimeMs()
{
    return clock()*1000/CLOCKS_PER_SEC;
}
