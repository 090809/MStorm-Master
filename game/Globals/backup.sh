#!/bin/bash
# Пароль пользователя root для  Mysql
PASSWD="_geroy2121"
# путь к базам MySQL
PATH1=/var/lib/mysql/
# путь для бакапа
PATH2=/root/mysql_backups/
# Шаблон даты/времени, добавляемый в имя файла бакапа
TMSTMP=`date +%d-%m-%Y_%H:%M`
 
if [  -d "$PATH1" ]
then
echo ok &> /dev/null
else
echo "Source folser don't exist!!!"
exit
fi
 
# Проверяем, существует ли папка, куда складываем бакап
if [  -d "$PATH2" ]
then
echo ok &> /dev/null
else
echo "Creating destination folder!!!"
mkdir $PATH2
fi
 
backup_name="$PATH2"archive-$TMSTMP
 
# Удаляем из папки с бакапами все файлы старше 7 дней
find $PATH2 -mtime +7 -exec rm {} \;
 
# Бакапим все базы данных
find $PATH1 -maxdepth 1 -type d | while read x; do
dir_name=`basename $x`
mysqldump -uroot -p$PASSWD --add-drop-table --add-locks --all --quick --lock-tables $dir_name > $PATH2$dir_name-$TMSTMP.sql
done
#cd $PATH2
zip -m "$backup_name" "$PATH2"*.sql &> /dev/null