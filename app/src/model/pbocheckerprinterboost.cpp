#include "pbocheckerprinterboost.h"

#include <QString>

#include "afisynclogger.h"

void PboCheckerPrinterBoost::printHeader(const QString& keyStr, const QString& mod)
{
    BOOST_LOG_TRIVIAL(info);
    BOOST_LOG_TRIVIAL(info) << "Verification Report";
    BOOST_LOG_TRIVIAL(info) << "===================";
    if (!mod.isEmpty())
    {
        BOOST_LOG_TRIVIAL(info) << "Mod:  " << mod;
    }
    BOOST_LOG_TRIVIAL(info) << keyStr;
}

void PboCheckerPrinterBoost::printResult(const QString& fileName, bool success, const QString& extra)
{
    QString statusStr = success ? " OK " : "FAIL";
    QString extraStr;
    if (!extra.isEmpty())
    {
        extraStr = " (" + extra + ")";
    }
    BOOST_LOG_TRIVIAL(info) << statusStr << " " << fileName << extraStr;
}

void PboCheckerPrinterBoost::println(const QString& line) {
    BOOST_LOG_TRIVIAL(info) << line;
}

void PboCheckerPrinterBoost::printWarn(const QString& line) {
    BOOST_LOG_TRIVIAL(warning) << line;
}

void PboCheckerPrinterBoost::printSuccessMsg()
{
    BOOST_LOG_TRIVIAL(info) << "Verification successful";
    BOOST_LOG_TRIVIAL(info);
}

void PboCheckerPrinterBoost::printFailureMsg()
{
    BOOST_LOG_TRIVIAL(info) << "Verification failed";
    BOOST_LOG_TRIVIAL(info);
}
