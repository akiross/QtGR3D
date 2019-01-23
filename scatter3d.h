#ifndef SCATTER3D_H
#define SCATTER3D_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QBasicTimer>


struct Entity {
    QColor color;
    QVector<QVector3D> points;
    bool line;

    // Computes max and min coordinates of this entity
    QMatrix3x3 maxmin() const {
        QVector3D min = points[0];
        QVector3D max = points[0];
        foreach (QVector3D pt, points) {
            if (pt.x() < min.x()) min.setX(pt.x());
            if (pt.y() < min.y()) min.setY(pt.y());
            if (pt.z() < min.z()) min.setZ(pt.z());
            if (pt.x() > max.x()) max.setX(pt.x());
            if (pt.y() > max.y()) max.setY(pt.y());
            if (pt.z() > max.z()) max.setZ(pt.z());
        }
        QMatrix3x3 m;
        m(0, 0) = min.x();
        m(0, 1) = max.x();
        m(1, 0) = min.y();
        m(1, 1) = max.y();
        m(2, 0) = min.z();
        m(2, 1) = max.z();
        return m;
    }
};


class Scatter3D : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core
{
public:
    Scatter3D(QWidget *parent=nullptr);
    ~Scatter3D();

    void addSeries(const QVector<QVector3D> &vertices, QColor color);
    void setRangeX(float min, float max);
    void setRangeY(float min, float max);
    void setRangeZ(float min, float max);
    void resetRanges();

//    void addClipPlane();

    // Updates the limits using the current series
    void computeLimits();
protected:
    void initializeGL();
    void paintGL();

    // Load data from series to buffers
    void loadData();

    void keyPressEvent(QKeyEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void wheelEvent(QWheelEvent *event);
private:
    QBasicTimer m_timer;

    QVector2D m_mousePressPos;
    QVector2D m_translation;

    float m_axisLength;

    QColor m_axisColor;
    QColor m_bgColor;

    QMatrix3x3 m_ranges; // The values that we want to see, use 0,0 to use limits
    QMatrix3x3 m_limits; // The min/max values of the current series (all of them!) on each axis

    // Entities to draw
    QVector<Entity> m_entities;

    QMap<const char *, QOpenGLVertexArrayObject *> m_vaos;
    QMap<const char *, QOpenGLBuffer *> m_vbos;
    QMap<const char *, QOpenGLTexture *> m_textures;
    QMap<const char *, QOpenGLShaderProgram *> m_programs;
    QMap<const char *, GLuint> m_attribs;
    QMap<const char *, GLint> m_uniforms;

//     *m_vboAxis, *m_vboPoints, *m_vboCurves;
//     *m_axisTexture;

//    QVector3D m_rotationAxis;
    float m_angularSpeed;
//    QQuaternion m_rotation;
    QMatrix4x4 m_rotation;

    QImage m_axisLabelImage;
    QImage m_axisImageXY;
    QImage m_axisImageXZ;
    QImage m_axisImageYZ;
    int m_nVertices;

//    GLint m_colAttr;
//    GLint m_matrixUniform;
};

#endif // SCATTER3D_H
