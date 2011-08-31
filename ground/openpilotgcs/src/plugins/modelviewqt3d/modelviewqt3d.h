#ifndef MODELVIEW_H
#define MODELVIEW_H

#include <qglview.h>
#include <qglabstractscene.h>
#include <qgraphicsscale3d.h>
#include <qgraphicstranslation3d.h>
#include <qglcameraanimation.h>
#include <QBitArray>
#include <QEvent>
#include <QKeyEvent>
#include <QDebug>

class ModelViewQt3D : public QGLView
{
    Q_OBJECT

public:
    ModelViewQt3D(const QGLFormat &format = QGLFormat(), QWidget *parent = 0);
    ~ModelViewQt3D();

    void setModelFile(QString model)    { modelFilename = model; }
    QString getModelFile()              { return modelFilename; }

    void setWorldFile(QString world)    { worldFilename = world; }
    QString getWorldFile()              { return worldFilename; }

    void setTypeOfZoom(bool zoom)       { typeOfZoom = zoom; }
    bool getTypeOfZoom()                { return typeOfZoom; }

    void setFOV(qreal fov)              { camera()->setFieldOfView(fov); }  // don't work at init :(
    qreal getFOV()                      { return camera()->fieldOfView(); }

    void setPostprocess(QBitArray opt)  { ppOptions = opt; }
    QBitArray getPostprocess()          { return ppOptions; }

    void setAttitude(QQuaternion att)   { m_pose = att; }
    QQuaternion getAttitude()           { return m_pose; }

    void setCameraEye(QVector3D eye)    { camera()->setEye(eye); }
    QVector3D getCameraEye()            { return camera()->eye(); }

    void setCameraUp(QVector3D up)      { camera()->setUpVector(up); }
    QVector3D getCameraUp()             { return camera()->upVector(); }

    void updateAttitude()               { updateGL(); }

signals:
    void readyForUpdate();
    void focusIsChanged(bool focused);
    void fovIsChanged(qreal fov);
    void distanceIsChanged(qreal dist);

protected:
    void initializeGL(QGLPainter *painter);
    void paintGL(QGLPainter *painter);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void focusInEvent(QFocusEvent *)    { emit focusIsChanged(TRUE); }
    void focusOutEvent(QFocusEvent *)   { emit focusIsChanged(FALSE); }

private slots:
    void onProjectionChanged();
    void onViewChanged();

private:
    QGLAbstractScene    *m_scene;
    QGLSceneNode        *m_world;
    QGLSceneNode        *m_main;
    QGLLightParameters  *m_light;
    bool                typeOfZoom;
    QBitArray           ppOptions;
    bool                isGLInit;
    QString             modelFilename;
    QString             worldFilename;
    QQuaternion         m_pose;

    QGLCameraAnimation  anim;

    void initWorld();
    void initModel();
    void rotateCamera(QVector3D endPoint = QVector3D(0, -1, -3));
    QGLCamera::ProjectionType getProjection();
    QString parseOptions();
};

#endif // MODELVIEW_H
