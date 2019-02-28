Программа называется vlt (video lan text).

1. Компиляция

Программа сообиралась на ОС Linux 4.9.0-8-amd64 #1 SMP Debian 4.9.144-3 (2019-02-02) x86_64 GNU/Linux (KDE). 
Компилятор gcc (Debian 6.3.0-18+deb9u1) 6.3.0 20170516
Стандартные библиотеки - стоковые из стандартных репозиториев.

В каталоге ./ffmpeg находятся файлы, касающиеся сборки этой библиотеки (комплекта библиотек).
ffmpeg был собран из исходников. Архив ffmpeg-4.1.tar.bz2 получен с офф.сайта.
Опции сборки указаны в файле build. Сборка только статических библиотек, prefix=/usr/local. 
.deb пакет так же доступен в этом каталоге.


Сборка программ делается утилитой make на основе make-файла в корне каталога с программой.
Зависимости: *-dev версии пакетов библиотек ffmpeg и пакет libass-dev.
с ffmpeg программа у меня линковалась статически. Бинарник доступен в каталоге ./bin.

Для запуска этого бинариника как есть нужно доусатновить зависимости. Как вариант, для 
десктопной системы:

apt install libass5 libzmq5 libx264-148 libx265-95 libfdk-aac1 libopus0 libmp3lame0 libvpx4

dpkg -i ffmpeg_4.1-1_amd64.deb

2. Работа программы.
Программа выполняет функции согласно заданию.

./vlt
Usage: vlt -f <file> [slrutavh]
        -f, --file       video file for streaming (required)
        -s, --stream     stream: udp://<ip>:<port> (default 'udp://127.0.0.1:1234')
        -l, --loop       looping stream, 0 - infinitely loop (default 0)
        -r, --res        http resource(default 'sub')
        -u, --url        http url to listen (default off)
        -t, --title      initial title (default empty string)
        -a, --audio      enable stream audio (default disabled)
        -v, --version    print version and exit
        -h, --help       print this help and exit

        example: vlt -u http://127.0.0.1:4545 -r settext -s udp://224.1.1.3:1234 -f /home/user/my_video.mp4 -t 'Default subtitle' -a

        set text on video: curl http://127.0.0.1:4545/settext?text=hello 

        or open http://127.0.0.1:4545/settext in your web browser and try it

Тестовый видеофайл находится в каталоге ./assets. Пример запуска - скрипт run.bash.

3. Проблемы.

Я не до конца разобрался с тем, как правильно освобождать ресурсы, занятые объектами ffmeg. Программа дает утечку.
В качестве дополнительного параметра к строке stream параметра, возможно, нужно добавить размер пакета:

udp://127.0.0.1:1234?pkt_size=1316







