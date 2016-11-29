#!/bin/bash

cleverfile=$( kdialog --title "Файл заказа Clever" --getopenfilename $HOME "application/vnd.ms-excel application/vnd.oasis.opendocument.spreadsheet" )
[[ $? != "0" ]] && exit 1

smfile=$( kdialog --title "Файл заказа Супермамок" --getopenfilename "$( dirname $cleverfile )" "text/csv" )
[[ $? != "0" ]] && exit 1

echo "$cleverfile"
echo "$smfile"

kdialog --msgbox "Делаю заказ..." &
msgpid=$!

tmplog="/tmp/ace-filler.$$.log"
outfile="${cleverfile%.xls}.out.xls"

./ace-filler "$cleverfile" "$smfile" "$outfile" 2>&1 | tee $tmplog
retv=${PIPESTATUS[0]}
kill $msgpid

if [ $retv -eq 0 ]; then
    kdialog --msgbox "Заказ сохранен в файле $outfile"
    grep "!!!" $tmplog > ${tmplog}2
    kdialog --textbox ${tmplog}2 600 400
else
    kdialog --msgbox "Не получилось сделать заказ :("
    kdialog --textbox ${tmplog} 600 400
fi
