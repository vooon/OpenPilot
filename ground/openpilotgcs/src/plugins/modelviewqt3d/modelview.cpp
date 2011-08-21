#include "modelview.h"

ModelView::ModelView(QWidget *parent)
    : QGLView(parent)
{
    qDebug() << "ModelView::ModelView";
    /* 1 add teapot in scene */
    QGLBuilder builder;
    builder << QGLTeapot();
    model = builder.finalizedSceneNode();

    /* 2 add texture to teapot */
    tex.setImage(QImage("://modelview/models/tex_1.jpg"));
    QGLMaterial *mat = new QGLMaterial();
    mat->setTexture(&tex);
    model->setMaterial(mat);

    /* 3 start refresh timer */
    startTimer(50);
}

ModelView::~ModelView()
{
    qDebug() << "ModelView::~ModelView";
    delete model;
}

void ModelView::timerEvent (QTimerEvent *)
{
    angle += 1;
    if (angle > 360)
        angle = 0;
    updateGL();
}

void ModelView::paintGL(QGLPainter *painter)
{
    painter->setStandardEffect(QGL::FlatReplaceTexture2D);
    painter->modelViewMatrix().rotate(angle, 1.0, 1.0, 1.0);
    model->draw(painter);
}
