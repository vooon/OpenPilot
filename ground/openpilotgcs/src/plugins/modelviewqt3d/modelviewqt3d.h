#ifndef MODELVIEW_H
#define MODELVIEW_H

#include <qglview.h>
#include <qglabstractscene.h>
#include <qgraphicsscale3d.h>
#include <QBitArray>
#include <QColor>
#include <QDebug>

class QGLAbstractScene;
class QGLSceneNode;

class ModelView : public QGLView
{
    Q_OBJECT

public:
    ModelView(QWidget *parent = 0);
    ~ModelView();
    void setModelFile(QString filename) { modelFilename = filename; }
    void setBackground(QString bg) { worldBgFilename = bg; }
    void setProjection(bool proj) { projection = proj; }
    void setTypeOfZoom(bool zoom) { typeOfZoom = zoom; }
    void setPostprocess(QBitArray opt) { ppOptions = opt; }
    void setAttitude(QQuaternion att) { m_pose = att; }
    void updateAttitude() { updateGL(); }
    void reloadModel();

protected:
    void initializeGL(QGLPainter *painter);
    void paintGL(QGLPainter *painter);

private slots:
    void onProjectionChanged();
    void onViewChanged();

private:
    QGLAbstractScene *m_scene;
    QGLSceneNode *m_main;
    QGLLightParameters *m_light;
    bool projection;
    bool typeOfZoom;
    QBitArray ppOptions;
    bool isGLInit;
    QString modelFilename;
    QString worldBgFilename;
    QQuaternion m_pose;
};

#endif // MODELVIEW_H
