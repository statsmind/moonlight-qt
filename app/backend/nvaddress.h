#pragma once

#include <QHostAddress>
#include <QMap>

#define DEFAULT_HTTP_PORT 47989
#define DEFAULT_HTTPS_PORT 47984

class NvAddress
{
public:
    NvAddress();
    explicit NvAddress(QString addr, uint16_t port);
    explicit NvAddress(QHostAddress addr, uint16_t port);

    uint16_t port() const;
    void setPort(uint16_t port);

    QString address() const;
    void setAddress(QString addr);
    void setAddress(QHostAddress addr);

    bool isNull() const;
    QString toString() const;

    bool operator==(const NvAddress& other) const
    {
        return m_Address == other.m_Address &&
                m_Port == other.m_Port;
    }

    bool operator!=(const NvAddress& other) const
    {
        return !operator==(other);
    }

private:
    QString m_Address;
    uint16_t m_Port;
};

class NvProxyAddress : public NvAddress
{
public:
    NvProxyAddress(QJsonArray portGroupList);
    explicit NvProxyAddress(QString addr, uint16_t port, QJsonArray portGroupList);
    explicit NvProxyAddress(QHostAddress addr, uint16_t port, QJsonArray portGroupList);

    void setPortMapping(QJsonArray portGroupList);

private:
    QMap<int, int>* m_PortMapping;
};
