#ifndef PEMHTTPCLIENT_H
#define PEMHTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QUuid>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QLoggingCategory>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(pemHttpClient)

// 数据模型类定义
class HttpResponse {
public:
    int code;
    QString msg;
    QJsonObject data;
    
    static HttpResponse fromJsonObject(const QJsonObject &obj) {
        HttpResponse response;
        response.code = obj["code"].toInt();
        response.msg = obj["msg"].toString();
        response.data = obj["data"].toObject();
        return response;
    }
};

class QrCode {
public:
    QString qrCodeUrl;
    QString qrCodeType;
    bool isBind;
    
    static QrCode fromJsonObject(const QJsonObject &obj) {
        QrCode qr;
        qr.qrCodeUrl = obj["qrCodeUrl"].toString();
        qr.qrCodeType = obj["qrCodeType"].toString();
        qr.isBind = obj["isBind"].toBool();
        return qr;
    }
};

class DeviceInfo {
public:
    QString deviceId;
    QString name;
    QString qrCodeUrl;
    bool isBind;
    QJsonArray skuCustomPrice;
    QJsonObject userInfo;
    QJsonArray gameList;
    QJsonArray gameCategoryList;
    QJsonValue tenantId;
    int vol;
    QJsonValue useType;
    bool canRefund;
    
    static DeviceInfo fromJsonObject(const QJsonObject &obj) {
        DeviceInfo device;
        device.deviceId = obj["deviceId"].toString();
        device.name = obj["name"].toString();
        device.qrCodeUrl = obj["qrCodeUrl"].toString();
        device.isBind = obj["isBind"].toBool();
        device.skuCustomPrice = obj["skuCustomPrice"].toArray();
        device.userInfo = obj["userInfo"].toObject();
        device.gameList = obj["gameList"].toArray();
        device.gameCategoryList = obj["gameCategoryList"].toArray();
        device.tenantId = obj["tenantId"];
        device.vol = obj["vol"].toInt();
        device.useType = obj["useType"];
        device.canRefund = obj["canRefund"].toBool();
        return device;
    }
};

class FreeWindow {
public:
    QString deviceIp;
    QString deviceId;
    QString token;
    QJsonArray portGroupList;
    QString archivePath;
    QJsonValue cloudGameVo;
    QJsonValue portGroup;
    QString uuid;
    bool sslEnable;
    
    static FreeWindow fromJsonObject(const QJsonObject &obj) {
        FreeWindow window;
        window.deviceIp = obj["deviceIp"].toString();
        window.deviceId = obj["deviceId"].toString();
        window.token = obj["token"].toString();
        window.portGroupList = obj["portGroupList"].toArray();
        window.archivePath = obj["archivePath"].toString();
        window.cloudGameVo = obj["cloudGameVo"];
        window.portGroup = obj["portGroup"];
        window.uuid = obj["uuuid"].toString(); // 注意：C#中是uuuid，但可能是uuid的拼写错误
        window.sslEnable = obj["sslEnable"].toBool();
        return window;
    }
};

class PemHttpClient : public QObject
{
    Q_OBJECT

public:
    explicit PemHttpClient(QObject *parent = nullptr);
    ~PemHttpClient();

    Q_INVOKABLE QString getClientCertificate() const;

    Q_INVOKABLE void sendGetRequest(const QString &path, QObject *callbackObject = nullptr, const QString &callbackMethod = QString());
    Q_INVOKABLE void sendPostRequest(const QString &path, const QString &jsonBody, QObject *callbackObject = nullptr, const QString &callbackMethod = QString());

    // 云游戏客户端相关接口
    Q_INVOKABLE void registerDevice(const QString &deviceCode, const QString &model = "win10", const QString &type = "5", QObject *callbackObject = nullptr, const QString &callbackMethod = QString());
    Q_INVOKABLE void getQrcode(const QString &deviceId, QObject *callbackObject = nullptr, const QString &callbackMethod = QString());
    Q_INVOKABLE void queryFreeWindows(const QString &gameId, const QString &userId, const QString &gpuId = "4060", QObject *callbackObject = nullptr, const QString &callbackMethod = QString());
//    Q_INVOKABLE void launch(QObject *callbackObject = nullptr, const QString &callbackMethod = QString());

    // 获取设备唯一标识
    Q_INVOKABLE QString getDeviceUuid() const;
    void setBaseUrl(const QString &url);

signals:
    void requestFinished(const QString &response);
    void requestError(const QString &error);

private:
    QString m_baseUrl;
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QSslCertificate m_clientCertificate;
    QSslKey m_privateKey;

    // 辅助方法
    QString generateDeviceUuid() const;
    QString encryptString(const QString &input, const QString &key) const;
    QString decryptString(const QString &input, const QString &key) const;
    QString readStoredUuid() const;
    void storeUuid(const QString &uuid) const;

    bool loadCertificateAndKey();
    QSslConfiguration createSslConfiguration();
};

#endif // PEMHTTPCLIENT_H