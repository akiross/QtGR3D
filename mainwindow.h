#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

//#include <mgl2/qmathgl.h>
#include <scatter3d.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
//    QMathGL *m_view;
    Scatter3D *m_view;
};

#endif // MAINWINDOW_H
