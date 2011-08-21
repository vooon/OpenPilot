#ifndef MODELVIEW_H
#define MODELVIEW_H

#include <qglview.h>
#include <qglbuilder.h>
#include <qglteapot.h>

class ModelView : public QGLView
{
    Q_OBJECT

public:
    ModelView(QWidget *parent = 0);
    ~ModelView();

protected:
    void timerEvent (QTimerEvent *);
    void paintGL(QGLPainter *painter);

private:
    QGLSceneNode *model;
    QGLTexture2D tex;
    qreal angle;
};

#endif // MODELVIEW_H
