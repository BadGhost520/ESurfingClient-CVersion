//
// Created by bad_g on 2025/9/27.
//
#include "../headFiles/utils/XMLParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 通用XML标签内容提取函数
char* extract_xml_tag_content(const char* xml_data, const char* tag_name) {
    if (!xml_data || !tag_name) {
        return NULL;
    }

    // 构造开始标签 <tag_name>
    char start_tag[256];
    snprintf(start_tag, sizeof(start_tag), "<%s>", tag_name);

    // 构造结束标签 </tag_name>
    char end_tag[256];
    snprintf(end_tag, sizeof(end_tag), "</%s>", tag_name);

    // 查找开始标签
    const char* start_pos = strstr(xml_data, start_tag);
    if (!start_pos) {
        return NULL;
    }

    // 移动到开始标签后面
    start_pos += strlen(start_tag);

    // 查找结束标签
    const char* end_pos = strstr(start_pos, end_tag);
    if (!end_pos) {
        return NULL;
    }

    // 计算内容长度
    size_t content_length = end_pos - start_pos;

    // 分配内存并复制内容
    char* content = (char*)malloc(content_length + 1);
    if (!content) {
        return NULL;
    }

    strncpy(content, start_pos, content_length);
    content[content_length] = '\0';

    return content;
}

// 从XML字符串中提取ticket值
char* XML_Parser(const char* xml_data, const char* tag) {
    return extract_xml_tag_content(xml_data, tag);
}