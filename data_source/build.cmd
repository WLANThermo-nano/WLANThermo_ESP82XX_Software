cd %~dp0
gzip.exe -k -f %~dp0data\index.html %~dp0data\index.html.gz
move data\index.html.gz ..\data\index.html.gz
copy data\language.js ..\data\language.js





