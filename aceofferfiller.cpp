#include "aceofferfiller.h"

#include <QFile>
#include <QDebug>

AceOfferFiller::AceOfferFiller()
{

}

bool AceOfferFiller::load( const QString& filename )
{
    _filename = filename;

    QFile file( _filename );
    if ( ! file.open( QIODevice::ReadOnly ) ) {
        qCritical() << "cant load file:" << _filename << " " << file.errorString();
        return false;
    }
    if ( ! _doc.setContent( &file ) ) {
        file.close();
        return false;
    }
    file.close();

    return parseTable();
}

bool AceOfferFiller::save( const QString& fname )
{
    QFile outf( fname.isEmpty() ? _filename : fname );
    if( ! outf.open( QIODevice::WriteOnly ) ) {
        qCritical() << "cant open out file: " << outf.errorString();
        return false;
    }

    outf.write( _doc.toString().toLocal8Bit() );
    outf.close();

    return true;
}

bool AceOfferFiller::parse()
{
    static QRegExp rexSize( "р[0-9][0-9]" );
    static QString colorPrefix( "Цвет:" );

    QMap<QString /*size*/, int /*column*/> size_column;
    color_size_map_t& cur_art= _order.first();

    for( int r = 0; r < maxRows(); r++ ) {
        QString cv = cellValue( r, 0 );
        if( _order.contains( cv ) ) {
            QString art= cv;
            qDebug() << "found articul:" << art << " from order on row:" << r;

            size_column.clear();
            for( int c = 1; c < 40; c++ ) {
                QString cv = cellValue( r, c );
                if( cv.contains( rexSize ) ) {
                    qDebug( " col:%d size:%s", c, cv.toLocal8Bit().data() );
                    cv.remove("р");
                    size_column[cv]= c;
                }
            }
        } else if( cv.contains( colorPrefix ) ) {
            QString color= cv.remove( colorPrefix ).trimmed();
            if( cur_art.contains( color ) ){
                QString s= cur_art[ color ].size;
                QString a= cur_art[ color ].amount;

                int sizecol= size_column[ s ];

                qDebug() << "  set amount:" << a << " on cell:" << r << "," << sizecol;
                setCellValue( r, sizecol, a );
            }
        }

    }

//    setCellValue( 4, 0, "Test" );
//    setCellValue( 4, 2, "Test222" );
    save();

    return true;
}

bool AceOfferFiller::parseTable()
{
    QDomElement root = _doc.firstChildElement( "office:document-content" );
    Q_ASSERT( ! root.isNull() );

    QDomElement body = root.firstChildElement( "office:body" );
    Q_ASSERT( ! body.isNull() );

    QDomElement ssheet = body.firstChildElement( "office:spreadsheet" );
    Q_ASSERT( ! ssheet.isNull() );

    QDomElement table = ssheet.firstChildElement( "table:table" );
    Q_ASSERT( ! table.isNull() );

    return parseRow( table.firstChild() );
}

bool AceOfferFiller::parseRow( QDomNode np )
{
    for( QDomNode n = np; !n.isNull(); n = n.nextSibling() ) {
        QDomElement el = n.toElement();
        if( el.tagName() == "table:table-row" ) {
            _rows.append( el );

        } else if( el.tagName() == "table:table-row-group" ) {
            if( ! parseRow( el.firstChild() ) ) {
                return false;
            }
        }
    }

    return true;
}


QString AceOfferFiller::cellValue( int row, int column )
{
    bool isRangeNode = false;
    QDomNode colnode = getColumnNode( row, column, isRangeNode );

    if( colnode.isNull() ) {
        return "";
    }

    // if target column in "<table:table-cell table:number-columns-repeated=""/>"
    if( isRangeNode ) {
        return "";
    }

    return text( colnode.toElement() );
}

QString AceOfferFiller::text( const QDomElement& elp )
{
    QString t;
    for( QDomElement el = elp.firstChildElement( "text:p" ); ! el.isNull();
            el = el.nextSiblingElement( "text:p" ) ) {
        t.append( el.text() );
    }
    return t;
}

bool AceOfferFiller::setCellValue( int row, int col, const QString& txt )
{
    if( row >= _rows.size() ) {
        return "";
    }

    bool isRangeNode = false;
    QDomNode cellNode = getColumnNode( row, col, isRangeNode );
    if( isRangeNode ) {
        // if target column in "<table:table-cell table:number-columns-repeated=""/>"
        QDomElement el = cellNode.toElement();
        int colRepeat =
            el.toElement().attribute( "table:number-columns-repeated" ).toInt();
        el.removeAttribute( "table:number-columns-repeated" );

        QDomNode parent = cellNode.parentNode();
        Q_ASSERT( ! parent.isNull() );

        for( int c = 1; c < colRepeat; c++ ) {
            QDomElement newCellNode = _doc.createElement( "table:table-cell" );
            newCellNode.setAttribute( "table:style-name",
                                      el.attribute( "table:style-name" ) );
            parent.insertAfter( newCellNode, cellNode );

            QDomElement newCellNodeP = _doc.createElement( "text:p" );
            newCellNode.appendChild( newCellNodeP );

            QDomText newCellNodePText = _doc.createTextNode( "" );
            newCellNodeP.appendChild( newCellNodePText );
        }

        cellNode = getColumnNode( row, col, isRangeNode );
    }

    setCellText( cellNode, txt );

    return true;
}

QDomElement& AceOfferFiller::rowElement( int row )
{
    return _rows[row];
}

QDomNode AceOfferFiller::getColumnNode( int row,
                                        int col, /* OUT */ bool& isRangeNode )
{
    isRangeNode = false;

    int curcol = 0;

    if( row >= _rows.size() ) {
        return QDomNode();
    }

    QString colrange;
    QDomElement& rowel = rowElement( row );
    for( QDomNode n = rowel.firstChild(); !n.isNull(); n = n.nextSibling() ) {
        QDomElement el = n.toElement();
        if( el.tagName() != "table:table-cell" ) {
            continue;
        }

        colrange = el.attribute( "table:number-columns-repeated" );

        if( curcol == col ) {
            if( ! colrange.isEmpty() ) {
                isRangeNode = true;
            }
            return n;
        }

        if( ! colrange.isEmpty() ) {
            int crange = colrange.toInt();
            if( curcol < col && col < ( curcol + crange ) ) {
                isRangeNode = true;
                return n;
            } else {
                curcol += crange;
            }

        } else {
            curcol++;
        }
    }

    return QDomNode();
}

bool AceOfferFiller::setCellText( QDomNode cellNode, const QString& txt )
{
    static QString txtTag = "text:p";
    QDomElement txtNode = cellNode.toElement().firstChildElement( txtTag );
    while( ! txtNode.isNull() ) {
        cellNode.removeChild( txtNode );
        txtNode = cellNode.toElement().firstChildElement( txtTag );
    }

    QDomElement elp = _doc.createElement( txtTag );
    QDomText txtn = _doc.createTextNode( txt );
    cellNode.appendChild( elp );
    elp.appendChild( txtn );

    return true;
}

bool AceOfferFiller::loadOrder( const QString& orderfile )
{
    QFile f( orderfile );
    if ( ! f.open( QIODevice::ReadOnly ) ) {
        qCritical() << "cant load order file:" << orderfile << " " <<
                    f.errorString();
        return false;
    }

    QTextStream in( &f );
    in.setCodec( "UTF-8" );
    while ( !in.atEnd() ) {
        QString line = in.readLine();

        QStringList l = line.split( ';' );
        qDebug() << l;

        if( l[0] == "Наименование" ) {
            continue;
        }

        order_line_t o;
        o.articul = l[0].trimmed();
        o.color = l[1].trimmed();
        o.size = l[2];
        o.amount = l[4];

        color_size_map_t& csm= _order[o.articul];
        csm[ o.color ]= o;

        qDebug() << _order;
    }

    return true;
}



















