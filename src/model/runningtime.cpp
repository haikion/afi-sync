#include <chrono>
using namespace std::chrono;

static auto startupTime = system_clock::now();

unsigned runningTimeMs()
{
    auto currentTime = system_clock::now();
    auto dT = duration_cast<milliseconds>(currentTime - startupTime);
    unsigned rVal = dT.count();
    return rVal;
}
