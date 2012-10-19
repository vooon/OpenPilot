#include "downloadpage.h"
#include "updatelogic.h"

DownloadPage::DownloadPage()
{
    qDebug()<<"Download Page constructor called";
    manager = new QNetworkAccessManager(this);
    connect(manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(downloadComplete(QNetworkReply*)));
}

void DownloadPage::startDownload(QUrl urlToCall){
    qDebug()<<"Downloading Page...";
    manager->get(QNetworkRequest(urlToCall));
    qDebug()<<"Page call end but not finished yet";
}

void DownloadPage::downloadComplete(QNetworkReply *reply){
    qDebug()<<"Page call finished";
    if(reply->error()){
        if(reply->error() == QNetworkReply::HostNotFoundError)
            qDebug()<<"not found ...";
        else
            qDebug()<<"there was an error:" + reply->errorString();
    }
    else{
        QByteArray byteArray = reply->readAll();
        UpdateLogic ul;
        ul.parseXML(byteArray);
    }
    reply->deleteLater();
}
