#ifndef ACEOFFERFILLER_H
#define ACEOFFERFILLER_H

#include <QDomDocument>
#include <QDomElement>
#include <QList>

class AceOfferFiller
{
    QString _filename;
    QDomDocument _doc;
    int _maxRow;

    QList<QDomElement> _rows;

public:
    AceOfferFiller();

    bool load( const QString& filename );
    bool save();

    bool parse();
    int maxRows() { return _maxRow; }
    QString cellValue( int row, int column );

    bool parseTable();
    bool parseRow( QDomNode np );
};

#endif // ACEOFFERFILLER_H
