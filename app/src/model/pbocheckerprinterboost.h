#pragma once

#include <QString>

#include "pbocheckerprinter.h"

class PboCheckerPrinterBoost: public PboCheckerPrinter
{
public:
    PboCheckerPrinterBoost() = default;

    void printHeader(const QString& keyStr, const QString& mod = {}) override;
    void printResult(const QString& fileName, bool success, const QString& reason = {}) override;
    void println(const QString& line) override;
    void printWarn(const QString& line) override;
    void printSuccessMsg() override;
    void printFailureMsg() override;
};
