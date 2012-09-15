#include "port.h"

//#define FLUSH_ECHO
//#define DEBUG_ECHO

int readBytes = 0;
port::port(PortSettings settings,QString name):mstatus(port::closed)
{
    portMutex = new QMutex();
    timer.start();
    sport = new QextSerialPort(name,settings, QextSerialPort::Polling);
    if(sport->open(QIODevice::ReadWrite|QIODevice::Unbuffered))
        mstatus=port::open;
    else
        mstatus=port::error;
}
port::portstatus port::status()
{
    return mstatus;
}
int16_t port::pfSerialRead(void)
{
    if (inputBuffer.length() > 0) {
        int16_t c = inputBuffer.at(0);
        inputBuffer = inputBuffer.mid(1,inputBuffer.length() - 1);
        return c;
    }

    char c[1];
    if(sport->bytesAvailable()) {
        sport->read(c,1);
#if defined(DEBUG_ECHO)
        qDebug() << QString("Read %1").arg((uint8_t)c[0],1,16);
#endif
    }
    else
        return -1;

    return (uint8_t)c[0];
}

void port::pfSerialWrite(uint8_t c)
{

#if defined(FLUSH_ECHO)
    usleep(100);
    while(sport->bytesAvailable()) {
        char c[1];
        sport->read(c,1);
        inputBuffer.append(c);
    }
#endif

    usleep(100);
    sport->write((char *) &c,1);

#if defined(FLUSH_ECHO)
    uint8_t echo[1];
    sport->read((char*)echo,1);
    if (echo[0] != c) {
        qDebug() << "Unoh.  Probably excluding echoed data";
    }
#endif

}

uint32_t port::pfGetTime(void)
{
    return timer.elapsed();
}
