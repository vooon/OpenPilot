#include "port.h"

int readBytes = 0;
port::port(PortSettings settings,QString name):mstatus(port::closed)
{
    portMutex = new QMutex();
    timer.start();
    sport = new QextSerialPort(name,settings, QextSerialPort::Polling);
    if(sport->open(QIODevice::ReadWrite|QIODevice::Unbuffered))
    {
        mstatus=port::open;
      //  sport->setDtr();
    }
    else
        mstatus=port::error;
}
port::portstatus port::status()
{
    return mstatus;
}
int16_t port::pfSerialRead(void)
{
    QMutexLocker lock(portMutex);
/*
    if (inputBuffer.length() > 0) {
        int16_t c = inputBuffer.at(0);
        inputBuffer = inputBuffer.mid(1,inputBuffer.length() - 1);
        qDebug() << "returning a buffer byte";
        return c;
    }
*/
    char c[1];
    if(sport->bytesAvailable()) {
        sport->read(c,1);
        //qDebug() << QString("Read %1").arg((uint8_t)c[0],1,16);
    }
    else
        return -1;
    //qDebug() << "Read " << (uint8_t) c[0];
    return (uint8_t)c[0];
}

void port::pfSerialWrite(uint8_t c)
{
    //QMutexLocker lock(portMutex);
/*
    usleep(100);
    while(sport->bytesAvailable()) {
        qDebug() << "Flushing byte";
        char c[1];
        sport->read(c,1);
        inputBuffer.append(c);
    }
*/

    usleep(5000);
    char cc[1];
    cc[0]=c;
    sport->write(cc,1);
    uint8_t echo[1];
    //sport->read((char*)echo,1);
    //qDebug() << QString("wrote %1 ").arg(c) << QString(" and echoed %1").arg(echo[0]);
    /*if (echo[0] != c) {
        qDebug() << "Unoh.  Probably excluding echoed data";
    }*/
}

uint32_t port::pfGetTime(void)
{
    return timer.elapsed();
}
