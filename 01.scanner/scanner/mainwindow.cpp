#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPixmap>
#include <QDate>
#include <QFile>
#include <QFontDatabase>
#include <QDateTime>
#include <QTimer>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

struct UserInfo {
    QString user;
    QString food;
    QString url_image;
};

QVector<UserInfo> userHistory(4);  // Lưu danh sách tối đa 4 người đã quét gần nhất

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- Init scanner ---
    this->scanner_device_rfid = new scanner_device("/dev/hidraw0", DEVICE_RFID, this);
    this->scanner_device_barcode = new scanner_device("/dev/hidraw1", DEVICE_BARCODE, this);
    scanner_device_rfid->scanner_open();
    scanner_device_barcode->scanner_open();

    // --- Fetch Firebase lần đầu ---
    this->fetcher_firebase.fetcher_firebase_data("https://datn-e35a5-default-rtdb.firebaseio.com/.json", "data.json");
    this->fetcher_firebase.fetcher_download_all_user_images("data.json", ".");
    this->fetcher_firebase.fetcher_download_all_food_images("data.json", ".");

    // --- Connect scanner ---
    connect(scanner_device_rfid, &scanner_device::scanner_received_data,
            this, &MainWindow::handle_scanner_data);
    connect(scanner_device_barcode, &scanner_device::scanner_received_data,
            this, &MainWindow::handle_scanner_data);

    // --- Font + style cho datetime ---
    QFont font("Segoe UI", 20, QFont::Bold);
    ui->date_time->setFont(font);
    ui->date_time->setStyleSheet(
        "QLabel { "
        "color: #212121;"
        "background-color: transparent;"
        "padding: 6px;"
        "border-radius: 6px;"
        "}"
        );

    // --- Timer update giờ ---
    QTimer *timerClock = new QTimer(this);
    connect(timerClock, &QTimer::timeout, this, &MainWindow::updateDateTime);
    timerClock->start(1000);
    updateDateTime();

    // --- Timer auto fetch từ Firebase (5 giây 1 lần) ---
    QTimer *timerFetch = new QTimer(this);
    connect(timerFetch, &QTimer::timeout, this, &MainWindow::refreshFirebaseData);
    timerFetch->start(5000); // mỗi 5 giây refetch
}

MainWindow::~MainWindow()
{
    delete ui;
    this->scanner_device_rfid->scanner_close();
    this->scanner_device_barcode->scanner_close();
}

// ===================== REFRESH FIREBASE DATA =====================
    void MainWindow::refreshFirebaseData()
{
    qDebug() << "🔄 Refetching Firebase data...";
    if (!fetcher_firebase.fetcher_firebase_data(
            "https://datn-e35a5-default-rtdb.firebaseio.com/.json",
            "data.json")) {
        qWarning() << "❌ Fetch firebase failed";
    }

    this->fetcher_firebase.fetcher_download_all_user_images("data.json", ".");
    this->fetcher_firebase.fetcher_download_all_food_images("data.json", ".");

    // TODO: parse lại data.json -> nếu có user mới thì cập nhật UI
    // ví dụ:
    // QJsonObject latestUser = fetcher_firebase.fetcher_get_latest_user();
    // nếu có thì updateUserDisplay();
}

// ===================== STYLE CHỮ =====================
void MainWindow::applyCustomStyle(QLabel *label, QString color, int fontSize)
{
    QFont customFont("Roboto", fontSize, QFont::Bold);
    customFont.setStyleStrategy(QFont::PreferAntialias);

    label->setFont(customFont);
    label->setStyleSheet(QString("color: %1;").arg(color));

    auto *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(6);
    shadowEffect->setOffset(1, 1);
    shadowEffect->setColor(Qt::gray);
    label->setGraphicsEffect(shadowEffect);
}

// ===================== SCALE ẢNH =====================
void MainWindow::setImageToLabel(QLabel *label, const QString &path)
{
    if (!QFile::exists(path)) {
        qDebug() << "Ảnh không tồn tại:" << path;
        return;
    }

    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        qDebug() << "Không load được ảnh:" << path;
        return;
    }

    label->setPixmap(pixmap.scaled(
        label->size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
}

// ===================== ẢNH AVATAR TRÒN =====================
void MainWindow::setCircleImageToLabel(QLabel *label, const QString &path)
{
    if (!QFile::exists(path)) return;

    QPixmap src(path);
    if (src.isNull()) return;

    int size = qMin(label->width(), label->height());
    QPixmap dst(size, size);
    dst.fill(Qt::transparent);

    QPainter painter(&dst);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath pathCircle;
    pathCircle.addEllipse(0, 0, size, size);
    painter.setClipPath(pathCircle);

    painter.drawPixmap(0, 0, src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    label->setPixmap(dst);
    label->setAlignment(Qt::AlignCenter);
}

// ===================== HIỂN THỊ HISTORY =====================
void MainWindow::updateUserDisplay()
{
    QLabel* imageLabels[4] = { ui->image_name_1, ui->image_name_2, ui->image_name_3, ui->image_name_4 };
    QLabel* nameLabels[4]  = { ui->name_1, ui->name_2, ui->name_3, ui->name_4 };
    QLabel* foodLabels[4]  = { ui->food_1, ui->food_2, ui->food_3, ui->food_4 };

    QStringList colors = { "#2980b9", "#27ae60", "#7f8c8d", "#e67e22" };

    for (int i = 0; i < userHistory.size() && i < 4; ++i) {
        const UserInfo &usr = userHistory[i];

        int fontSize = (i == 0 ? 18 : 14);
        applyCustomStyle(nameLabels[i], colors[i % colors.size()], fontSize);
        applyCustomStyle(foodLabels[i], "#8e44ad", fontSize);

        nameLabels[i]->setText(usr.user);
        foodLabels[i]->setText(usr.food);

        if (!usr.url_image.isEmpty() && QFile::exists(usr.url_image)) {
            setCircleImageToLabel(imageLabels[i], usr.url_image);
        } else {
            imageLabels[i]->clear();
        }
    }
}

// ===================== HANDLE SCANNER DATA =====================
void MainWindow::handle_scanner_data(QString data)
{
    qDebug() << "RFID nhận được:" << data;
    QString rfid = data;
    UserInfo newUser;

    // Style mặc định cho label chính
    applyCustomStyle(ui->name_1, "#2c3e50", 18);
    applyCustomStyle(ui->food_1, "#8e44ad", 18);

    // --- Lấy thông tin user theo RFID ---
    QJsonObject userInfo = fetcher_firebase.fetcher_get_user_info_by_rfid(rfid);
    if (!userInfo.isEmpty()) {
        QString userId = userInfo["userId"].toString();
        QString userName = userInfo["ten"].toString();

        // 🖼 Ảnh user
        QString userImgPath = "img_usr/" + rfid + ".jpg";
        if (QFile::exists(userImgPath)) {
            ui->name_1->setText(userName);
            setCircleImageToLabel(ui->image_name_1, userImgPath);
        } else {
            ui->name_1->setText(userName);
            QString unknownImgPath(":/new/prefix/image/unknown_user.png");
            setCircleImageToLabel(ui->image_name_1, unknownImgPath);
        }

        // 📅 Hôm nay
        QString today = QDate::currentDate().toString("yyyy-MM-dd");

        // --- Lấy món ăn hôm nay ---
        QJsonObject orderToday = fetcher_firebase.fetcher_get_order_by_user_id(userId);
        if (!orderToday.isEmpty()) {
            // ✅ Có món ăn
            QString foodName = orderToday["food"].toString();
            ui->food_1->setText(foodName);

            QString foodImgPath = "img_food/" + rfid + "_" + today + ".jpg";
            setImageToLabel(ui->image_food, foodImgPath);

            // Lưu history
            newUser.user = userName;
            newUser.food = foodName;
            newUser.url_image = userImgPath;

        } else {
            // ⚠️ User có nhưng chưa đặt món
            applyCustomStyle(ui->food_1, "#d35400", 18);  // chữ cam
            ui->food_1->setText("Forgot Order!!!");

            QString noFoodPath(":/new/prefix/image/forgot_order_food.jpg");
            if (QFile::exists(noFoodPath)) {
                QPixmap pix(noFoodPath);
                ui->image_food->setPixmap(pix.scaled(
                    ui->image_food->size(),
                    Qt::KeepAspectRatio,        // ✅ scale vừa khung, không bị tràn
                    Qt::SmoothTransformation)); // ✅ mượt
                ui->image_food->setAlignment(Qt::AlignCenter);
                ui->food_1->setText("Forgot Order!!!!");
            } else {
                ui->image_food->clear();
            }

            // Lưu history với "Chưa đặt món"
            newUser.user = userName;
            newUser.food = "Forgot Order!!!!";
            newUser.url_image = userImgPath;
        }

        // --- Check thanh toán ---
        QJsonObject paymentToday = fetcher_firebase.fetcher_get_payments_by_user_id(userId);
        if (!paymentToday.isEmpty()) {
            qDebug() << "Đã thanh toán:" << paymentToday["paid"].toBool();
        }

        // Shift lịch sử (đẩy xuống)
        for (int i = userHistory.size() - 1; i > 0; --i) {
            userHistory[i] = userHistory[i - 1];
        }
        userHistory[0] = newUser;

        updateUserDisplay();

    } else {
        // ❌ Không tìm thấy user
        qDebug() << "Không tìm thấy người dùng có RFID:" << rfid;

        applyCustomStyle(ui->name_1, "#c0392b", 18);  // đỏ
        applyCustomStyle(ui->food_1, "#c0392b", 18);

        ui->name_1->setText("Invalid User!!!!");
        ui->food_1->setText("Invalid Food!!!!");

        // Ảnh unknown user avatar tròn
        QString unknownImgPath(":/new/prefix/image/unknown_user.png");
        if (QFile::exists(unknownImgPath)) {
            setCircleImageToLabel(ui->image_name_1, unknownImgPath);
        } else {
            ui->image_name_1->clear();
        }

        // Ảnh invalid food scale khung
        QString invalidFoodPath(":/new/prefix/image/id_invalid_nn.jpg");
        if (QFile::exists(invalidFoodPath)) {
            setImageToLabel(ui->image_food, invalidFoodPath);
        } else {
            ui->image_food->clear();
        }
    }
}

// ===================== UPDATE DATETIME =====================
void MainWindow::updateDateTime()
{
    QString currentTime = QDateTime::currentDateTime()
    .toString("dddd, dd/MM/yyyy - HH:mm:ss");
    ui->date_time->setText(currentTime);
}
