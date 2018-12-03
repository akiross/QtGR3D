#include "scatter3d.h"

#include <iostream>
#include <QOpenGLBuffer>
#include <QPainter>
#include "GL/gl.h"

#include <QLabel>


Scatter3D::Scatter3D(QWidget *parent):
    QOpenGLWidget(parent),
//    m_rotation(0, 0),
    m_nVertices(0)
//    m_program(nullptr),
//    m_posAttr(0),
//    m_colAttr(0),
//    m_matrixUniform(0)
{
    setFocusPolicy(Qt::StrongFocus);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    m_axisColor = QColor(255, 255, 255);

    // Prepare axis texture
    m_axisLabelImage = QImage(500, 300, QImage::Format_ARGB32);
    m_axisLabelImage.fill(Qt::transparent);
    QPainter lp(&m_axisLabelImage);
//    lp.fillRect(0, 0, 500, 100, Qt::white);
//    lp.fillRect(0, 100, 500, 100, Qt::gray);
//    lp.fillRect(0, 200, 500, 100, Qt::white);
    lp.drawText(0, 0, 500, 100, Qt::AlignCenter, "This js THE Variable ² Total ASS");
    lp.drawText(0, 100, 500, 100, Qt::AlignCenter, "This js THE Variable ² Total COLL");
    lp.drawText(0, 200, 500, 100, Qt::AlignCenter, "This js THE Variable ² Total g,UOCOO");
//    QLabel *magic = new QLabel();
//    magic->setPixmap(QPixmap::fromImage(m_axisLabelImage));
//    magic->show();

    resetRanges();
}

Scatter3D::~Scatter3D()
{
    makeCurrent();
    foreach (const char *key, m_programs.keys()) {
        delete m_programs[key];
    }
    doneCurrent();
}

void Scatter3D::addSeries(const QVector<QVector3D> &vertices, QColor color)
{
    m_entities.append(Entity{color, vertices, false});
}

void Scatter3D::setRangeX(float min, float max)
{
    m_ranges(0, 0) = min;
    m_ranges(0, 1) = max;
    update();
}

void Scatter3D::setRangeY(float min, float max)
{
    m_ranges(1, 0) = min;
    m_ranges(1, 1) = max;
    update();
}

void Scatter3D::setRangeZ(float min, float max)
{
    m_ranges(2, 0) = min;
    m_ranges(2, 1) = max;
    update();
}

void Scatter3D::resetRanges() {
    m_ranges.fill(0);
}

void Scatter3D::computeLimits()
{
    if (m_entities.size() == 0) {
        // No values, make this a null box
        m_limits.fill(0);
        return;
    } else {
        m_limits = m_entities[0].maxmin();
    }
    for (int i = 1; i < m_entities.size(); i++) {
        auto el = m_entities[i].maxmin();
        for (int j = 0; j < 3; j++) {
            m_limits(j, 0) = qMin(m_limits(j, 0), el(j, 0));
            m_limits(j, 1) = qMax(m_limits(j, 1), el(j, 1));
        }
    }
}

void Scatter3D::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(static_cast<float>(m_bgColor.redF()),
                 static_cast<float>(m_bgColor.greenF()),
                 static_cast<float>(m_bgColor.blueF()),
                 0.0f);

    glPointSize(3.0);
    glLineWidth(3.0);

    std::vector<float> cubeCoords = {
        -1, -1, -1,
         1, -1, -1,
         1,  1, -1,
        -1,  1, -1,
        -1, -1,  1,
         1, -1,  1,
         1,  1,  1,
        -1,  1,  1,
    };
    std::vector<float> cubeIndices = {
        6, 5, 2, 1, 3, 0, 7, 4,
        2, 3, 6, 7, 5, 4, 1, 0,
    };

//    m_vaos["axis"]

    m_vaos["data"] = new QOpenGLVertexArrayObject(this);
    m_vaos["data"]->create();
    m_vaos["data"]->bind();

    // Create buffers to store axis
    m_vbos["axis"] = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbos["axis"]->create();
    m_vbos["axis"]->bind();

    float axRed = static_cast<float>(m_axisColor.redF());
    float axGreen = static_cast<float>(m_axisColor.redF());
    float axBlue = static_cast<float>(m_axisColor.redF());

    float l = 0.5f;
    std::vector<float> axisVert {
        -l,  l, -l, axRed, axGreen, axBlue,
        -l, -l, -l, axRed, axGreen, axBlue,
         l, -l, -l, axRed, axGreen, axBlue,
        -l, -l, -l, axRed, axGreen, axBlue,
        -l, -l, -l, axRed, axGreen, axBlue,
        -l, -l,  l, axRed, axGreen, axBlue
    };
    m_vbos["axis"]->allocate(axisVert.data(), static_cast<int>(sizeof(float) * axisVert.size()));

    m_vbos["points"] = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbos["points"]->create();

    m_vbos["curves"] = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbos["curves"]->create();

    m_textures["axis"] = new QOpenGLTexture(m_axisLabelImage.mirrored());
    m_textures["axis"]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_textures["axis"]->setMagnificationFilter(QOpenGLTexture::Linear);

    static const char *vertexShaderSource =
        "#version 430 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aCol;\n"
        "out vec3 outCol;\n"
        "uniform mat4 transform = mat4(1.0);\n"
        "uniform mat3x3 ranges = mat3x3(0);\n"
        "uniform mat3x3 limits = mat3x3(0);\n"
        "void main() {\n"
        "    mat3x3 wat = ranges;\n"
        "    if (ranges[0][1] <= ranges[0][0]) {\n"
        "        wat[0][0] = limits[0][0];\n"
        "        wat[0][1] = limits[0][1];\n"
        "    }\n"
        "    if (ranges[1][1] <= ranges[1][0]) {\n"
        "        wat[1][0] = limits[1][0];\n"
        "        wat[1][1] = limits[1][1];\n"
        "    }\n"
        "    if (ranges[2][1] <= ranges[2][0]) {\n"
        "        wat[2][0] = limits[2][0];\n"
        "        wat[2][1] = limits[2][1];\n"
        "    }\n"
        "    vec3 pos = aPos;\n"
        "    pos.x = (aPos.x - wat[0][0]) / (wat[0][1] - wat[0][0]) - 0.5;\n"
        "    pos.y = (aPos.y - wat[1][0]) / (wat[1][1] - wat[1][0]) - 0.5;\n"
        "    pos.z = (aPos.z - wat[2][0]) / (wat[2][1] - wat[2][0]) - 0.5;\n"
        "    gl_Position = transform * vec4(pos, 1.0);\n"
        "    outCol = aCol;\n"
        "}\n";
    static const char *fragmentShaderSource =
        "#version 430 core\n"
        "in vec3 outCol;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "   FragColor = vec4(outCol, 1.0f);\n"
        "}\n";

    m_programs["pos-col"] = new QOpenGLShaderProgram(this);
    if (!m_programs["pos-col"]->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        std::cout << "Compilation error!" << std::endl;
        std::cout << m_programs["pos-col"]->log().toStdString() << std::endl;
        exit(0);
    }
    if (!m_programs["pos-col"]->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        std::cout << "Compilation error!" << std::endl;
        std::cout << m_programs["pos-col"]->log().toStdString() << std::endl;
        exit(0);
    }
    if (!m_programs["pos-col"]->link()) {
        std::cout << "Linking error!" << std::endl;
        std::cout << m_programs["pos-col"]->log().toStdString() << std::endl;
        exit(0);
    }

    m_attribs["pos-col"] = m_programs["pos-col"]->attributeLocation("posAttr");
    m_attribs["pos-col"] = m_programs["pos-col"]->attributeLocation("colAttr");
    m_uniforms["pos-col"] = m_programs["pos-col"]->uniformLocation("transform");
    m_uniforms["ranges"] = m_programs["pos-col"]->uniformLocation("ranges");
    m_uniforms["limits"] = m_programs["pos-col"]->uniformLocation("limits");

    loadData();
}

void Scatter3D::paintGL()
{
    const GLsizei retinaScale = devicePixelRatio();
    const qreal W = width() * retinaScale;
    const qreal H = height() * retinaScale;
    glViewport(0, 0, static_cast<GLsizei>(W), static_cast<GLsizei>(H));

    glClear(GL_COLOR_BUFFER_BIT);

    // TODO drawing axis labels (why not the full axes as images? it could be possible)s
    // Bind program that uses textures
    // Bind axis quads for labels
    // Draw quads with labels

    m_programs["pos-col"]->bind();

    QMatrix4x4 transform;
    transform.scale(static_cast<float>(H / W), 1.0f, 1.0f);
    transform.translate(m_translation.x(), m_translation.y());
    transform = transform * m_rotation;  // .rotate(m_rotation);// .x(), 0.0, 1.0, 0.0); // FIXME use better rotation see this: http://doc.qt.io/qt-5/qtopengl-cube-example.html
//    transform.rotate(m_rotation.y(), 1.0, 0.0, 0.0); // FIXME use better rotation see this: http://doc.qt.io/qt-5/qtopengl-cube-example.html
    m_programs["pos-col"]->setUniformValue(m_uniforms["pos-col"], transform);
    m_programs["pos-col"]->setUniformValue(m_uniforms["ranges"], m_ranges.transposed());
    m_programs["pos-col"]->setUniformValue(m_uniforms["limits"], m_limits.transposed());

    m_vaos["data"]->bind();

    // Draw the axis lines
    m_vbos["axis"]->bind();

    // Bind vertex values to position and color attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Draw axis as lines
    glDrawArrays(GL_LINES, 0, 6);

    // Draw the points
    m_vbos["points"]->bind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_POINTS, 0, m_nVertices);

    m_programs["pos-col"]->release();

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "GOT GL ERROR " << err << std::endl;
    }
}

void Scatter3D::loadData() {
    makeCurrent();
    m_vaos["data"]->bind();

    m_nVertices = 0;
    std::vector<float> vertices;
    for (int i = 0; i < m_entities.size(); i++) {
        for (int j = 0; j < m_entities[i].points.size(); j++) {
            m_nVertices++;
            vertices.push_back(m_entities[i].points[j].x());
            vertices.push_back(m_entities[i].points[j].y());
            vertices.push_back(m_entities[i].points[j].z());
            vertices.push_back(static_cast<float>(m_entities[i].color.redF()));
            vertices.push_back(static_cast<float>(m_entities[i].color.greenF()));
            vertices.push_back(static_cast<float>(m_entities[i].color.blueF()));
        }
    }

    // Allocate new memory, not great when just updating FIXME

    m_vbos["points"]->bind();
    m_vbos["points"]->allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
}

void Scatter3D::keyPressEvent(QKeyEvent *event)
{
    std::cout << "Key pressed" << std::endl;
    switch (event->key()) {
    case Qt::Key_Left: m_rotation.rotate(-10, 0, 1); break;
    case Qt::Key_Right: m_rotation.rotate(10, 0, 1); break;
    case Qt::Key_Up: {
        QMatrix4x4 r;
        r.rotate(-10, 1, 0);
        m_rotation = r * m_rotation;
        break;
    }
    case Qt::Key_Down: {
        QMatrix4x4 r;
        r.rotate(10, 1, 0);
        m_rotation = r * m_rotation;
        break;
    }
    case Qt::Key_W: m_translation += QVector2D( 0.0f,  0.1f); break;
    case Qt::Key_A: m_translation += QVector2D(-0.1f,  0.0f); break;
    case Qt::Key_S: m_translation += QVector2D( 0.0f, -0.1f); break;
    case Qt::Key_D: m_translation += QVector2D( 0.1f,  0.0f); break;
    case Qt::Key_0: m_translation = QVector2D(); m_rotation.setToIdentity(); break;
    default:
        QWidget::keyReleaseEvent(event);
        return;
    }
    update();
}

void Scatter3D::mousePressEvent(QMouseEvent *e)
{
    // Save mouse press position
    m_mousePressPos = QVector2D(e->localPos());
//    m_timer.stop();
}

void Scatter3D::mouseMoveEvent(QMouseEvent *e)
{
    QVector2D curPos = QVector2D(e->localPos());

    // Distance of cursor, relative to window size
    QVector2D diff = curPos - m_mousePressPos;
    QVector2D relDist = diff / QVector2D(width(), height());

    // Update mouse position
    m_mousePressPos = curPos;

    // Rotate
    QMatrix4x4 rx;
    QMatrix4x4 ry;
    rx.rotate(360 * relDist.x(), 0, 1);
    ry.rotate(360 * relDist.y(), 1, 0);
    m_rotation = ry * m_rotation * rx;

    update();
}

void Scatter3D::wheelEvent(QWheelEvent *event)
{
}

