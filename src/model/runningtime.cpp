#include <chrono>
using namespace std::chrono;

static auto startupTime = system_clock::now();

int64_t runningTimeMs()
{
    const auto dT = duration_cast<milliseconds>(system_clock::now() - startupTime);
    return dT.count();
}

int64_t runningTimeS()
{
    const auto dT = duration_cast<seconds>(system_clock::now() - startupTime);
    return dT.count();
}
