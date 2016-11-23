#ifndef ACEOFFERFILLER_H
#define ACEOFFERFILLER_H

#include <QDomDocument>
#include <QDomElement>
#include <QList>

class AceOfferFiller
{
    QString _filename;
    QDomDocument _doc;

    QList<QDomElement> _rows;

public:
    AceOfferFiller();

    bool load( const QString& filename );
    bool save();

    bool parse();
    int maxRows() { return _rows.size(); }
    QString cellValue( int row, int column );
    QString text( const QDomElement &el );

    bool parseTable();
    bool parseRow( QDomNode np );

};

#endif // ACEOFFERFILLER_H
