#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <qdebug.h>
#include "shengtutest.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    puTuLogic = new shengpubutu(this);
    puTuLogic->setGraphicsView(ui->graphicsView);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open XTF File", "", "XTF Files (*.xtf);;All Files (*)");
    if (fileName.isEmpty()) return;

    shengtutest reader(this);
    // 修改 parseXtfHeader，使其接收 shengpubutu 的指针，用于显示
    QVector<std::vector<uint8_t>> portData;
    QVector<std::vector<uint8_t>> starboardData;

    reader.parseXtfHeader(fileName, portData, starboardData);
    // for(int i=2000; i<portData[0].size();++i){
    //     qDebug()<<"portData value"<< i<<": " <<portData[0][i];
    // }
    // for(int i=0; i<starboardData[0].size()*0.4;++i){
    //     qDebug()<<"starboardData value"<< i<<": " <<starboardData[0][i];
    // }

    puTuLogic->createPingImages(portData, starboardData);
}

void MainWindow::on_bottomTrackButton_clicked()
{
    if (puTuLogic)
        puTuLogic->doBottomTrack();
}

void MainWindow::on_Imagefusion_clicked()
{
    if (puTuLogic)
        puTuLogic->showMergedImage();
}
