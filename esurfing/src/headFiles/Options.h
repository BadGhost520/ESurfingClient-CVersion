#ifndef ESURFINGCLIENT_OPTIONS_H
#define ESURFINGCLIENT_OPTIONS_H

typedef struct
{
    int isSmallDevice;
    int isDebug;
    char* usr;
    char* pwd;
    char* chn;
} Options;

extern Options opt;

/**
 * 更改选项
 * @param options 设置结构体
 */
void setOpt(Options options);

#endif //ESURFINGCLIENT_OPTIONS_H