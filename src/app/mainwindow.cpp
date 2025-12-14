#include "mainwindow.h"

#include <QStandardItem>

#include "./ui_mainwindow.h"
#include "usb_tool.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    InitModel();
    UsbTool tool;
    tool.Search();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::InitModel() {
    ui->treeView->setHeaderHidden(true);
    model = new QStandardItemModel(this);

    QStandardItem* usb_item = new QStandardItem();
    usb_item->setText("USB设备");
    model->appendRow(usb_item);

    QStandardItem* network_item = new QStandardItem();
    network_item->setText("网络适配器");
    model->appendRow(network_item);

    ui->treeView->setModel(model);
}
