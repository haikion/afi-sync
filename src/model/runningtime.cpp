#include <chrono>
using namespace std::chrono;

static auto startupTime = system_clock::now();

unsigned runningTimeMs()
{
    auto dT = duration_cast<milliseconds>(system_clock::now() - startupTime);
    unsigned rVal = dT.count();
    return rVal;
}

unsigned runningTimeS()
{
    auto dT = duration_cast<seconds>(system_clock::now() - startupTime);
    unsigned rVal = dT.count();
    return rVal;
}
