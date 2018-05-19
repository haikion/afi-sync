#include <chrono>
using namespace std::chrono;

static auto startupTime = system_clock::now();

unsigned runningTimeMs()
{
    const auto dT = duration_cast<milliseconds>(system_clock::now() - startupTime);
    return dT.count();
}

unsigned runningTimeS()
{
    const auto dT = duration_cast<seconds>(system_clock::now() - startupTime);
    return dT.count();
}
