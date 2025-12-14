#include "fetcher.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>

fetcher::fetcher(QObject *parent)
    : QObject{parent}
{}
    // struct UserInfo {
    //     QString user;
    //     QString food;
    //     QString url_image;
    // };
    // QVector<UserInfo> userHistory(4); // vector với 4 phần tử rỗng ban đầu

QJsonObject fetcher::fetcher_get_user_info_by_rfid(const QString &rfid)
{
    QFile file("data.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Không thể mở file data.json";
        return {};
    }

    QByteArray rawData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Lỗi khi parse JSON:" << parseError.errorString();
        return {};
    }

    QJsonObject root = doc.object();
    QJsonObject usersObject = root["users"].toObject();

    for (auto it = usersObject.constBegin(); it != usersObject.constEnd(); ++it) {
        QJsonObject userObject = it.value().toObject();
        QString userRfid = userObject.value("rfid").toString();

        if (userRfid == rfid) {
            QJsonObject matched = userObject;
            matched.insert("userId", it.key());
            // qDebug() << "Matched user:" << QJsonDocument(matched).toJson(QJsonDocument::Indented);
            return matched;
        }
    }

    qWarning() << "Không tìm thấy người dùng có rfid =" << rfid;
    return {};
}
QJsonObject fetcher::fetcher_get_order_by_user_id(QString &userid)
{
    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QJsonObject orders = rootObject["orders"].toObject().value(userid).toObject();

    if (orders.contains(currentDate)) {
        return orders.value(currentDate).toObject();
    }
    return {};
}

QJsonObject fetcher::fetcher_get_payments_by_user_id(QString &userid)
{
    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QJsonObject payments = rootObject["payments"].toObject().value(userid).toObject();

    if (payments.contains(currentDate)) {
        return payments.value(currentDate).toObject();
    }
    return {};
}
bool fetcher::fetcher_firebase_data(const QString &url, const QString &output_file_path)
{
    QNetworkAccessManager manager;
    QEventLoop loop;

    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error()) {
        qWarning() << "cannot fetch data from firebase " << reply->errorString();
        reply->deleteLater();
        return false;
    }

    QByteArray data = reply->readAll();
    QFile file(output_file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "cannot open file" << output_file_path;
        reply->deleteLater();
        return false;
    }
    file.write(data);
    file.flush();
    file.close();
    reply->deleteLater();

    return loadLocalData(output_file_path);
}
bool fetcher::loadLocalData(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "cannot open file JSON:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "data invalid";
        return false;
    }

    rootObject = doc.object();
    return true;
}

void fetcher::fetcher_download_all_user_images(const QString &jsonPath, const QString &outputDir)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Cannot open JSON file:" << jsonPath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "❌ Invalid JSON format!";
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject users = root["users"].toObject();

    QDir dir;
    QString saveDir = outputDir + "/img_usr";
    if (!dir.exists(saveDir)) {
        dir.mkpath(saveDir);
        qDebug() << "📁 Created directory:" << saveDir;
    }

    // Quản lý mạng thuộc về đối tượng fetcher
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    for (const QString &userId : users.keys()) {
        QJsonObject user = users[userId].toObject();
        QString rfid = user["rfid"].toString();
        QString url = user["url_image"].toString();

        if (rfid.isEmpty() || !url.startsWith("http"))
            continue;

        QString imagePath = saveDir + "/" + rfid + ".jpg";
        if (QFile::exists(imagePath)) {
            qDebug() << "✅ Ảnh đã tồn tại:" << imagePath;
            continue;
        }

        QUrl imageUrl(url);
        QNetworkRequest request(imageUrl);
        QNetworkReply *reply = manager->get(request);

        // Xử lý kết quả khi ảnh tải xong
        connect(reply, &QNetworkReply::finished, this, [reply, imagePath, rfid]() {
            reply->deleteLater();

            if (reply->error() == QNetworkReply::NoError) {
                QByteArray imageData = reply->readAll();
                QFile imgFile(imagePath);
                if (imgFile.open(QIODevice::WriteOnly)) {
                    imgFile.write(imageData);
                    imgFile.close();
                    qDebug() << "📥 Đã tải ảnh RFID:" << rfid << "->" << imagePath;
                } else {
                    qWarning() << "❌ Không thể lưu ảnh:" << imagePath;
                }
            } else {
                qWarning() << "❌ Lỗi tải ảnh từ:" << reply->url().toString() << "→" << reply->errorString();
            }
        });
    }
}
void fetcher::fetcher_download_all_food_images(const QString &jsonPath, const QString &outputDir)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Cannot open JSON file:" << jsonPath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "❌ Invalid JSON format!";
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject orders = root["orders"].toObject();
    QJsonObject users = root["users"].toObject();

    QDir dir;
    QString saveDir = outputDir + "/img_food";
    if (!dir.exists(saveDir)) {
        dir.mkpath(saveDir);
        qDebug() << "📁 Created directory:" << saveDir;
    }

    // Sử dụng QNetworkAccessManager bất đồng bộ
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    for (const QString &userId : orders.keys()) {
        if (!users.contains(userId)) continue;

        QString rfid = users[userId].toObject()["rfid"].toString();
        if (rfid.isEmpty()) continue;

        QJsonObject orderByDate = orders[userId].toObject();

        for (const QString &dateKey : orderByDate.keys()) {
            QJsonObject order = orderByDate[dateKey].toObject();
            QString foodUrl = order["url_food"].toString();

            if (!foodUrl.startsWith("http")) continue;

            QString imagePath = QString("%1/%2_%3.jpg").arg(saveDir, rfid, dateKey);
            if (QFile::exists(imagePath)) {
                qDebug() << "✅ Ảnh đã tồn tại:" << imagePath;
                continue;
            }

            QUrl imageUrl(foodUrl);
            QNetworkRequest request(imageUrl);
            QNetworkReply *reply = manager->get(request);

            // Bắt sự kiện khi ảnh tải xong
            connect(reply, &QNetworkReply::finished, this, [reply, imagePath, rfid, dateKey]() {
                reply->deleteLater();

                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray imageData = reply->readAll();
                    QFile imgFile(imagePath);
                    if (imgFile.open(QIODevice::WriteOnly)) {
                        imgFile.write(imageData);
                        imgFile.close();
                        qDebug() << "📥 Đã tải ảnh cho" << rfid << "ngày" << dateKey << "->" << imagePath;
                    } else {
                        qWarning() << "❌ Không thể ghi ảnh tại:" << imagePath;
                    }
                } else {
                    qWarning() << "❌ Tải ảnh thất bại từ:" << reply->url().toString()
                               << "->" << reply->errorString();
                }
            });
        }
    }
}

