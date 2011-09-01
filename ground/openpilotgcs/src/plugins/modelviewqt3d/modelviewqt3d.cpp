#include "modelviewqt3d.h"

ModelViewQt3D::ModelViewQt3D(const QGLFormat& format, QWidget *parent)
    : QGLView(format, parent)
    , m_scene(0)
    , m_world(0)
    , m_main(0)
    , m_light(0)
    , typeOfZoom(0)
    , ppOptions(8)
    , isGLInit(0)
{
    qDebug() << this << "constructed";
    ppOptions.setBit(0, TRUE);
    anim.setCamera(camera());

    connect(camera(), SIGNAL(projectionChanged()), this, SLOT(onProjectionChanged()));
    connect(camera(), SIGNAL(viewChanged()), this, SLOT(onViewChanged()));
}

ModelViewQt3D::~ModelViewQt3D()
{
    qDebug() << this << "deconstructed";
    delete m_scene;
}

// protected ///////////////////////////////////////////////////////////////////////////////////////

void ModelViewQt3D::initializeGL(QGLPainter *painter)
{
    qDebug() << this << "initializeGL";
    isGLInit = TRUE;
    initWorld();
    initModel();

    // camera config
    camera()->setProjectionType(getProjection());
    setOption(QGLView::FOVZoom, typeOfZoom);

    m_light = new QGLLightParameters(this);
    m_light->setDirection(QVector3D(-25,25,25));
    m_light->setAmbientColor(Qt::lightGray);

    painter->setClearColor(Qt::lightGray);
}

void ModelViewQt3D::paintGL(QGLPainter *painter)
{
    if (m_world && m_main) {
        m_world->setPosition(camera()->eye());
        m_world->draw(painter);

        painter->modelViewMatrix().rotate(m_pose);
        painter->setMainLight(m_light);
        m_main->draw(painter);
    }
}

void ModelViewQt3D::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    rotateCamera();
}

void ModelViewQt3D::keyPressEvent(QKeyEvent *e)
{
//    qDebug() << this << e->key();
    QVector3D rot = QVector3D();
    switch (e->key()) {
        case Qt::Key_0:             // change zoom
            typeOfZoom = !typeOfZoom;
            setOption(QGLView::FOVZoom, typeOfZoom);
            break;
        case Qt::Key_2:             // back
            rot = QVector3D(0,-1,-2.5);
            break;
        case Qt::Key_4:             // left
            rot = QVector3D(2.5,-1,0);
            break;
        case Qt::Key_8:             // forward
            rot = QVector3D(0,-1,2.5);
            break;
        case Qt::Key_6:             // right
            rot = QVector3D(-2.5,-1,0);
            break;
        default:
            break;
    }
    if (rot != QVector3D())
        rotateCamera(rot);

    QGLView::keyPressEvent(e);
}

// private slots ///////////////////////////////////////////////////////////////////////////////////

void ModelViewQt3D::onProjectionChanged()
{
//    qDebug() << this << "proj changed"
//             << camera()->eye().length() << "\t" << camera()->fieldOfView();

    camera()->setUpVector(QVector3D(0,1,0));

    qreal fov = camera()->fieldOfView();
    if (fov > 120.0) {
        camera()->setFieldOfView(120.0);
    } else if (fov < 20.0) {
        camera()->setFieldOfView(20.0);
    }

    emit fovIsChanged(camera()->fieldOfView());
}

void ModelViewQt3D::onViewChanged()
{
//    qDebug() << this << "view changed"
//             << camera()->eye().length() << "\t" << camera()->fieldOfView();
    camera()->setUpVector(QVector3D(0,1,0));

    QVector3D center = camera()->center();
    QVector3D viewVector = camera()->eye() - center;
    QVector3D normal = viewVector.normalized();
    QRay3D viewLine(center, normal);

    qreal length = viewVector.length();
    if (length > 8.0) {
        camera()->setEye(viewLine.point(8.0));
    } else if (length < 1.74) {
        camera()->setEye(viewLine.point(1.74));
    }

    emit distanceIsChanged(camera()->eye().length());
}

// private /////////////////////////////////////////////////////////////////////////////////////////

void ModelViewQt3D::initWorld()
{
    if (!isGLInit)
        return;
    qDebug() << this << "initWorld";

    QGLBuilder builder;
    builder << QGLSphere(50.0);
    m_world = builder.finalizedSceneNode();

    QUrl url;
    url.setPath(worldFilename);
    url.setScheme(QString("file"));

    QGLMaterial *mat = new QGLMaterial();
    mat->setTextureUrl(url);

    m_world->setMaterial(mat);
    m_world->setEffect(QGL::FlatReplaceTexture2D);

    QGraphicsRotation3D *rot;
    rot = new QGraphicsRotation3D(m_world);
    rot->setAxis(QVector3D(1,0,0));
    rot->setAngle(-90);
    m_world->addTransform(rot);

    QGeometryData *data;
    data = new QGeometryData(m_world->allChildren().takeAt(0)->geometry());
    for (int i = 0; i < data->normals().count(); ++i) {
        data->normal(i) *= -1;
    }
    for (int i = 0; i < data->texCoords().count(); ++i) {
        data->texCoord(i).setX(1 - data->texCoord(i).x());
    }
}

void ModelViewQt3D::initModel()
{
    if (!isGLInit)
        return;
    qDebug() << this << "initModel";

/*
 * 1. load model
 */
    QString op = parseOptions();
    m_scene = QGLAbstractScene::loadScene(modelFilename, QString(), op);
    if (!m_scene) {
        qWarning() << this << "loadScene failed, use buit-in model";
        m_scene = QGLAbstractScene::loadScene(":/modelviewqt3d/models/cube.dae");
        if (!m_scene) {
            qCritical() << this << "loadScene failed twice, alarm, alarm!";
            return;
        }
    }
    m_main = m_scene->mainNode();

/*
 * 2. take bounding box of model, calculate ratio to fit model in (2,2,2) box
 */
    QVector3D size = m_main->boundingBox().size();
    qreal max = qMax(qMax(size.x(), size.y()), size.z());
    qreal ratio = 2.0 / max;
    QVector3D scale = QVector3D(ratio, ratio, ratio);

/*
 * 3. scale model to (2, 2, 2) box
 */
    QGraphicsScale3D *s = new QGraphicsScale3D(m_main);
    s->setScale(scale);
    m_main->addTransform(s);

/*
 * 4. translate center of model to origin
 */
    QVector3D center = m_main->boundingBox().center();
    QVector3D trans = QVector3D(center.x(), center.y(), center.z());
    QGraphicsTranslation3D *t = new QGraphicsTranslation3D(m_main);
    t->setTranslate(-trans);
    m_main->addTransform(t);

/*
 * 5. additional steps
 */
//    m_main->setEffect(QGL::LitModulateTexture2D);

}

void ModelViewQt3D::rotateCamera(QVector3D endPoint)
{
    if (qFuzzyCompare(camera()->eye(), endPoint))
        return;
    if (camera()->center() == QVector3D())
        camera()->setCenter(QVector3D(0,0,0.001));

    anim.setStartCenter(camera()->center());
    anim.setStartEye(camera()->eye());
    anim.setStartUpVector(camera()->upVector());
    anim.setEndEye(endPoint);
    anim.setDuration(1000);
    anim.setEasingCurve(QEasingCurve::InOutCubic);

    anim.start();
}

QGLCamera::ProjectionType ModelViewQt3D::getProjection()
{
    QGLCamera::ProjectionType proj;
    proj = QGLCamera::Perspective;
    camera()->setFieldOfView(60.0);
    camera()->setNearPlane(0.1);
    return proj;
}

QString ModelViewQt3D::parseOptions()
{
    QStringList options;
    options << "NoOptions "
            << "CalculateNormals "
            << "ForceFaceted "
            << "Optimize2 "
            << ""               // reserved
            << ""               // reserved
            << ""               // reserved
            << "ShowWarnings ";
    QString op = "";
    for (int i=0; i<8; i++) {
        if (ppOptions.testBit(i))
            op.append(options.at(i));
    }
//    qDebug() << this << "options for AssImp" << op;
    return op;
}
