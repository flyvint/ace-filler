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
    bool save( const QString& fname= QString() );

    int maxRows() { return _rows.size(); }

    QString cellValue( int row, int column );
    bool setCellValue( int row, int col, const QString& txt );

    bool parse();

private:
    bool parseTable();
    bool parseRow( QDomNode np );
    QString text( const QDomElement &el );
    QDomElement& rowElement( int row );
    QDomNode getColumnNode( int row, int col, /* OUT */ bool& isRangeNode );
    bool setCellText( QDomNode cellNode, const QString& txt );
};

#endif // ACEOFFERFILLER_H
