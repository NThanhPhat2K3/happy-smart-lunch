#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->scanner_device_rfid = new scanner_device("/dev/hidraw0", DEVICE_RFID, this);
    this->scanner_device_barcode = new scanner_device("/dev/hidraw1", DEVICE_BARCODE, this);
    scanner_device_rfid->scanner_open();
    scanner_device_barcode->scanner_open();
}

MainWindow::~MainWindow()
{
    delete ui;
    this->scanner_device_rfid->scanner_close();
    this->scanner_device_barcode->scanner_close();
}
