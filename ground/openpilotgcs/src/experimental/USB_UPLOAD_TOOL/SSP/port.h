#ifndef PORT_H
#define PORT_H
#include <stdint.h>
#include "../../../libs/qextserialport/src/qextserialport.h"
#include <QTime>
#include <QDebug>
#include <QMutex>
#include "common.h"

class port
{
public:
    enum portstatus{open,closed,error};
    //! function to read a character from the serial input stream
    virtual int16_t pfSerialRead(void);

    //! function to write a byte to be sent out the serial port
    virtual void pfSerialWrite( uint8_t );
    virtual uint32_t pfGetTime(void);
    uint8_t		retryCount;
    //! max. times to try to transmit the 'send' packet
    uint8_t 	maxRetryCount;
    //! Maximum number of retrys for a single transmit.
    uint16_t 	max_retry;
    //! how long to wait for each retry to succeed
    int32_t 	timeoutLen;
    //! current timeout. when 'time' reaches this point we have timed out
    int32_t		timeout;
    //! current 'send' packet sequence number
    uint8_t 	txSeqNo;
    //!  current buffer position in the receive packet
    uint16_t 	rxBufPos;
    //! number of 'data' bytes in the buffer
    uint16_t	rxBufLen;
    //! current 'receive' packet number
    uint8_t 	rxSeqNo;
    //! size of the receive buffer.
    uint16_t 	rxBufSize;
    //! size of the transmit buffer.
    uint16_t 	txBufSize;
    //! transmit buffer. REquired to store a copy of packet data in case a retry is needed.
    uint8_t		*txBuf;
    //! receive buffer. Used to store data as a packet is received.
    uint8_t		*rxBuf;
    //! flag to indicate that we should send a synchronize packet to the host
    uint16_t    sendSynch;
    // this is required when switching from the application to the bootloader
    // and vice-versa. This fixes the firwmare download timeout.
    // when this flag is set to true, the next time we send a packet we will first                                                                                 // send a synchronize packet.
    ReceiveState	InputState;
    decodeState_	DecodeState;
    uint16_t		SendState;
    uint16_t		crc;
    uint32_t		RxError;
    uint32_t		TxError;
    uint16_t		flags;
    port(PortSettings settings,QString name);
    portstatus status();
private:
    portstatus mstatus;
    QMutex *portMutex;
    QTime timer;
    QextSerialPort *sport;

    QByteArray inputBuffer;
};

#endif // PORT_H
