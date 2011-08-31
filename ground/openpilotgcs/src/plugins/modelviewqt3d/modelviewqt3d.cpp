#include "modelview.h"

ModelView::ModelView(QWidget *parent)
    : QGLView(parent)
    , m_scene(0)
    , m_main(0)
    , m_light(0)
    , projection(1)
    , typeOfZoom(1)
    , ppOptions(8)
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
/*    options:
 *           NoOptions,
 *           ShowWarnings,        // show any warnings while loading the file
 *           CalculateNormals,    // replace normals from the file with smooth generated ones
 *           ForceFaceted,        // generate non-smooth normals (implies CalculateNormals)
 *           ForceSmooth,         // deprecated - retained only for backward compatibility
 *           IncludeAllMaterials, // include even redundant (unused) materials
 *           IncludeLinesPoints,  // include even collapsed triangles (lines or points)
 *           FixNormals,          // try to fix incorrect (in facing) normals
 *           DeDupMeshes,         // replace copied meshes with refs to a single instance
 *           Optimize,            // collapse meshes, nodes & scene heierarchies
 *           FlipUVs,             // flips UV's on the y-axis (for upside-down textures)
 *           FlipWinding,         // makes faces CW instead of CCW
 *           UseVertexColors,     // use vertex colors that are in a model
 *           VertexSplitLimitx2,  // double the vertex count which will split a large mesh
 *           TriangleSplitLimitx2 // double the triangle count which will split a large mesh
 ********************************************************************************************/
    QString op = "";
    if (ppOptions.testBit(0))
        op.append("Optimize ");     // leave a space at end!
    if (ppOptions.testBit(1))
        op.append("Optimize2 ");
    if (ppOptions.testBit(2))
        op.append("");
    if (ppOptions.testBit(3))
        op.append("CalculateNormals ");
    if (ppOptions.testBit(4))
        op.append("ForceFaceted ");
    if (ppOptions.testBit(5))
        op.append("FixNormals ");
    if (ppOptions.testBit(6))
        op.append("");
    if (ppOptions.testBit(7))
        op.append("");

    m_scene = QGLAbstractScene::loadScene(modelFilename, QString(), op);
    if (!m_scene) {
        qWarning() << "QGLAbstractScene::loadScene failed, use buitin model";
        m_scene = QGLAbstractScene::loadScene(":/modelviewqt3d/models/warning_sign.obj");
        if (!m_scene) {
            qCritical() << "QGLAbstractScene::loadScene failed twice, alarm, alarm!";
            return;
        }
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
    camera()->setEye(QVector3D(0.0, 0.0, -10.0));
    setOption(QGLView::FOVZoom, typeOfZoom);

    // 7. world light
    m_light = new QGLLightParameters(this);
    m_light->setPosition(QVector3D(0.0f, 20.0f, -10.0f));
    m_light->setAmbientColor(Qt::darkGray);
}

// protected

void ModelView::initializeGL(QGLPainter *painter)
{
    qDebug() << "ModelView::initializeGL";
//    Q_UNUSED(painter);

    // flag added because reloadModel() called from plugins constructor many times
    isGLInit = TRUE;
    reloadModel();
    painter->setMainLight(m_light);
}

void ModelView::paintGL(QGLPainter *painter)
{
    painter->modelViewMatrix().rotate(m_pose);
//    m_light->setPosition(camera()->eye());
//    painter->setMainLight(m_light);
    if (m_main)
        m_main->draw(painter);
}

// private slots

void ModelView::onProjectionChanged()
{
//    qDebug() << "proj changed" << camera()->viewSize();
    qreal limit;
    if (projection) {
        // perspective
        limit = 5.0;
    } else {
        // orthographic
        limit = 10.0;
    }
    QSizeF view = camera()->viewSize();
    if (view.width() > limit || view.height() > limit)
        camera()->setViewSize(QSizeF(limit, limit));
}

void ModelView::onViewChanged()
{
//    qDebug() << "view changed" << camera()->eye();
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
