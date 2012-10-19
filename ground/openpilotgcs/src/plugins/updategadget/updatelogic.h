#ifndef UPDATELOGIC_H
#define UPDATELOGIC_H

#include "downloadpage.h"
#include "extensionsystem/pluginmanager.h"
//#include "uavobjectmanager.h"
//#include "uavobject.h"
#include "uavobjectutilmanager.h"

class UpdateLogic:QObject
{
    Q_OBJECT

public:
    UpdateLogic();
    void checkForNewGCS();
    void parseXML(QByteArray response);
    void printName(QString name);


private:
    DownloadPage *downloadPage;
    QString buildDate;

};

class GCSVersion{
public:
    GCSVersion(QString releaseDate,QString url);
    static bool versionCompare(const GCSVersion &i, const GCSVersion &j);
    QString releaseDate;
    QString URL;
};


#endif // UPDATELOGIC_H


