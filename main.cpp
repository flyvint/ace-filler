#include <QCoreApplication>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QXmlSimpleReader>
#include <QXmlInputSource>

#include <unistd.h>

#include "acexmlreader.h"


void changeODS( const QString& odsfile);
bool parseContent( const QString& xmlfile );

void usage()
{
    qFatal( "usage: %s ace.xls | ace.ods", qApp->applicationName().toLocal8Bit().constData() );
}

bool convert2ods( const QString& infile, const QString& outfile )
{
    QString cmd= "unoconv --format=ods -o " + outfile + " " + infile;
    qDebug() << "convert " << infile << " cmd[" << cmd << "]";
    if( system( cmd.toLocal8Bit().constData() ) != 0 ){
        if( system( cmd.toLocal8Bit().constData() ) != 0 ){
            qCritical( "cant convert to ods" );
            return false;
        }
    }

    return true;
}




int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if( argc < 2 )
        usage();

    QString infile= argv[1];
    if( infile.endsWith(".xls") ){
        QString outfile= infile;
        outfile.replace(".xls", ".ods");

        if( ! QFile::exists( outfile ) )
            convert2ods( infile, outfile );
        infile= outfile;
    }

    if( ! infile.endsWith(".ods") )
        usage();

    changeODS( infile );

    return 0;
}

void changeODS( const QString& odsfile)
{
    QString tmpdir= QString("~/tmp/ace-filler.%1").arg( getpid() );

    system( QString("mkdir -p %1").arg( tmpdir ).toLocal8Bit().constData() );

    QString cmd_unzip= QString("unzip %1 -d %2").arg( odsfile ).arg( tmpdir );
    qDebug() << "unzip cmd[" << cmd_unzip << "]";

    if( system( cmd_unzip.toLocal8Bit().constData() ) != 0 ){
        qWarning("cant unzip: ");
        return;
    }


    if( ! parseContent( QString( "%1/%2").arg(tmpdir).arg("content.xml") ) ){
        qDebug("cant parse");
        return;
    }
}

bool parseContent( const QString& xmlfile )
{
    qDebug() << "parsing: " << xmlfile ;

    QXmlSimpleReader xmlReader;
    QXmlInputSource *source = new QXmlInputSource( new QFile(xmlfile) );

    AceXmlReader *handler = new AceXmlReader;
    xmlReader.setContentHandler(handler);
    xmlReader.setErrorHandler(handler);

    bool ok = xmlReader.parse(source);

    if (!ok)
        qCritical("Parsing failed.");

    return true;
}
