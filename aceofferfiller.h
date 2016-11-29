#ifndef ACEOFFERFILLER_H
#define ACEOFFERFILLER_H

#include <QDomDocument>
#include <QDomElement>
#include <QList>
#include <QMap>

class AceOfferFiller
{
    QString _filename;
    QDomDocument _doc;

    QList<QDomElement> _rows;

    typedef QString articul_t;
    typedef QString color_t;

    struct order_line_t {
        typedef QString size_t;
        typedef QString amount_t;

        articul_t articul;
        color_t   color;
        QMap<size_t, amount_t> size_amount_map;

        QStringList lines;  /* строки из файла */

        operator QString() const
        {
            return QString( "%1:%2:%3" ).
                   arg( articul ).
                   arg( color ).
                   arg( ( ( QStringList )size_amount_map.keys() ).join( "," ) );
        }
    };

    typedef QMap< color_t, order_line_t> color_size_map_t;
    QMap< articul_t, color_size_map_t > _order;

public:
    AceOfferFiller();

    bool load( const QString& filename );
    bool loadOrder( const QString& orderfile );
    bool save( const QString& fname = QString() );

    int maxRows()
    {
        return _rows.size();
    }

    QString cellValue( int row, int column );
    bool setCellValue( int row, int col, const QString& txt );

    bool parse();

private:
    bool parseTable();
    bool parseRow( QDomNode np );
    QString text( const QDomElement& el );
    QDomElement& rowElement( int row );
    QDomNode getColumnNode( int row, int col, /* OUT */ bool& isRangeNode );
    bool setCellText( QDomNode cellNode, const QString& txt );
};

#endif // ACEOFFERFILLER_H
