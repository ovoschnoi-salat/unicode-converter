# Лабораторная работа Unicode Converter
## Перекодировка
### Описание:
Программа выполняет перекодировку текстового файла.
### Запуск:
```
mkdir ./cmake-build
cmake -S ./ -B ./cmake-build
cmake --build ./cmake-build
./cmake-build/lab2 *.in *.out encoding
```
#### Программа должна запускаться с тремя аргументами:
* Первый аргумент должен содержать имя входного файла.
* Второй аргумент должен содержать имя выходного файла.
* Третий аргумент должен содержать кодировку выходного файла:
    *  0 – UTF-8 без BOM
    *  1 – UTF-8 с BOM
    *  2 – UTF-16 Little Endian
    *  3 – UTF-16 Big Endian
    *  4 – UTF-32 Little Endian
    *  5 – UTF-32 Big Endian

