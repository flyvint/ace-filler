#include <QCoreApplication>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QXmlSimpleReader>
#include <QXmlInputSource>
#include <QDir>

#include <QDomDocument>

#include <unistd.h>

#include "aceofferfiller.h"

bool fill_ace_order( const QString& odsfile, const QString& orderfile );
bool parse_ods_content( const QString& xmlfile,
                        const QString& orderfile, /* OUT */ int& processedOrdersCount );

void usage()
{
    qCritical( "usage: %s ace.xls | ace.ods aceorder.csv outfile.xls",
               qApp->applicationName().toLocal8Bit().constData() );
    exit( 1 );
}

bool convert_xls_to_ods( const QString& infile, const QString& outfile )
{
    QString cmd = "unoconv --format=ods -o \"" + outfile + "\" \"" + infile + "\"";
    qDebug() << "convert " << infile << " cmd[" << cmd << "]";
    if( system( cmd.toLocal8Bit().constData() ) != 0 ) {
        if( system( cmd.toLocal8Bit().constData() ) != 0 ) {
            qCritical( "!!! cant convert to ods" );
            return false;
        }
    }

    return true;
}

bool convert_ods_to_xls( const QString& infile, const QString& outfile )
{
    QString cmd = "unoconv --format=xls -o \"" + outfile + "\" \"" + infile + "\"";
    qDebug() << "convert " << infile << " cmd[" << cmd << "]";
    if( system( cmd.toLocal8Bit().constData() ) != 0 ) {
        if( system( cmd.toLocal8Bit().constData() ) != 0 ) {
            qCritical( "!!! cant convert to xls" );
            return false;
        }
    }

    return true;
}

int main( int argc, char *argv[] )
{
    QCoreApplication a( argc, argv );

    if( argc < 4 ) {
        usage();
    }

    QString infile = argv[1];
    if( infile.endsWith( ".xls" ) ) {
        QString outfile = infile;
        outfile.replace( ".xls", ".ods" );

        if( ! convert_xls_to_ods( infile, outfile ) ) {
            return 1;
        }

        infile = outfile;
    }

    if( ! infile.endsWith( ".ods" ) ) {
        usage();
    }

    QString orderfile = argv[2];
    QString outfile = argv[3];

    if( ! fill_ace_order( infile, orderfile ) ) { /// FIXME: continue ordercount
        qCritical( "!!! Ошибка при выполнении программы" );
        return 1;
    }

    if( ! convert_ods_to_xls( infile, outfile ) ) {
        return 1;
    }

    return 0;
}

bool fill_ace_order( const QString& odsfile, const QString& orderfile )
{
    QFileInfo ofi( odsfile );

    QString tmpdir = QString( "%1/tmp/ace-filler.%2.d" ).arg(
                         qgetenv( "HOME" ).constData() ).arg( getpid() ) ;
    //    QString tmpdir = QString( "ace-filler.%1.d" ).arg( getpid );

    QDir tmpdird( tmpdir );
    tmpdird.removeRecursively();

    if( system( qPrintable( QString( "mkdir -p \"%1\"" ).arg( tmpdir ) ) ) != 0 ) {
        qCritical( "!!! не могу создать временный каталог" );
        return false;
    }

    QString cmd_unzip = QString( "unzip -q -o \"%1\" -d \"%2\"" ).arg(
                            ofi.absoluteFilePath() ).arg( tmpdir );
    qDebug() << "unzip cmd[" << cmd_unzip << "]";

    if( system( cmd_unzip.toLocal8Bit().constData() ) != 0 ) {
        qCritical( "!!! cant unzip: %s", qPrintable( odsfile ) );
        return false;
    }

    int orders;
    qDebug( "parse file %s", qPrintable( odsfile ) );
    if( ! parse_ods_content( QString( "%1/%2" ).arg( tmpdir ).arg( "content.xml" ),
                             orderfile, orders ) ) {
        qDebug( "cant parse" );
        return false;
    }

    if( orders == 0 ){
        qInfo("!!! В этом файле не найдено ни одного заказа");
        return true;
    }

    QString cmd = QString( "cd \"%1\" && zip -q -r \"%2\" ." )
                  .arg( tmpdir )
                  .arg( ofi.absoluteFilePath() );
    qDebug() <<  "save ods: " << cmd;
    if(  system( cmd.toLocal8Bit().constData() ) != 0 ) {
        return false;
    }

    return true;
}

bool parse_ods_content( const QString& xmlfile,
                        const QString& orderfile, /* OUT */ int& processedOrdersCount )
{
    AceOfferFiller p;
    if( ! p.load( xmlfile ) ) {
        return false;
    }
    if( ! p.loadOrder( orderfile ) ) {
        return false;
    }

    if( ! p.parse() ) {
        return false;
    }

    if( p.processedOrdersCount() == 0 ) {
        qDebug( "!!! Не найдено ни одного заказа" );
        return true;
    }

    if( ! p.save() ) {
        return false;
    }

    if( ! p.saveUnprocessedOrders( orderfile ) ) {
        return false;
    }

    return true;
}
