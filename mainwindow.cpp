#include "mainwindow.h"

#include <QOpenGLWidget>


#include <random>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
//    m_view = new QMathGL(this);
    m_view = new Scatter3D(this);
    setCentralWidget(m_view);

    resize(400, 400);

    // Generate some random test points
    std::default_random_engine generator;
    std::normal_distribution<float> n1(-1.1f, 2.4f);
    std::normal_distribution<float> n2(2.5f, 1.5f);

    QVector<QVector3D> points1, points2;

    for (int i = 0; i < 10000; i++) {
        points1 << QVector3D(n1(generator), n1(generator), n1(generator));
        points2 << QVector3D(n2(generator), n2(generator), n2(generator));
    }

    QColor color1 = QColor(255, 255, 0);
    QColor color2 = QColor(0, 255, 0);
    m_view->addSeries(points1, color1);
    m_view->addSeries(points2, color2);
    m_view->computeLimits();
//    m_view->setRangeX(-0.2, 0);
}

MainWindow::~MainWindow()
{

}
