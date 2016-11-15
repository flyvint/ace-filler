#include "acexmlreader.h"

#include <QDebug>

AceXmlReader::AceXmlReader()
    : QXmlDefaultHandler()
{

}

bool AceXmlReader::fatalError ( const QXmlParseException& exception )
{
    qWarning() << "Fatal error on line" << exception.lineNumber()
               << ", column" << exception.columnNumber() << ':'
               << exception.message();

    return false;
}

bool AceXmlReader::startEntity(const QString &name)
{
    qDebug() << "startEnt: "  << name;

    return true;
}
