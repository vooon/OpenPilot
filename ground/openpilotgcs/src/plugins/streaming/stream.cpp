#include "stream.h"
#include <QDebug>
#include <QtGlobal>

Stream::Stream(QObject *parent) :
    QIODevice(parent)
{
    connect(&timer, SIGNAL(timeout()), this, SLOT(timerFired()));
}

/**
 * Opens the logfile QIODevice and the underlying logfile. In case
 * we want to save the logfile, we open in WriteOnly. In case we
 * want to read the logfile, we open in ReadOnly.
 */
bool Stream::open(OpenMode mode) {

    // start a timer for playback
    myTime.restart();
    if (file.isOpen()) {
        // We end up here when doing a replay, because the connection
        // manager will also try to open the QIODevice, even though we just
        // opened it after selecting the file, which happens before the
        // connection manager call...
        return true;
    }

    if(file.open(mode) == FALSE)
    {
        qDebug() << "Unable to open " << file.fileName() << " for streaming";
        return false;
    }

    // TODO: Write a header at the beginng describing objects so that in future
    // they can be read back if ID's change

    // Must call parent function for QIODevice to pass calls to writeData
    // We always open ReadWrite, because otherwise we will get tons of warnings
    // during a logfile replay. Read nature is checked upon write ops below.
    QIODevice::open(QIODevice::ReadWrite);

    return true;
}

void Stream::close()
{
    emit aboutToClose();

    if (timer.isActive())
        timer.stop();
    file.close();
    QIODevice::close();
}

qint64 Stream::writeData(const char * data, qint64 dataSize) {
    if (!file.isWritable())
        return dataSize;

    quint32 timeStamp = myTime.elapsed();

    file.write((char *) &timeStamp,sizeof(timeStamp));
    file.write((char *) &dataSize, sizeof(dataSize));

    qint64 written = file.write(data, dataSize);
    if(written != -1)
        emit bytesWritten(written);

    return dataSize;
}

qint64 Stream::readData(char * data, qint64 maxSize) {
    QMutexLocker locker(&mutex);
    qint64 toRead = qMin(maxSize,(qint64)dataBuffer.size());
    memcpy(data,dataBuffer.data(),toRead);
    dataBuffer.remove(0,toRead);
    return toRead;
}

qint64 Stream::bytesAvailable() const
{
    return dataBuffer.size();
}

void Stream::timerFired()
{
    qint64 dataSize;

    if(file.bytesAvailable() > 4)
    {

	int time;
	time = myTime.elapsed();

        // TODO: going back in time will be a problem
        while ((lastPlayed + ((time - timeOffset)* playbackSpeed) > lastTimeStamp)) {
	    lastPlayed += ((time - timeOffset)* playbackSpeed);
            if(file.bytesAvailable() < 4) {
                stopReplay();
                return;
            }

            file.read((char *) &dataSize, sizeof(dataSize));

	    if (dataSize<1 || dataSize>(1024*1024)) {
	        qDebug() << "Error: Stream corrupted! Unlikely packet size: " << dataSize << "\n";
		stopReplay();
		return;
	    }
            if(file.bytesAvailable() < dataSize) {
                stopReplay();
                return;
            }

            mutex.lock();
            dataBuffer.append(file.read(dataSize));
            mutex.unlock();
            emit readyRead();

            if(file.bytesAvailable() < 4) {
                stopReplay();
                return;
            }

            int save=lastTimeStamp;
            file.read((char *) &lastTimeStamp,sizeof(lastTimeStamp));
	    // some validity checks
	    if (lastTimeStamp<save // logfile goies back in time
		    || (lastTimeStamp-save) > (60*60*1000)) { // gap of more than 60 minutes)
		qDebug() << "Error: Stream corrupted! Unlikely timestamp " << lastTimeStamp << " after "<< save << "\n";
		stopReplay();
		return;
            }

            timeOffset = time;
            time = myTime.elapsed();

        }
    } else {
        stopReplay();
    }

}

bool Stream::startReplay() {
    dataBuffer.clear();
    myTime.restart();
    timeOffset = 0;
    lastPlayed = 0;
    playbackSpeed = 1;
    file.read((char *) &lastTimeStamp,sizeof(lastTimeStamp));
    timer.setInterval(10);
    timer.start();
    emit replayStarted();
    return true;
}

bool Stream::stopReplay() {
    close();
    emit replayFinished();
    return true;
}

void Stream::pauseReplay()
{
    timer.stop();
}

void Stream::resumeReplay()
{
    timeOffset = myTime.elapsed();
    timer.start();
}

