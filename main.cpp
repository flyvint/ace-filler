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

void changeODS( const QString& odsfile, const QString& orderfile );
bool parseContent( const QString& xmlfile, const QString& orderfile );

void usage()
{
    qFatal( "usage: %s ace.xls | ace.ods aceorder.csv",
            qApp->applicationName().toLocal8Bit().constData() );
}

bool convert2ods( const QString& infile, const QString& outfile )
{
    QString cmd = "unoconv --format=ods -o " + outfile + " " + infile;
    qDebug() << "convert " << infile << " cmd[" << cmd << "]";
    if( system( cmd.toLocal8Bit().constData() ) != 0 ) {
        if( system( cmd.toLocal8Bit().constData() ) != 0 ) {
            qCritical( "cant convert to ods" );
            return false;
        }
    }

    return true;
}

int main( int argc, char *argv[] )
{
    QCoreApplication a( argc, argv );

    if( argc < 3 ) {
        usage();
    }

    QString infile = argv[1];
    if( infile.endsWith( ".xls" ) ) {
        QString outfile = infile;
        outfile.replace( ".xls", ".ods" );

        if( ! QFile::exists( outfile ) ) {
            convert2ods( infile, outfile );
        }
        infile = outfile;
    }

    if( ! infile.endsWith( ".ods" ) ) {
        usage();
    }

    QString orderfile = argv[2];

    changeODS( infile, orderfile );

    return 0;
}

void changeODS( const QString& odsfile, const QString& orderfile )
{
    //    QString tmpdir= QString("~/tmp/ace-filler.%1.d").arg( odsfile );
    QString tmpdir = QString( "ace-filler.%1.d" ).arg( odsfile );

    QDir tmpdird( tmpdir );
    if( ! tmpdird.exists() ) {

        system( QString( "mkdir -p %1" ).arg( tmpdir ).toLocal8Bit().constData() );

        QString cmd_unzip = QString( "unzip -o %1 -d %2" ).arg( odsfile ).arg( tmpdir );
        qDebug() << "unzip cmd[" << cmd_unzip << "]";

        if( system( cmd_unzip.toLocal8Bit().constData() ) != 0 ) {
            qWarning( "cant unzip: " );
            return;
        }
    }

    qDebug( "parse" );
    if( ! parseContent( QString( "%1/%2" ).arg( tmpdir ).arg( "content.xml" ),
                        orderfile ) ) {
        qDebug( "cant parse" );
        return;
    }

    QString cmd = QString( "cd \"%1\" && zip -q -r \"%2\" ." )
                  .arg( tmpdir )
                  .arg( QDir::currentPath() + "/" + odsfile );
    qDebug() <<  "save ods: " << cmd;
    system( cmd.toLocal8Bit().constData() );
}

bool parseContent( const QString& xmlfile, const QString& orderfile )
{
    AceOfferFiller p;
    if( ! p.load( xmlfile ) ) {
        return false;
    }
    if( ! p.loadOrder( orderfile ) ) {
        return false;
    }

    p.parse();
    p.save();

    return true;
}
