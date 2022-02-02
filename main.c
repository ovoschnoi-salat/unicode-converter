#include <stdio.h>
#include <string.h>

union bytes {
    unsigned int codePoint;
    unsigned char part[4];
};

void swapUChar(unsigned char *a, unsigned char *b) {
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

void change2ByteUtfEndian(union bytes *a) {
    swapUChar(&a->part[0], &a->part[1]);
}

void change4ByteUtfEndian(union bytes *a) {
    swapUChar(&a->part[1], &a->part[2]);
    swapUChar(&a->part[0], &a->part[3]);
}

void change4ByteUtf16Endian(union bytes *a) {
    swapUChar(&a->part[1], &a->part[0]);
    swapUChar(&a->part[2], &a->part[3]);
}

int printError(const int r, const char *str) {
    printf("Error: %s", str);
    return r;
}

int between(const unsigned char c, const unsigned char a, const unsigned char b) {
    if (c < a || c > b)
        return 0;
    return 1;
}

void wrongUtf8toErrorCodePoint(union bytes *point) {
    point->codePoint = 0xDC00 + point->part[0];
}

void ErrorCodePointToWrongUtf8(union bytes *point) {
    point->codePoint = point->part[0];
}

union bytes utf8toCodePoint(FILE *input) {
    union bytes point;
    point.codePoint = 0;
    fread(&point.part, sizeof(unsigned char), 1, input);
    if (point.part[0] < 0x80) {
        //корректная одно-битная последовательность
        return point;
    } else if (between(point.part[0], 0xC2, 0xDF)) {//двух-битная последовательность
        fread(&point.part[1], sizeof(unsigned char), 1, input);
        if (!feof(input)) {
            if (between(point.part[1], 0x80, 0xBF)) {
                //корректная двух-битная последовательность
                point.codePoint = ((point.part[0] - 0xC0) << 6) + (point.part[1] - 0x80);
                return point;
            } else {//незаконченная двух-битная последовательность
                fseek(input, -1, SEEK_CUR);
            }
        } //else незаконченная двух-битная последовательность
    } else if (between(point.part[0], 0xE0, 0xEF)) {//трёх-битная последовательность
        fread(&point.part[1], sizeof(unsigned char), 1, input);
        if (!feof(input)) {
            if (between(point.part[1], 0x80, 0xBF)) {
                if (point.part[0] == 0xE0 && point.part[1] < 0xA0 || //не минимальная трех-битная последовательность
                point.part[0] == 0xED && point.part[1] >= 0xA0) { //суррогатная пара
                    fseek(input, -1, SEEK_CUR);
                } else {
                    fread(&point.part[2], sizeof(unsigned char), 1, input);
                    if (feof(input)) {//незаконченная трех-битная последовательность
                        fseek(input, -1, SEEK_CUR);
                    } else if (between(point.part[2], 0x80, 0xBF)) {
                        //корректная трех-битная последовательность
                        point.codePoint = ((point.part[0] - 0xE0) << 12) +
                                          ((point.part[1] - 0x80) << 6) + (point.part[2] - 0x80);
                        return point;
                    } else {//незаконченная трех-битная последовательность
                        fseek(input, -2, SEEK_CUR);
                    }
                }
            } else {//незаконченная трех-битная последовательность
                fseek(input, -1, SEEK_CUR);
            }
        } //else незаконченная трех-битная последовательность
    } else if (between(point.part[0], 0xF0, 0xF4)) {//четырех-битная последовательность
        fread(&point.part[1], sizeof(unsigned char), 1, input);
        if (!feof(input)) {
            if (between(point.part[1], 0x80, 0xBF)) {
                if (point.part[0] == 0xF0 && point.part[1] < 0x90 || //не минимальная четырех-битная последовательность
                    point.part[0] == 0xF4 && point.part[1] >= 0x90) { //Выход за границы Unicode U+110000..U+13FFFF
                    fseek(input, -1, SEEK_CUR);
                } else {
                    fread(&point.part[2], sizeof(unsigned char), 1, input);
                    if (!feof(input)) {
                        if (between(point.part[2], 0x80, 0xBF)) {
                            fread(&point.part[3], sizeof(unsigned char), 1, input);
                            if (!feof(input)) {
                                if (between(point.part[3], 0x80, 0xBF)) {
                                    //корректная четырех-битная последовательность
                                    point.codePoint = ((point.part[0] - 0xF0) << 18) + ((point.part[1] - 0x80) << 12)
                                                      + ((point.part[2] - 0x80) << 6) + (point.part[3] - 0x80);
                                    return point;
                                } else {//незаконченная четырех-битная последовательность
                                    fseek(input, -3, SEEK_CUR);
                                }
                            } else {//незаконченная четырех-битная последовательность
                                fseek(input, -2, SEEK_CUR);
                            }
                        } else {//незаконченная четырех-битная последовательность
                            fseek(input, -2, SEEK_CUR);
                        }
                    } else {//незаконченная четырех-битная последовательность
                        fseek(input, -1, SEEK_CUR);
                    }
                }
            } else {//незаконченная четырех-битная последовательность
                fseek(input, -1, SEEK_CUR);
            }
        } //else незаконченная четырех-битная последовательность
    } //else неначатая , слишком длинная, не минимальная двух-битная, Выход за границы Unicode ≥ U+140000
    fseek(input, 0, SEEK_CUR);
    wrongUtf8toErrorCodePoint(&point);
    return point;
}

void utf16LELongToCP(union bytes *point) {
    point->codePoint = ((((point->part[1] - 0xD8) << 8) + point->part[0]) << 10) +
                       ((point->part[3] - 0xDC) << 8) + point->part[2] + 0x10000;
}

union bytes utf16LEtoCodePoint(FILE *input) {
    union bytes point;
    point.codePoint = 0;
    fread(&point.part, sizeof(unsigned char), 2, input);
    if (!feof(input) && between(point.part[1], 0xD8, 0xDB) &&
        fread(&point.part[2], sizeof(unsigned char), 2, input)) {
        if (between(point.part[3], 0xDC, 0xDF)) {
            utf16LELongToCP(&point);
        } else {
            point.part[2] = point.part[3] = 0;
            fseek(input, -2, SEEK_CUR);
        }
    }
    return point;
}

union bytes utf16BEtoCodePoint(FILE *input) {
    union bytes point;
    point.codePoint = 0;
    fread(&point.part, sizeof(unsigned char), 2, input);
    if (!feof(input) && between(point.part[0], 0xD8, 0xDB) &&
        fread(&point.part[2], sizeof(unsigned char), 2, input)) {
        if (between(point.part[2], 0xDC, 0xDF)) {
            change4ByteUtf16Endian(&point);
            utf16LELongToCP(&point);
            return point;
        }
        point.part[2] = point.part[3] = 0;
        fseek(input, -2, SEEK_CUR);
    }
    change2ByteUtfEndian(&point);
    return point;
}

union bytes utf32LEtoCodePoint(FILE *input) {
    union bytes point;
    point.codePoint = 0;
    fread(&point.part, sizeof(unsigned char), 4, input);
    return point;
}

union bytes utf32BEtoCodePoint(FILE *input) {
    union bytes point = utf32LEtoCodePoint(input);
    change4ByteUtfEndian(&point);
    return point;
}

void writeCodePointToUtf8(FILE *output, union bytes point) {
    if (point.codePoint < 0x80) {
        fwrite(&point.part, sizeof(unsigned char), 1, output);
    } else if (point.codePoint < 0x800) {
        point.part[3] = 0xC0 + (point.part[1] << 2) + (point.part[0] >> 6);
        point.part[2] = 0x80 + (point.part[0] & 0x3F);
        change4ByteUtfEndian(&point);
        fwrite(&point.part, sizeof(unsigned char), 2, output);
    } else if (point.codePoint < 0x10000) {
        if (point.part[1] == 0xDC && between(point.part[0], 0x80, 0xFF)) {
            ErrorCodePointToWrongUtf8(&point);
            fwrite(&point.part, sizeof(unsigned char), 1, output);
            return;
        }
        point.part[3] = 0xE0 + (point.part[1] >> 4);
        point.part[2] = 0x80 + ((point.part[1] & 0x0F) << 2) + (point.part[0] >> 6);
        point.part[1] = 0x80 + (point.part[0] & 0x3F);
        change4ByteUtfEndian(&point);
        fwrite(&point.part, sizeof(unsigned char), 3, output);
    } else if (point.codePoint < 0x200000) {
        point.part[3] = 0xF0 + (point.part[2] >> 2);
        point.part[2] = 0x80 + ((point.part[2] & 0x03) << 4) + (point.part[1] >> 4);
        point.part[1] = 0x80 + ((point.part[1] & 0x0F) << 2) + (point.part[0] >> 6);
        point.part[0] = 0x80 + (point.part[0] & 0x3F);
        change4ByteUtfEndian(&point);
        fwrite(&point.part, sizeof(unsigned char), 4, output);
    }
}

void CPLongToUtf16(union bytes *point) {
    point->codePoint -= 0x10000;
    point->codePoint = ((point->codePoint & 0x3FF | 0xDC00) << 16) + ((point->codePoint >> 10 & 0x3FF) | 0xD800);
}

void writeCodePointToUtf16BE(FILE *output, union bytes point) {
    if (point.codePoint > 0xFFFF) {
        CPLongToUtf16(&point);
        change4ByteUtf16Endian(&point);
        fwrite(&point.part, sizeof(unsigned char), 4, output);
    } else {
        change2ByteUtfEndian(&point);
        fwrite(&point.part, sizeof(unsigned char), 2, output);
    }
}

void writeCodePointToUtf16LE(FILE *output, union bytes point) {
    if (point.codePoint > 0xFFFF) {
        CPLongToUtf16(&point);
        fwrite(&point.part, sizeof(unsigned char), 4, output);
    } else {
        fwrite(&point.part, sizeof(unsigned char), 2, output);
    }
}

void writeCodePointToUtf32LE(FILE *output, union bytes point) {
    fwrite(&point.part, sizeof(unsigned char), 4, output);
}

void writeCodePointToUtf32BE(FILE *output, union bytes point) {
    change4ByteUtfEndian(&point);
    writeCodePointToUtf32LE(output, point);
}

int main(int argc, char **argv) {
    //checking if number of arguments is correct
    if (argc != 4) {
        return printError(1, "Incorrect input arguments.\n"
                             "This program expect launching with 3 arguments: *name* input output mode\n"
                             "Where:\n"
                             "    input - name of input file\n"
                             "    output - name of output file\n"
                             "    mode - encoding of output file \n");
    }
    //checking output mode
    unsigned int outputMode;
    if (strlen(argv[3]) != 1 || argv[3][0] < '0' || argv[3][0] > '5') {
        return printError(1, "Incorrect output mode argument.");
    } else {
        outputMode = argv[3][0] - '0';
    }
    //opening input file
    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        return printError(1, "Error occurred while opening input file.");
    }

    //reading bom and determining the encoding of file
    unsigned int inputMode;
    union bytes (*inputFunction)(FILE *);
    union bytes BOM;
    BOM.codePoint = 0;
    fread(&BOM.part[0], sizeof(unsigned char), 2, input);
    if (!feof(input)) {
        switch (BOM.codePoint) {
        case 0xBBEF:
            //utf8bom
            fread(&BOM.part[2], sizeof(unsigned char), 1, input);
            if (!feof(input)) {
                if (BOM.codePoint == 0xBFBBEF) {
                    inputFunction = utf8toCodePoint;
                    inputMode = 1;
                    break;
                }
            }
            //utf8
            inputFunction = utf8toCodePoint;
            inputMode = 0;
            fseek(input, 0, SEEK_SET);
            break;
        case 0xFEFF:
            fread(&BOM.part[2], sizeof(unsigned char), 2, input);
            if (!feof(input)) {
                if (BOM.codePoint == 0x0000FEFF) {
                    //utf32le or 16le
                    inputFunction = utf32LEtoCodePoint;
                    inputMode = 4;
                    break;
                }
                fseek(input, -2, SEEK_CUR);
            }
            //utf16le
            inputFunction = utf16LEtoCodePoint;
            inputMode = 2;
            break;
        case 0xFFFE:
            //utf16be
            inputFunction = utf16BEtoCodePoint;
            inputMode = 3;
            break;
        case 0x0000:
            fread(&BOM.part[2], sizeof(unsigned char), 2, input);
            if (!feof(input)) {
                if (BOM.codePoint == 0xFFFE0000) {
                    //utf32be
                    inputFunction = utf32BEtoCodePoint;
                    inputMode = 5;
                    break;
                }
            }
        default:
            //utf8
            inputFunction = utf8toCodePoint;
            inputMode = 0;
            fseek(input, 0, SEEK_SET);
            break;
        }
    } else {
        //utf8
        inputFunction = utf8toCodePoint;
        inputMode = 0;
        fseek(input, 0, SEEK_SET);
    }

    //setting suitable output function
    void (*outputFunction)(FILE *, union bytes);
    switch (outputMode) {
    case 0:
    case 1:
        outputFunction = writeCodePointToUtf8;
        break;
    case 2:
        outputFunction = writeCodePointToUtf16LE;
        break;
    case 3:
        outputFunction = writeCodePointToUtf16BE;
        break;
    case 4:
        outputFunction = writeCodePointToUtf32LE;
        break;
    case 5:
        outputFunction = writeCodePointToUtf32BE;
        break;
    }

    //opening output file
    FILE *output = fopen(argv[2], "wb");
    if (output == NULL) {
        if (fclose(input)) {
            return printError(1, "Error occurred while opening output file and closing input.");
        }
        return printError(1, "Error occurred while opening output file.");
    }

    //writing bom if needed
    if (outputMode != 0) {
        if (inputMode > 0) {
            fseek(input, 0, SEEK_SET);
        } else {
            BOM.codePoint = 0xFEFF;
            outputFunction(output, BOM);
        }
    }
    // decoding/encoding
    if (inputMode == 4) {
        //writing in utf32le and checking if it's not utf16le
        union bytes tmp;
        while (!feof(input)) {
            tmp = inputFunction(input);
            if (tmp.codePoint > 0x10FFFF) {
                inputFunction = utf16LEtoCodePoint;
                inputMode = 2;
                break;
            }
            if (!feof(input))
                outputFunction(output, tmp);
        }
        if (inputMode == 2) {
            fseek(output, 0, SEEK_SET);
            if (outputMode == 0) {
                fseek(input, 2, SEEK_SET);
            } else {
                fseek(input, 0, SEEK_SET);
            }
        }
    }
    if (inputMode == outputMode || inputMode + outputMode == 1) {
        unsigned char tmp;
        while (1) {
            fread(&tmp, sizeof(unsigned char), 1, input);
            if (feof(input)) {
                break;
            }
            fwrite(&tmp, sizeof(unsigned char), 1, output);
        }
    } else {
        union bytes tmp;
        while (1) {
            tmp.codePoint = 0;
            tmp = inputFunction(input);
            if (feof(input)) {
                break;
            }
            outputFunction(output, tmp);
        }
    }

    //closing output file
    if (fclose(output)) {
        if (fclose(input)) {
            return printError(2, "Error occurred while closing input and output files.");
        }
        return printError(2, "Error occurred while closing output file.");
    }
    //closing input file
    if (fclose(input)) {
        return printError(2, "Error occurred while closing input file.");
    }
    return 0;
}