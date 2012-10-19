#ifndef DOWNLOADPAGE_H
#define DOWNLOADPAGE_H

#include <Qt/QtNetwork>
#include <QDebug>


class DownloadPage:QObject
{
    Q_OBJECT
public:
    DownloadPage();
    void startDownload(QUrl urlToCall);

private slots:
    void downloadComplete(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
};

#endif // DOWNLOADPAGE_H
