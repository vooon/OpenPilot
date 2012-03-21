#include "notifylogging.h"
#include "notificationitem.h"
#include "mediaobject.h"

PhononObject::PhononObject()
{
     QAudioFormat format;
     // Set up the format, eg.
     format.setSampleRate(44100);
     format.setChannels(1);
     format.setSampleSize(16);
     format.setCodec("audio/pcm");
     format.setByteOrder(QAudioFormat::LittleEndian);
     format.setSampleType(QAudioFormat::SignedInt);

     QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
     qDebug() << "Found audio device: " << info.deviceName();
     QList<int> rates_(info.supportedSampleRates());
     qNotifyDebug() << "supported rates: ";
     foreach(int item, rates_)
     {
         qNotifyDebug() << item;
     }

     if (!info.isFormatSupported(format)) {
         qNotifyDebug() << "raw audio format not supported by backend, cannot play audio.";
         return;
     }

     output_audio_device_.reset(new QAudioOutput(format, this));
     connect(output_audio_device_.data(), SIGNAL(stateChanged(QAudio::State)),
             this, SLOT(finishedPlaying(QAudio::State)));
     //audio->start(&inputFile);
}

void PhononObject::enqueue(QString file)
{
    play_list_.enqueue(file);
}

void PhononObject::clearQueue()
{
    play_list_.clear();
}

QAudio::State PhononObject::state()
{
    return output_audio_device_->state();
}

QAudio::Error PhononObject::error()
{
    return output_audio_device_->error();
}

void PhononObject::play()
{
    if(play_list_.isEmpty()) return;
    //output_audio_device_.reset();
    playNext();
}

void PhononObject::clear()
{
    clearQueue();
    output_audio_device_->reset();
}

void PhononObject::stop()
{
    clear();
    output_audio_device_->stop();
}

void PhononObject::finishedPlaying(QAudio::State state)
{
    if(state == QAudio::IdleState) {
        if(!play_list_.isEmpty()) {
            playNext();
            return;
        } else {
            output_audio_device_->stop();
            //output_audio_device_.reset();
        }
    }
    emit stateChanged(state);
}

void PhononObject::playNext()
{
    if(!play_item_.isNull() && play_item_->isOpen()) {
        play_item_->close();
    }
    play_item_.reset(new QFile(play_list_.dequeue()));
    if (!play_item_->open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    output_audio_device_->start(play_item_.data());
}


