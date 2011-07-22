#include "qextserialenumerator.h"
#include <QDebug>
#include <QMetaType>
#include <QStringList>
#include <QDir>
#include <QProcess>

QextSerialEnumerator::QextSerialEnumerator( )
{
    if( !QMetaType::isRegistered( QMetaType::type("QextPortInfo") ) )
        qRegisterMetaType<QextPortInfo>("QextPortInfo");
}

QextSerialEnumerator::~QextSerialEnumerator( )
{
}

QList<QextPortInfo> QextSerialEnumerator::getPorts()
{
    QList<QextPortInfo> infoList;
#ifdef Q_OS_LINUX
    // check which serial devices are hardware ports (they have
    // an entry in the system log)
    QStringList args;
    args << "-c" << "dmesg | grep 'ttyS[0-9]\\+'";

    QProcess process;
    process.start("sh", args, QIODevice::ReadOnly);
    process.waitForFinished(3000);
    QString syslogOutput = process.readAllStandardOutput();

    QStringList portNamePrefixes, portNameList;
    QDir dir("/dev");

    // get the non standard serial ports names first
    // (USB-serial, bluetooth-serial, 18F PICs, and so on)
    // if you know an other name prefix for serial ports please let us know
    portNamePrefixes << "ttyACM*" << "ttyUSB*" << "rfcomm*";
    portNameList = dir.entryList(portNamePrefixes, (QDir::System | QDir::Files), QDir::Name);

    // now get the standard serial ports
    portNamePrefixes.clear();
    portNamePrefixes << "ttyS*";

    foreach(QString path, dir.entryList(portNamePrefixes, (QDir::System | QDir::Files), QDir::Name))
    {
        if( path.startsWith("ttyS") )
        {
            // ignore values which are not serial ports e.g.  /dev/ttysa
            bool ok;

            // remove the ttyS part, and check, if the other part is a number
            path.mid(4).toInt(&ok, 10);
            if( !ok )
                continue;

            // now check if there realy exists a hardware port
            if( !syslogOutput.contains(path) )
                continue;
        }

        portNameList.append(path);
    }

    foreach (QString str , portNameList) {
        QextPortInfo inf;
        inf.physName = "/dev/"+str;
        inf.portName = str;

        if (str.startsWith("ttyS")) {
            inf.friendName = "Serial port "+str.remove(0, 4);
        }
        else if (str.startsWith("ttyUSB")) {
            inf.friendName = "USB-serial adapter "+str.remove(0, 6);
        }
        else if (str.startsWith("ttyACM")) {
            inf.friendName = "Serial over USB "+str.remove(0, 6);
        }
        else if (str.startsWith("rfcomm")) {
            inf.friendName = "Bluetooth-serial adapter "+str.remove(0, 6);
        }
        inf.enumName = "/dev"; // is there a more helpful name for this?
        infoList.append(inf);
    }
#else
    qCritical("Enumeration for POSIX systems (except Linux) is not implemented yet.");
#endif
    return infoList;
}

void QextSerialEnumerator::setUpNotifications( )
{
    qCritical("Notifications for *Nix/FreeBSD are not implemented yet");
}
