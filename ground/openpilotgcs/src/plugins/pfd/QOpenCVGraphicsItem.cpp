
#include "QOpenCVGraphicsItem.h"
#include <stdio.h>
//#include <QtMultimedia/qvideoframe.h>
#include <QDebug>

CameraWorker::CameraWorker(CvCapture *camera, QObject *parent) :
    QObject(parent),
    m_isActive(false),
    m_camera(camera)
{
}

CameraWorker::~CameraWorker()
{
}

void CameraWorker::start()
{
    if (!m_isActive) {
        m_isActive = true;
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }
}

void CameraWorker::stop()
{
    m_isActive = false;
}

void CameraWorker::update()
{
    if (m_camera && m_isActive) {
        QTime t;
        t.start();
        IplImage *cvImage = cvQueryFrame(m_camera);
        qDebug() << "time to query image:" << t.elapsed();

        if (cvImage) {
            t.restart();
            if (cvImage->depth == IPL_DEPTH_8U && cvImage->nChannels == 3) {
                QImage img((const uchar*)cvImage->imageData,
                           cvImage->width,
                           cvImage->height,
                           cvImage->widthStep,
                           QImage::Format_RGB888);

                img = img.rgbSwapped();
                emit frameAvailable(img);
            }
            qDebug() << "time to convert image:" << t.elapsed() << QThread::currentThread() << qApp->thread();
        }

        if (m_isActive)
            QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }
}


CvCapture * QOpenCVGraphicsItem::camera = 0;

QOpenCVGraphicsItem::QOpenCVGraphicsItem(QGraphicsItem *parent,int camNumber) :
    QGraphicsObject(parent)
{
    if (!camera)
        camera = cvCreateCameraCapture(camNumber);

    if (!camera)
        qDebug() << "Failed to load camera";
    else {
        qDebug() << Q_FUNC_INFO << cvGetCaptureProperty(camera,CV_CAP_PROP_FPS);
        for (int i=0; i<19; i++)
            qDebug() << "Camera property" << i << cvGetCaptureProperty(camera,i);

        m_workerThread = new QThread(this);
        m_workerThread->start(QThread::LowPriority);

        m_worker = new CameraWorker(camera);
        m_worker->moveToThread(m_workerThread);

        connect(m_worker, SIGNAL(frameAvailable(QImage)),
                this, SLOT(updateFrame(QImage)), Qt::QueuedConnection);

        m_worker->start();
    }
}

QOpenCVGraphicsItem::~QOpenCVGraphicsItem()
{
    qDebug() << Q_FUNC_INFO << "*****************************";
    m_worker->stop();
    m_worker->deleteLater();
    //cvReleaseCapture(&camera);
}

void QOpenCVGraphicsItem::start()
{
    m_worker->start();
}

void QOpenCVGraphicsItem::stop()
{
    m_worker->stop();
}

QRectF QOpenCVGraphicsItem::boundingRect() const
{
    return QRectF(0, 0, scene()->sceneRect().width(), scene()->sceneRect().height());
}

void QOpenCVGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (m_currentFrame.isNull()) {
        painter->fillRect(boundingRect(), Qt::black);
        return;
    }

    if (painter->paintEngine()->type() != QPaintEngine::OpenGL &&
            painter->paintEngine()->type() != QPaintEngine::OpenGL2) {
        painter->drawImage(boundingRect(), m_currentFrame);
    } else {

    }

}

void QOpenCVGraphicsItem::updateCamera()
{
}

void QOpenCVGraphicsItem::updateFrame(const QImage & frame)
{
    qDebug() << "time since last frame:" << t.elapsed();
    t.restart();
    m_currentFrame = frame;
    update();
}
