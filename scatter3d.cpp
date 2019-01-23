#include "scatter3d.h"

#include <iostream>
#include <QOpenGLBuffer>
#include <QPainter>
#include "GL/gl.h"

#include <QLabel>

// FIXME normalization is broken: what happens if points are all on the same plane?

// Using macros to concatenate strings
#define PROG_TEXT "texture-quad"
#define PROG_POINTS "pos-col"

void demoDraw(QImage &img, const QString &vert, const QString &hori) {
    QPainter lp(&img);

    lp.drawText(20, 20, "0,0");
    lp.drawText(120, 120, "100,100");
    lp.drawText(20, 120, "0,100");

    lp.drawText(128, 128, vert);
    lp.drawText(150, 150, hori);
}

void showImage(const QImage &img) {
    QLabel *magic = new QLabel();
    magic->setPixmap(QPixmap::fromImage(img));
    magic->show();
}

Scatter3D::Scatter3D(QWidget *parent):
    QOpenGLWidget(parent),
    m_nVertices(0)
{
    setFocusPolicy(Qt::StrongFocus);
    m_axisLength = 1.0f;

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    m_axisColor = QColor(255, 255, 255);

    m_axisImageXY = QImage(512, 512, QImage::Format_ARGB32);
    m_axisImageXZ = QImage(512, 512, QImage::Format_ARGB32);
    m_axisImageYZ = QImage(512, 512, QImage::Format_ARGB32);

    m_axisImageXY.fill(Qt::white);
    m_axisImageXZ.fill(Qt::white);
    m_axisImageYZ.fill(Qt::white);

    demoDraw(m_axisImageXY, "X", "Y");
    demoDraw(m_axisImageXZ, "X", "Z");
    demoDraw(m_axisImageYZ, "Y", "Z");

    // Prepare axis texture
    m_axisLabelImage = QImage(1024, 1024, QImage::Format_ARGB32);
    QPainter p(&m_axisLabelImage);
    p.drawImage(0, 0, m_axisImageXY);
    p.drawImage(0, 512, m_axisImageXZ);
    p.drawImage(512, 0, m_axisImageYZ);

    showImage(m_axisLabelImage);

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
    }

    m_limits = m_entities[0].maxmin();
    for (int i = 1; i < m_entities.size(); i++) {
        auto el = m_entities[i].maxmin();
        for (int j = 0; j < 3; j++) {
            m_limits(j, 0) = qMin(m_limits(j, 0), el(j, 0));
            m_limits(j, 1) = qMax(m_limits(j, 1), el(j, 1));
        }
    }
}

GLuint uintCheck(int v) {
    if (v < 0)
        throw "Error!";
    return static_cast<GLuint>(v);
}

void Scatter3D::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_LINE_SMOOTH);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(static_cast<float>(m_bgColor.redF()),
                 static_cast<float>(m_bgColor.greenF()),
                 static_cast<float>(m_bgColor.blueF()),
                 0.0f);

    // Cull back faces
    glCullFace(GL_BACK);


    glPointSize(5.0);
//    glLineWidth(10);

    float axRed = static_cast<float>(m_axisColor.redF());
    float axGreen = static_cast<float>(m_axisColor.redF());
    float axBlue = static_cast<float>(m_axisColor.redF());

//    std::vector<float> cubeCoords = {
//        -1, -1, -1,
//         1, -1, -1,
//         1,  1, -1,
//        -1,  1, -1,
//        -1, -1,  1,
//         1, -1,  1,
//         1,  1,  1,
//        -1,  1,  1,
//    };
//    std::vector<float> cubeIndices = {
//        6, 5, 2, 1, 3, 0, 7, 4,
//        2, 3, 6, 7, 5, 4, 1, 0,
//    };

    float l = m_axisLength;
    std::vector<float> cubeVerts = {
        // Strip
        -l, -l, -l, 0.0f, 0.0f, // 0
        -l, -l, +l, 0.0f, 0.0f, // 0
        -l, +l, -l, 0.0f, 0.0f, // 0
        -l, +l, +l, 0.0f, 0.0f, // 0

        -l, +l, -l, 0.0f, 0.0f, // 0
        -l, +l, +l, 0.0f, 0.0f, // 0
        +l, +l, -l, 0.0f, 0.0f, // 0
        +l, +l, +l, 0.0f, 0.0f, // 0

        +l, +l, -l, 0.0f, 0.0f, // 0
        +l, +l, +l, 0.0f, 0.0f, // 0
        +l, -l, -l, 0.0f, 0.0f, // 0
        +l, -l, +l, 0.0f, 0.0f, // 0

        +l, -l, -l, 0.0f, 0.0f, // 0
        +l, -l, +l, 0.0f, 0.0f, // 0
        -l, -l, -l, 0.0f, 0.0f, // 0
        -l, -l, +l, 0.0f, 0.0f, // 0

        +l, +l, -l, 0.0f, 0.0f, // 0
        +l, -l, -l, 0.0f, 0.0f, // 0
        -l, +l, -l, 0.0f, 0.0f, // 0
        -l, -l, -l, 0.0f, 0.0f, // 0

        -l, -l, +l, 0.0f, 0.0f, // 0
        +l, -l, +l, 0.0f, 0.0f, // 0
        -l, +l, +l, 0.0f, 0.0f, // 0
        +l, +l, +l, 0.0f, 0.0f // 0
    };

    m_vaos["axis"] = new QOpenGLVertexArrayObject(this);
    m_vaos["axis"]->create();
    m_vaos["axis"]->bind();

    m_vbos["cube"] = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbos["cube"]->create();
    m_vbos["cube"]->bind();
    m_vbos["cube"]->allocate(cubeVerts.data(), static_cast<int>(sizeof(float) * cubeVerts.size()));
    m_vbos["cube"]->release();

//    m_vbos["cube/indices"] = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
//    m_vbos["cube/indices"]->create();
//    m_vbos["cube/indices"]->bind();
//    m_vbos["cube/indices"]->allocate(cubeCoords.data(), static_cast<int>(sizeof(float) * cubeCoords.size()));
//    m_vbos["cube/indices"]->release();

    m_vaos["axis"]->release();

    m_vaos["data"] = new QOpenGLVertexArrayObject(this);
    m_vaos["data"]->create();
    m_vaos["data"]->bind();

    // Create buffers to store axis
    m_vbos["axis"] = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbos["axis"]->create();
    m_vbos["axis"]->bind();

//    float l = m_axisLength;
    std::vector<float> axisVert {
        -l, -l, -l, axRed, axGreen, axBlue,
         l, -l, -l, axRed, axGreen, axBlue,
        -l, -l, -l, axRed, axGreen, axBlue,
        -l,  l, -l, axRed, axGreen, axBlue,
        -l, -l, -l, axRed, axGreen, axBlue,
        -l, -l,  l, axRed, axGreen, axBlue
    };

    m_vbos["axis"]->allocate(axisVert.data(), static_cast<int>(sizeof(float) * axisVert.size()));
    m_vbos["axis"]->release();

    m_vbos["points"] = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbos["points"]->create();

    m_vbos["curves"] = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbos["curves"]->create();

    m_textures["axis"] = new QOpenGLTexture(m_axisLabelImage.mirrored());
    m_textures["axis"]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_textures["axis"]->setMagnificationFilter(QOpenGLTexture::Linear);

    // Data program
    static const char *vertexShaderSource =
        "#version 430 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aCol;\n"
        "out vec3 outCol;\n"
        "uniform mat4 transform = mat4(1.0);\n"
        "uniform mat3x3 ranges = mat3x3(0);\n"
        "uniform mat3x3 limits = mat3x3(-1, 1, 0, -1, 1, 0, -1, 1, 0);\n" // Column-major
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
        "out vec4 fragCol;\n"
        "void main() {\n"
        "   fragCol = vec4(outCol, 1.0f);\n"
        "   fragCol = mix(fragCol, vec4(0.1f), gl_FragCoord.z);\n"
        "}\n";

    m_programs[PROG_POINTS] = new QOpenGLShaderProgram(this);
    if (!m_programs[PROG_POINTS]->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        std::cout << "Compilation error!" << std::endl;
        std::cout << m_programs[PROG_POINTS]->log().toStdString() << std::endl;
        exit(0);
    }
    if (!m_programs[PROG_POINTS]->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        std::cout << "Compilation error!" << std::endl;
        std::cout << m_programs[PROG_POINTS]->log().toStdString() << std::endl;
        exit(0);
    }
    if (!m_programs[PROG_POINTS]->link()) {
        std::cout << "Linking error!" << std::endl;
        std::cout << m_programs[PROG_POINTS]->log().toStdString() << std::endl;
        exit(0);
    }

    m_attribs[PROG_POINTS"/pos"] = uintCheck(m_programs[PROG_POINTS]->attributeLocation("aPos"));
    m_attribs[PROG_POINTS"/col"] = uintCheck(m_programs[PROG_POINTS]->attributeLocation("aCol"));
    m_uniforms[PROG_POINTS"/transform"] = m_programs[PROG_POINTS]->uniformLocation("transform");
    m_uniforms[PROG_POINTS"/ranges"] = m_programs[PROG_POINTS]->uniformLocation("ranges");
    m_uniforms[PROG_POINTS"/limits"] = m_programs[PROG_POINTS]->uniformLocation("limits");

    // Texture programs
    static const char *textureVertexShaderSource =
        "#version 430 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 vertTexCoord;\n"
        "out vec2 fragTexCoord;\n"
        "uniform mat4 transform = mat4(1.0);\n"
        "uniform mat3x3 ranges = mat3x3(0);\n"
        "uniform mat3x3 limits = mat3x3(-1, 1, 0, -1, 1, 0, -1, 1, 0);\n" // Column-major
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
        "    fragTexCoord = vertTexCoord;\n"
        "}\n";


    static const char *textureFragShaderSource =
        "#version 430 core\n"
        "in vec2 fragTexCoord;\n"
        "out vec3 fragCol;\n"
        "uniform sampler2D texSampler;\n"
        "void main() {\n"
//        "    fragCol = mix(max(texture(texSampler, fragTexCoord).rgb, vec3(0.2f, 0.1f, 0.1f)), vec3(0.1f), gl_FragCoord.z);\n"
        "    fragCol = texture(texSampler, fragTexCoord).rgb;\n"
        "}\n";

    m_programs[PROG_TEXT] = new QOpenGLShaderProgram(this);
    if (!m_programs[PROG_TEXT]->addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexShaderSource)) {
        std::cout << "Compilation error!" << std::endl;
        std::cout << m_programs[PROG_TEXT]->log().toStdString() << std::endl;
        exit(0);
    }
    if (!m_programs[PROG_TEXT]->addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragShaderSource)) {
        std::cout << "Compilation error!" << std::endl;
        std::cout << m_programs[PROG_TEXT]->log().toStdString() << std::endl;
        exit(0);
    }
    if (!m_programs[PROG_TEXT]->link()) {
        std::cout << "Linking error!" << std::endl;
        std::cout << m_programs[PROG_TEXT]->log().toStdString() << std::endl;
        exit(0);
    }

    m_attribs[PROG_TEXT"/pos"] = uintCheck(m_programs[PROG_TEXT]->attributeLocation("aPos"));
    m_attribs[PROG_TEXT"/txt"] = uintCheck(m_programs[PROG_TEXT]->attributeLocation("vertTexCoord"));
    m_uniforms[PROG_TEXT"/transform"] = m_programs[PROG_TEXT]->uniformLocation("transform");
    m_uniforms[PROG_TEXT"/ranges"] = m_programs[PROG_TEXT]->uniformLocation("ranges");
    m_uniforms[PROG_TEXT"/limits"] = m_programs[PROG_TEXT]->uniformLocation("limits");


    loadData();
}

void Scatter3D::paintGL()
{
    const GLsizei retinaScale = devicePixelRatio();
    const qreal W = width() * retinaScale;
    const qreal H = height() * retinaScale;

    const float axisRanges[] = { 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f};
    const float axisLimits[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f}; // Row-major
    const QMatrix3x3 axisRangesMtx(axisRanges);
    const QMatrix3x3 axisLimitsMtx(axisLimits);

    glViewport(0, 0, static_cast<GLsizei>(W), static_cast<GLsizei>(H));

    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1);

    // Set transformation matrix: it will affect axis and data with translation and rotation
    QMatrix4x4 transform, scale;
    scale.scale(static_cast<float>(H / W), 1.0f, 1.0f); // Fit inside viewport
    transform.translate(m_translation.x(), m_translation.y()); // Move objects around
    transform = transform * m_rotation;
    transform = scale * transform;

    // Draw axis cube
    if (true) {
        // Cull faces to show only back faces
        glEnable(GL_CULL_FACE);

        m_programs[PROG_TEXT]->bind(); // TODO use PROG_TEXT later

        m_programs[PROG_TEXT]->setUniformValue(m_uniforms[PROG_TEXT"/transform"], transform);
        m_programs[PROG_TEXT]->setUniformValue(m_uniforms[PROG_TEXT"/ranges"], axisRangesMtx);
        m_programs[PROG_TEXT]->setUniformValue(m_uniforms[PROG_TEXT"/limits"], axisLimitsMtx);

        // Use texture unit 0 which contains cube.png
        glActiveTexture(GL_TEXTURE0);
        m_textures["axis"]->bind();
        m_programs[PROG_TEXT]->setUniformValue("texSampler", 0);

        m_vaos["axis"]->bind();
        m_vbos["cube"]->bind();
//        m_vbos["cube/indices"]->bind();

        // Bind vertex values to position and color attributes
        glVertexAttribPointer(m_attribs[PROG_TEXT"/pos"], 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        glEnableVertexAttribArray(m_attribs[PROG_TEXT"/pos"]);
        glVertexAttribPointer(m_attribs[PROG_TEXT"/txt"], 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(m_attribs[PROG_TEXT"/txt"]);

        // Draw cube as lines
        for (int i = 0; i < 6; i++)
            glDrawArrays(GL_TRIANGLE_STRIP, i*4, 4);

        // Done drawing axes
        m_vaos["axis"]->release();
        m_programs[PROG_TEXT]->release(); // TODO use PROG_TEXT later

        glDisable(GL_CULL_FACE);
    }

    // Program to display axis and data
    m_programs[PROG_POINTS]->bind();

    // VAO with data and tags
    m_vaos["data"]->bind();

    // Draw axis
    if (true) {
        m_programs[PROG_POINTS]->setUniformValue(m_uniforms[PROG_POINTS"/transform"], transform);
        m_programs[PROG_POINTS]->setUniformValue(m_uniforms[PROG_POINTS"/ranges"], axisRangesMtx);
        m_programs[PROG_POINTS]->setUniformValue(m_uniforms[PROG_POINTS"/limits"], axisLimitsMtx);

        // Draw the axis lines
        m_vbos["axis"]->bind();
        // Bind vertex values to position and color attributes
        glVertexAttribPointer(m_attribs[PROG_POINTS"/pos"], 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glEnableVertexAttribArray(m_attribs[PROG_POINTS"/pos"]);
        glVertexAttribPointer(m_attribs[PROG_POINTS"/col"], 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(m_attribs[PROG_POINTS"/col"]);
        // Draw axis as lines
        glDrawArrays(GL_LINES, 0, 6);
        // Done drawing axes
        m_vbos["axis"]->release();
    }

    // Draw points
    if (true) {
        m_programs[PROG_POINTS]->setUniformValue(m_uniforms[PROG_POINTS"/transform"], transform);
        m_programs[PROG_POINTS]->setUniformValue(m_uniforms[PROG_POINTS"/ranges"], m_ranges.transposed());
        m_programs[PROG_POINTS]->setUniformValue(m_uniforms[PROG_POINTS"/limits"], m_limits.transposed());

        // Draw the points
        m_vbos["points"]->bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_POINTS, 0, m_nVertices);
        m_vbos["points"]->release();
    }

    // Done drawing things
    m_programs[PROG_POINTS]->release();

    m_programs[PROG_TEXT]->bind();
    m_programs[PROG_TEXT]->release();

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
    rx.rotate(-360 * relDist.x(), 0, 1);
    ry.rotate(-360 * relDist.y(), 1, 0);
    m_rotation = ry * m_rotation * rx;

    update();
}

void Scatter3D::wheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event);
}

