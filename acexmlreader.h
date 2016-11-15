#ifndef ACEXMLREADER_H
#define ACEXMLREADER_H

#include <QXmlDefaultHandler>

class AceXmlReader : public QXmlDefaultHandler
{
public:
    AceXmlReader();
    bool fatalError ( const QXmlParseException& exception );
    bool startEntity(const QString &name);
};

#endif // ACEXMLREADER_H
