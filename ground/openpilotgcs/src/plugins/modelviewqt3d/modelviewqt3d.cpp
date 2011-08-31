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
    if (ppOptions.testBit(4))
        m_light->setDirection(-m_light->direction());
    m_light->setDirection(QVector3D(-25,25,25));
    m_light->setAmbientColor(Qt::lightGray);

    painter->setClearColor(Qt::lightGray);
}

void ModelViewQt3D::paintGL(QGLPainter *painter)
{
    if (m_world && m_main) {
        m_world->setPosition(camera()->eye());
        m_world->draw(painter);

        painter->setMainLight(m_light);
        painter->modelViewMatrix().rotate(m_pose);
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
    emit fovIsChanged(camera()->fieldOfView());
    emit distanceIsChanged(camera()->eye().length());
}

void ModelViewQt3D::onViewChanged()
{
    QVector3D center = camera()->center();
    QVector3D viewVector = camera()->eye() - center;
    qreal length = viewVector.length();
    QVector3D normal = viewVector.normalized();
    QRay3D viewLine(center, normal);
    length = qBound(1.74, length, 10.0);
    camera()->setEye(viewLine.point(length));
//    qDebug() << this << "view changed"
//             << camera()->eye().length() << "\t" << camera()->fieldOfView();
    camera()->setUpVector(QVector3D(0,1,0));
    emit fovIsChanged(camera()->fieldOfView());
    emit distanceIsChanged(camera()->eye().length());
}

// private /////////////////////////////////////////////////////////////////////////////////////////

void ModelViewQt3D::initWorld()
{
    if (!isGLInit)
        return;
    qDebug() << this << "initWorld";

    m_scene = QGLAbstractScene::loadScene(":/modelviewqt3d/models/world.dae");
    if (!m_scene) {
        qCritical() << this << "loadWorld failed, alarm, alarm!";
        return;
    }
    m_world = m_scene->mainNode();

    QVector3D size = m_world->boundingBox().size();
    qreal max = qMax(qMax(size.x(), size.y()), size.z());
    qreal ratio = 50.0 / max;
    QVector3D scale = QVector3D(ratio, ratio, ratio);
    QGraphicsScale3D *s = new QGraphicsScale3D(m_world);
    s->setScale(scale);

    m_world->addTransform(s);
    QVector3D center = m_world->boundingBox().center();
    QVector3D trans = QVector3D(center.x(), center.y(), center.z());
    QGraphicsTranslation3D *t = new QGraphicsTranslation3D(m_world);
    t->setTranslate(-trans);

    m_world->addTransform(t);
    m_world->setEffect(QGL::FlatReplaceTexture2D);
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
