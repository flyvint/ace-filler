#include "aceofferfiller.h"

#include <QFile>
#include <QDebug>
#include <QRegularExpression>

AceOfferFiller::AceOfferFiller()
{

}

bool AceOfferFiller::load( const QString& filename )
{
    _filename = filename;

    QFile file( _filename );
    if ( ! file.open( QIODevice::ReadOnly ) ) {
        qCritical() << "!!! не могу открыть файл:" << _filename << " " << file.errorString();
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
        qCritical() << "!!! не могу открыть файл: " << outf.errorString();
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
    static color_size_map_t nul;

    QMap<QString /*size*/, int /*column*/> size_column;
    color_size_map_t& cur_color_size_map = nul;
    QString cur_art;

    for( int r = 0; r < maxRows(); r++ ) {
        QString cv = cellValue( r, 0 );
        if( _order.contains( cv ) ) {
            cur_art = cv;
            cur_color_size_map= _order[cv];
            qDebug() << "found articul:" << cur_art << " from order on row:" << r;

            size_column.clear();
            for( int c = 1; c < 40; c++ ) {
                QString cv = cellValue( r, c );
                if( cv.contains( rexSize ) ) {
//                    qDebug( " col:%d size:%s", c, cv.toLocal8Bit().data() );
                    cv.remove( "р" );
                    size_column[cv] = c;
                }
            }
        } else if( ( ! cur_art.isEmpty() ) && cv.contains( colorPrefix ) ) {
            QString color = cv.remove( colorPrefix ).trimmed();
            qDebug() << " color:" << color << " map:" << cur_color_size_map.keys();
            if( cur_color_size_map.contains( color ) ) {
                qDebug() << " found color:" << color;
                foreach( order_line_t::size_t s,
                         cur_color_size_map[color].size_amount_map.keys() ) {
                    QString a = cur_color_size_map[color].size_amount_map[s];

                    int sizecol = size_column[ s ];

                    qDebug() << "  set amount:" << a << " on size:" << s << " cell:" << r << "," << sizecol;
                    setCellValue( r, sizecol, a );
                    _order[cur_art][color].size_amount_map[s]= "done"; // mark item as processed
                }
            }
        } else if( cv.contains( "Итого" ) ) {
            size_column.clear();
            cur_color_size_map = _order.first();
            cur_art.clear();
        }
    }

//    save( "out.xml" );
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
        QString styleAttr = el.attribute( "table:style-name" );
        qDebug() <<  "   el style:" << styleAttr;

        int colRepeat =
            el.toElement().attribute( "table:number-columns-repeated" ).toInt();
        el.removeAttribute( "table:number-columns-repeated" );

        QDomNode parent = cellNode.parentNode();
        Q_ASSERT( ! parent.isNull() );

        for( int c = 1; c < colRepeat; c++ ) {
            QDomElement newCellNode = _doc.createElement( "table:table-cell" );
            if( ! styleAttr.isEmpty() )
                newCellNode.setAttribute( "table:style-name",
                                          styleAttr );
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


QString valueType( const QString& txt )
{
    bool isOk;
    QString t= txt;
    t.toFloat( &isOk );
    if( isOk ) {
        return "float";
    }
    return "string";
}

bool AceOfferFiller::setCellText( QDomNode cellNode, const QString& txt )
{
    static QString txtTag = "text:p";

    QDomElement cellEl = cellNode.toElement();

    QString valType = valueType( txt );
    cellEl.setAttribute( "office:value-type", valType );
    cellEl.setAttribute( "calcext:value-type", valType );
    if( valType == "float" ) {
        cellEl.setAttribute( "office:value", txt );
    }

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
        qCritical() << "!!! не могу открыть файл заказа:" << orderfile << " " <<
                    f.errorString();
        return false;
    }

    QTextStream in( &f );
//    in.setCodec( "UTF-8" );
    in.setCodec( "CP1251" );
    while ( !in.atEnd() ) {
        QString line = in.readLine();
        if( line.isEmpty() ) {
            continue;
        }

        QStringList l = line.split( ';' );
        qDebug() << "orderline: " << l;

        if( l.size() < 6 || l[0].contains( "Наименование" ) ) {
            continue;
        }

        QString articul = l[0].trimmed();
        QString color = l[1].trimmed();
        QString size = l[2];
        QString amount = l[4];
        amount.remove( QRegularExpression(",.*") );

        color_size_map_t& csm = _order[ articul ];
        order_line_t& ol = csm[ color ];
        ol.articul = articul;
        ol.color = color;
        ol.size_amount_map[ size ] = amount;
        ol.lines.append( line );

        qDebug() << "order: " << articul << " " << csm;
    }

    return true;
}

bool AceOfferFiller::saveUnprocessedOrders( const QString& orderfile )
{
    QFile fo( orderfile );

    if( ! fo.open( QIODevice::WriteOnly ) ){
        qCritical("!!! cant open file:%s for writing", qPrintable(orderfile));
        return false;
    }

    qDebug() << "";
    foreach( color_size_map_t csm, _order ) {
        foreach( order_line_t ol, csm ) {
            foreach ( order_line_t::size_t s, ol.size_amount_map.keys() ) {
                if( ol.size_amount_map[s] != "done" ) {
                    qCritical( "!!! не найден в файле заказа: артикул[%s] цвет[%s] размер[%s]",
                               qPrintable( ol.articul ),
                               qPrintable( ol.color ),
                               qPrintable( s ) );
                    fo.write( ol.lines )
                }
            }
        }
    }
    qDebug() << "";

    fo.close();
}
