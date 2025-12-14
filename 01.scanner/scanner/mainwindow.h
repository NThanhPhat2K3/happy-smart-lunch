#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "scanner_device.h"
#include "fetcher.h"
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    fetcher fetcher_firebase;
    scanner_device *scanner_device_rfid;
    scanner_device *scanner_device_barcode;

    void setImageToLabel(QLabel *label, const QString &path);
    void applyCustomStyle(QLabel *label, QString color , int fontSize);
    void setCircleImageToLabel(QLabel *label, const QString &path);
    void updateUserDisplay();

private slots:
    void handle_scanner_data(QString data);
    void updateDateTime();
    void refreshFirebaseData();

};
#endif // MAINWINDOW_H
