
#include "QOpenCVGraphicsItem.h"
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
        //qDebug() << "time to query image:" << t.elapsed();

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
            //qDebug() << "time to convert image:" << t.elapsed() << QThread::currentThread() << qApp->thread();
        }

        if (m_isActive)
            QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }
}


CvCapture * QOpenCVGraphicsItem::camera = 0;

QOpenCVGraphicsItem::QOpenCVGraphicsItem(QGraphicsItem *parent,int camNumber) :
    QGraphicsObject(parent),
    m_texture(0),
    m_textureDirty(true)
{
    if (!camera)
        camera = cvCreateCameraCapture(camNumber);

    if (camera) {
        m_workerThread = new QThread(this);
        m_workerThread->start(QThread::LowPriority);

        m_worker = new CameraWorker(camera);
        m_worker->moveToThread(m_workerThread);

        connect(m_worker, SIGNAL(frameAvailable(QImage)),
                this, SLOT(updateFrame(QImage)), Qt::QueuedConnection);

        m_worker->start();
    } else {
        qWarning() << "Failed to load camera";
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
        //qDebug() << "Render with raster";
        painter->drawImage(boundingRect(), m_currentFrame);
        return;
    }

    //qDebug() << "Render with gl";

    QRectF br = boundingRect();
    painter->beginNativePainting();

    if (!m_texture) {
        glGenTextures(1, &m_texture);
        m_context = const_cast<QGLContext*>(QGLContext::currentContext());
        m_textureDirty = true;
    }

    int textureWidth = (m_currentFrame.width() + 3) & ~3;
    int textureHeight = (m_currentFrame.height() + 3) & ~3;

    if (m_textureDirty) {
        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGB,
                    textureWidth,
                    textureHeight,
                    0,
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    m_currentFrame.bits());

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glDisable(GL_TEXTURE_2D);

        m_textureDirty = false;
    }

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    //texture may be slightly large than svn image, ensure only used area is rendered
    qreal tw = qreal(m_currentFrame.width())/textureWidth;
    qreal th = qreal(m_currentFrame.height())/textureHeight;

    glBegin(GL_QUADS);
    glTexCoord2d(0,  0 ); glVertex3d(br.left(), br.top(), -1);
    glTexCoord2d(tw, 0 ); glVertex3d(br.right(), br.top(), -1);
    glTexCoord2d(tw, th); glVertex3d(br.right(), br.bottom(), -1);
    glTexCoord2d(0,  th); glVertex3d(br.left(), br.bottom(), -1);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    painter->endNativePainting();
}


void QOpenCVGraphicsItem::updateFrame(const QImage & frame)
{
    //qDebug() << "time since last frame:" << t.elapsed();
    t.restart();
    m_currentFrame = frame;
    m_textureDirty = true;
    update();
}
