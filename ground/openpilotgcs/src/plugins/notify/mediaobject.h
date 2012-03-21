#ifndef MEDIAOBJECT_H
#define MEDIAOBJECT_H

#include <QScopedPointer>
#include <QQueue>
#include <QFile>
#include <QtMultimedia/QAudioOutput>

class NotificationItem;

class PhononObject : public QObject
{
    Q_OBJECT

public:
    PhononObject();
    void enqueue(QString file);
    void play();
    void clear();
    void stop();
    void clearQueue();
    QAudio::State state();
    QAudio::Error error();

signals:
    void stateChanged(QAudio::State state);

public slots:
    void finishedPlaying(QAudio::State state);

private:

    void playNext();

private:
    QQueue<QString> play_list_;
    QScopedPointer<QFile> play_item_;
    //Phonon::MediaObject* mo;
    QScopedPointer<QAudioOutput> output_audio_device_;
    NotificationItem* notify_to_play_;
public:
    bool firstPlay;
};

#endif // MEDIAOBJECT_H
