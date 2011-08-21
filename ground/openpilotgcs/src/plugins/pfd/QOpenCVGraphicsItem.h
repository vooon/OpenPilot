
#ifndef QOPENCVGRPAHICSITEM_H
#define QOPENCVGRPAHICSITEM_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cv.h>

#include <QGraphicsObject>
#include <QImage>
#include <QtOpenGL>
#include <QMutex>

class CameraWorker : public QObject
{
    Q_OBJECT
public:
    CameraWorker(CvCapture * camera, QObject *parent = 0);
    ~CameraWorker();

    bool isActive() const { return m_isActive; }

public slots:
    void start();
    void stop();

    void update();

signals:
    void frameAvailable(const QImage &);

private:
    bool m_isActive;
    CvCapture *m_camera;
};

class QOpenCVGraphicsItem : public QGraphicsObject
{
    Q_OBJECT
public:
    QOpenCVGraphicsItem(QGraphicsItem *parent = 0, int camNumber = 0);
    ~QOpenCVGraphicsItem();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    bool isActive() const;

public slots:
    void start();
    void stop();

    void updateCamera();
    void updateFrame(const QImage &);

private:
    static CvCapture * camera;

    CameraWorker *m_worker;
    QThread *m_workerThread;
    QImage m_currentFrame;

    QGLContext *m_context;
    QSizeF m_size;
    GLuint m_texture;

    QTime t;
}; 

#endif
