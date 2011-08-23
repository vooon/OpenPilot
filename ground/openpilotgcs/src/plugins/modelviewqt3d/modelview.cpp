#include "modelview.h"

ModelView::ModelView(QWidget *parent)
    : QGLView(parent)
    , m_scene(0)
    , m_main(0)
    , projection(1)
    , typeOfZoom(1)
    , isGLInit(0)
{
    qDebug() << "ModelView::ModelView";

    connect(camera(), SIGNAL(projectionChanged()), this, SLOT(onProjectionChanged()));
    connect(camera(), SIGNAL(viewChanged()), this, SLOT(onViewChanged()));
}

ModelView::~ModelView()
{
    qDebug() << "ModelView::~ModelView";
    delete m_scene;
}

void ModelView::reloadModel()
{
    if (!isGLInit)
        return;

    qDebug() << "ModelView::reloadModel";

    // 1. load model
    m_scene = QGLAbstractScene::loadScene(modelFilename);
    if (!m_scene) {
        qDebug() << "QGLAbstractScene::loadScene failed";
        return;
    }
    m_main = m_scene->mainNode();

    // 2. take bounding box of model
    QVector3D size = m_main->boundingBox().size();

    // 3. find maximum value
    qreal max = qMax(qMax(size.x(), size.y()), size.z());

    // 4. calculate ratio
    qreal ratio = 5.0 / max;
    QVector3D scale = QVector3D(ratio, ratio, ratio);

    // 5. scale model to (5, 5, 5) box
    QGraphicsScale3D *s = new QGraphicsScale3D(m_main);
    s->setScale(scale);
    m_main->addTransform(s);

    // 6. camera config
    QGLCamera::ProjectionType proj;
    if (projection) {
        // perspective
        proj = QGLCamera::Perspective;
        // camera()->setViewSize(QSizeF(2.0, 2.0)); // this is default value
        camera()->setMinViewSize(QSizeF(1.0, 1.0));
    } else {
        // orthographic
        proj = QGLCamera::Orthographic;
        camera()->setViewSize(QSizeF(4.0, 4.0));
        camera()->setMinViewSize(QSizeF(2.0, 2.0));
    }
    camera()->setProjectionType(proj);
    camera()->setEye(QVector3D(0.0, 2.0, 10.0));
    setOption(QGLView::FOVZoom, typeOfZoom);

}

// protected

void ModelView::initializeGL(QGLPainter *painter)
{
    qDebug() << "ModelView::initializeGL";
    Q_UNUSED(painter);

    // flag added because reloadModel() called from plugin constructor many times
    isGLInit = TRUE;
    reloadModel();
}

void ModelView::paintGL(QGLPainter *painter)
{
    painter->modelViewMatrix().rotate(m_pose);
    if (m_main)
        m_main->draw(painter);
}

// private slots

void ModelView::onProjectionChanged()
{
//    qDebug() << "onProjectionChanged()";
//    qDebug() << camera()->viewSize() << camera()->fieldOfView() << camera()->eye();

    qreal limit;
    if (projection) {
        // perspective
        limit = 5.0;
    } else {
        // orthographic
        limit = 10.0;
    }
    if (camera()->viewSize().width() > limit || camera()->viewSize().height() > limit)
        camera()->setViewSize(QSizeF(limit, limit));
}

void ModelView::onViewChanged()
{
//    qDebug() << "onViewChanged()";
//    qDebug() << camera()->viewSize() << camera()->fieldOfView() << camera()->eye();

    QVector3D viewVector = camera()->eye() - camera()->center();
    qreal length = viewVector.length();
    if (length < 8.0) {
        QRay3D viewLine(camera()->center(), viewVector.normalized());
        camera()->setEye(viewLine.point(8.0));
    } else if (length > 20.0) {
        QRay3D viewLine(camera()->center(), viewVector.normalized());
        camera()->setEye(viewLine.point(20.0));
    }
}
