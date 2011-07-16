
#ifndef QOPENCVWIDGET_H
#define QOPENCVWIDGET_H
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cv.h>
#include <QPixmap>
#include <QLabel>
#include <QGraphicsPixmapItem>
#include <QVBoxLayout>
#include <QImage>
#include <QGraphicsScene>
class QOpenCVGraphicsItem : public QObject,public QGraphicsPixmapItem {
    Q_OBJECT
    private:
         QImage image;
         CvCapture * camera;
         void putImage(IplImage *);
    public:
        QOpenCVGraphicsItem(QGraphicsItem *parent = 0,int camNumber = 0);
        ~QOpenCVGraphicsItem();
    public slots:
        void camupdate();
}; 

#endif
