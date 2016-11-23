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

bool AceOfferFiller::save()
{
    QFile outf( "out.xml" );
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

    for( int r = 0; r < maxRows(); r++ ) {
        QString t = cellValue( r, 0 );
        if( t.contains( "CLE" ) ) {
            qDebug() << "found CLE:" << t << " on row:" << r;

            for( int c= 1; c < 40; c++ ){
                QString cv= cellValue( r, c );
                if( cv.contains( rexSize ) ){
                    qDebug( " col:%d size:%s", c, cv.toLocal8Bit().data() );
                }
            }

        }



//        qDebug( "%d,1=%s \t %d,14=%s",
//                r, cellValue( r, 1 ).toLocal8Bit().data(),
//                r, cellValue( r, 14 ).toLocal8Bit().data() );
    }

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
    int curcol = 0;

    if( row >= _rows.size() ) {
        return "";
    }

    QString colrange;
    QDomElement& rowel = _rows[ row ];
    for( QDomNode n = rowel.firstChild(); !n.isNull(); n = n.nextSibling() ) {
        QDomElement el = n.toElement();
        if( el.tagName() != "table:table-cell" ) {
            continue;
        }

        if( curcol == column ) {
            return text( el );
        }

        colrange = el.attribute( "table:number-columns-repeated" );
        if( ! colrange.isEmpty() ) {
            int crange = colrange.toInt();
            if( curcol < column && column < ( curcol + crange ) ) {
                return "";
            } else {
                curcol += crange;
            }

        } else {
            curcol++;
        }
    }

    return "";
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
