#include "updatelogic.h"
#include <QtDebug>
#include <QtXml>
#include <coreplugin/coreconstants.h>
#include <QList>
#include "devicedescriptorstruct.h"
#include "infodialog.h"

UpdateLogic::UpdateLogic()
{
    buildDate = QLocale(QLocale::C).toDate(QString(__DATE__).simplified(), QLatin1String("MMM d yyyy")).toString("yyyyMMdd");

    downloadPage = new DownloadPage();
}

void UpdateLogic::checkForNewGCS(){
    qDebug("let's do it!");

    qDebug()<<buildDate;

    //    void (*functionPointer)(QString);
    //    functionPointer = printName;
    //    functionPointer(QString("hello"));

    downloadPage->startDownload(QUrl("http://www.the-trowbridge-analogue.ch/version.xml"));
}

void UpdateLogic::parseXML(QByteArray response){
    InfoDialog infoDialog;
    // set modal to true so the user can't do anything in the GCS except in the UpdateDialog
    infoDialog.setModal(true);


    qDebug("I got the XML!");
    QDomDocument doc;
    doc.setContent(response);

    QList<GCSVersion> allGCSVersions;

    QDomElement root = doc.firstChildElement();
    QDomNodeList gcsVersions = root.elementsByTagName("GCSVersion");
    qDebug()<<"items count"<<gcsVersions.count();
    for (int i = 0; i < gcsVersions.count(); i++){
        QDomNode gcsVersion = gcsVersions.at(i);

        if(gcsVersion.isElement()){
            QDomElement gcsVersionElement = gcsVersion.toElement();
            QDomElement releaseDate = gcsVersionElement.elementsByTagName("releaseDate").at(0).toElement();
            QDomElement url = gcsVersionElement.elementsByTagName("url").at(0).toElement();

            allGCSVersions.append(GCSVersion(QString(releaseDate.text()),QString(url.text())));
        }
    }

    qSort(allGCSVersions.begin(),     allGCSVersions.end(),GCSVersion::versionCompare);

    //         for (int i = 0; i < allVersions.count(); i++){
    //         qDebug()<<allVersions[i];
    //     }
    GCSVersion latestVersion =      allGCSVersions.last();
    qDebug()<<latestVersion.releaseDate;

    if(latestVersion.releaseDate>buildDate){
        qDebug("there is a new version!");

        const QString description = tr(
                    "There is a new version! You can download it here: <a href=\"%1\" target=\"_blank\">%1</a>")
                .arg(latestVersion.URL);

        infoDialog.SetMainLabelText(description);
    }
    else
    {
        qDebug("you sir, you are up to date!");
        infoDialog.SetMainLabelText(QString("you sir, you are up to date!"));
    }
    // execute (open) the dialog
    infoDialog.exec();

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager* utilMngr = pm->getObject<UAVObjectUtilManager>();
    QString serial = utilMngr->getBoardCPUSerial().toHex();
    qDebug()<<serial;
}

void UpdateLogic::printName(QString name)
{
    qDebug()<<name;
}


GCSVersion::GCSVersion(QString releaseDate, QString url){
    this->releaseDate = releaseDate;
    this->URL = url;
}

bool GCSVersion::versionCompare(const GCSVersion &i, const GCSVersion &j){
    return i.releaseDate < j.releaseDate;
}
