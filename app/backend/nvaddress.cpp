#include "nvaddress.h"

#include <QHostAddress>
#include <QJsonObject>
#include <QJsonArray>

NvAddress::NvAddress()
{
    setAddress(nullptr);
    setPort(0);
}

NvAddress::NvAddress(QString addr, uint16_t port)
{
    setAddress(addr);
    setPort(port);
}

NvAddress::NvAddress(QHostAddress addr, uint16_t port)
{
    setAddress(addr);
    setPort(port);
}

uint16_t NvAddress::port() const
{
    return m_Port;
}

QString NvAddress::address() const
{
    return m_Address;
}

void NvAddress::setPort(uint16_t port)
{
    m_Port = port;
}

void NvAddress::setAddress(QString addr)
{
    m_Address = addr;
}

void NvAddress::setAddress(QHostAddress addr)
{
    m_Address = addr.toString();
}

bool NvAddress::isNull() const
{
    return m_Address.isEmpty();
}

QString NvAddress::toString() const
{
    if (m_Address.isEmpty()) {
        return "<NULL>";
    }

    if (QHostAddress(m_Address).protocol() == QAbstractSocket::IPv6Protocol) {
        return QString("[%1]:%2").arg(m_Address).arg(m_Port);
    }
    else {
        return QString("%1:%2").arg(m_Address).arg(m_Port);
    }
}

NvProxyAddress::NvProxyAddress(QJsonArray portGroupList)
        : NvAddress()
{
    m_PortMapping = new QMap<int, int>();
    setPortMapping(portGroupList);
}

NvProxyAddress::NvProxyAddress(QString addr, uint16_t port, QJsonArray portGroupList)
    : NvAddress(QHostAddress(addr), port)
{
    m_PortMapping = new QMap<int, int>();
    setPortMapping(portGroupList);
}

NvProxyAddress::NvProxyAddress(QHostAddress addr, uint16_t port, QJsonArray portGroupList)
    : NvAddress(addr, port)
{
    m_PortMapping = new QMap<int, int>();
    setPortMapping(portGroupList);
}

void NvProxyAddress::setPortMapping(QJsonArray portGroupList) {
    for (int i = 0; i < portGroupList.size(); i++) {
        QJsonObject portGroup = portGroupList[i].toObject();
        int forwardPort = portGroup["forwardPort"].toInt();
        int internalPort = portGroup["internalPort"].toInt();

        m_PortMapping->insert(internalPort, forwardPort);
    }
}