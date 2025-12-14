#ifndef FETCHER_H
#define FETCHER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

// 🔥 BẮT BUỘC cho Firebase
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QFile>

class fetcher : public QObject
{
    Q_OBJECT;
public:
    explicit fetcher(QObject *parent = nullptr);


    QJsonObject fetcher_get_user_info_by_rfid(const QString &rfid);
    QJsonObject fetcher_get_order_by_user_id(QString &userid);
    QJsonObject fetcher_get_payments_by_user_id(QString &userid);
    bool        fetcher_firebase_data(const QString &url,const QString  &output_file_path);
    void        fetcher_download_all_user_images(const QString &jsonPath, const QString &outputDir);
    void        fetcher_download_all_food_images(const QString &jsonPath, const QString &outputDir);

private:
    QJsonObject rootObject;
    bool loadLocalData(const QString &filePath);

signals:
};

#endif // FETCHER_H
