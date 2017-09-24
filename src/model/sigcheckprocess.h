#ifndef SIGCHECKPROCESS_H
#define SIGCHECKPROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QSet>

class SigCheckProcess : public QProcess
{
   Q_OBJECT

public:
   enum class CheckStatus {
       NOT_STARTED,
       CHECKING,
       SUCCESS,
       FAILURE
   };
   Q_ENUM(CheckStatus)

   SigCheckProcess(const QString& modPath, const QSet<QString>& filePaths);
   SigCheckProcess(CheckStatus startStatus);

   QString modPath() const;
   CheckStatus checkStatus() const;

public slots:
   void start();

signals:
   void checkDone(SigCheckProcess::CheckStatus status);

private slots:
   void printOutput();
   void processDone();
   void handleError(QProcess::ProcessError error);

private:
   static const QString DS_CHECK_SIGNATURE_PATH;
   QString modPath_;
   CheckStatus checkStatus_;
   QString findKeyFolder(const QSet<QString>& filePaths);
};

#endif // SIGCHECKPROCESS_H
