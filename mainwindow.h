#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "shengpubutu.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_openFileButton_clicked();
    void on_bottomTrackButton_clicked(); // 新增：底部追踪按钮槽

private:
    Ui::MainWindow *ui;
    shengpubutu *puTuLogic; // 用于逻辑处理

};

#endif // MAINWINDOW_H
