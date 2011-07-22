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

    // check which serial devices are hardware ports (they have
    // an entry in the system log)
    QStringList args;
    args << "-c" << "dmesg | grep 'ttyS[0-9]\\+'";

    QProcess process;
    process.start("sh", args, QIODevice::ReadOnly);
    process.waitForFinished(3000);
    QString syslogOutput = process.readAllStandardOutput();

    foreach( QString portname,
             dir.entryList(portNamePrefixes, (QDir::System | QDir::Files), QDir::Name) )
    {
        // ignore values which are not serial ports e.g.  /dev/ttysa
        bool ok;

        // remove the ttyS part, and check, if the other part is a number
        portname.mid(4).toInt(&ok, 10);
        if( !ok )
            continue;

        // now check if there realy exists a hardware port
        if( !syslogOutput.contains(portname) )
            continue;

        portNameList << portname;
    }

    foreach(QString portname, portNameList)
    {
        QextPortInfo inf;
        inf.portName = portname;
        inf.physName = "/dev/"+portname;
        inf.enumName = "/dev"; // is there a more helpful name for this?
        inf.vendorID = 0;
        inf.productID = 0;

        if( portname.startsWith("ttyS") )
        {
            inf.friendName = "Serial port "+portname.remove(0, 4);
        }
        else if( portname.startsWith("ttyUSB") )
        {
            // set fallbackup name
            inf.friendName = QString("USB-Serial adapter %1").arg(portname.mid(6));

            // now try to get more info about the adapter
            QFile uevent("/sys/class/tty/"+portname+"/device/uevent");
            if( uevent.open(QFile::ReadOnly) )
            {
                QString driverName;
                forever
                {
                    QString line = uevent.readLine();
                    if( line.isEmpty() )
                        break;

                    if( line.startsWith("DRIVER=") )
                    {
                        driverName = line.mid(7).simplified(); // cut the "DRIVER=" part
                        break;
                    }
                }
                uevent.close();

                QDir driverDir("/sys/bus/usb/drivers/"+driverName);
                driverDir.setNameFilters(QStringList("*:*"));
                foreach(QString subDir, driverDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
                {
                    QString driverSubDir = driverDir.path()+"/"+subDir;
                    if( !QFile::exists(driverSubDir+'/'+portname) )
                        continue;

                    QFile vidPidFile(driverSubDir+"/uevent");
                    if( vidPidFile.open(QFile::ReadOnly) )
                    {
                        forever
                        {
                            QString line = vidPidFile.readLine();
                            if( line.isEmpty() )
                                break;

                            if( line.startsWith("PRODUCT=") )
                            {
                                line = line.mid(8); // cut the "PRODUCT=" part

                                QString vid = line.section('/',0,0);
                                bool ok;
                                inf.vendorID = vid.toInt(&ok, 16);

                                QString pid = line.section('/', 1,1);
                                inf.productID = pid.toInt(&ok, 16);

                                break;
                            }
                        }
                        vidPidFile.close();
                    }

                    QFile interfaces(driverSubDir+"/interface");
                    if( !interfaces.open(QFile::ReadOnly) )
                        continue;

                    QString name = interfaces.readLine().simplified();
                    if( !name.isEmpty() )
                        inf.friendName = name;
                }
            }
        }
        else if( portname.startsWith("ttyACM") )
        {
            // set fallbackup name
            inf.friendName = QString("USB CDC ACM adapter %1").arg(portname.mid(6));

            // now try to get more info about the adapter
            QFile uevent("/sys/class/tty/"+portname+"/device/uevent");
            if( uevent.open(QFile::ReadOnly) )
            {
                QString driverName;
                forever
                {
                    QString line = uevent.readLine();
                    if( line.isEmpty() )
                        break;

                    if( line.startsWith("DRIVER=") )
                        driverName = line.mid(7).simplified(); // cut the "DRIVER=" part

                    if( line.startsWith("PRODUCT=") )
                    {
                        line = line.mid(8); // cut the "PRODUCT=" part

                        QString vid = line.section('/',0,0);
                        bool ok;
                        inf.vendorID = vid.toInt(&ok, 16);

                        QString pid = line.section('/', 1,1);
                        inf.productID = pid.toInt(&ok, 16);
                    }
                }
                uevent.close();
            }
        }
        else if( portname.startsWith("rfcomm") )
        {
            // set fallbackup name
            inf.friendName = QString("Bluetooth-serial adapter %1").arg(portname.mid(6));

            // now try to get more info about the adapter
            QFile uevent("/sys/class/tty/"+portname+"/device/device/device/uevent");
            if( uevent.open(QFile::ReadOnly) )
            {
                QString driverName;
                forever
                {
                    QString line = uevent.readLine();
                    if( line.isEmpty() )
                        break;

                    if( line.startsWith("DRIVER=") )
                        driverName = line.mid(7).simplified(); // cut the "DRIVER=" part

                    if( line.startsWith("PRODUCT=") )
                    {
                        line = line.mid(8); // cut the "PRODUCT=" part

                        QString vid = line.section('/',0,0);
                        bool ok;
                        inf.vendorID = vid.toInt(&ok, 16);

                        QString pid = line.section('/', 1,1);
                        inf.productID = pid.toInt(&ok, 16);
                    }
                }
                uevent.close();
            }
        }

        infoList << inf;
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
