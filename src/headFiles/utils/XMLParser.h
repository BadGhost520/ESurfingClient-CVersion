//
// Created by bad_g on 2025/9/27.
//

#ifndef ESURFINGCLIENT_XMLPARSER_H
#define ESURFINGCLIENT_XMLPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

    // 从XML字符串中提取ticket值
    // 参数: xml_data - XML字符串
    // 返回: ticket值的字符串，需要调用者释放内存，失败返回NULL
    char* extract_ticket(const char* xml_data);

    // 从XML字符串中提取expire值
    // 参数: xml_data - XML字符串
    // 返回: expire值的字符串，需要调用者释放内存，失败返回NULL
    char* extract_expire(const char* xml_data);

    // 通用XML标签内容提取函数
    // 参数: xml_data - XML字符串, tag_name - 标签名(不包含<>)
    // 返回: 标签内容的字符串，需要调用者释放内存，失败返回NULL
    char* extract_xml_tag_content(const char* xml_data, const char* tag_name);

#ifdef __cplusplus
}
#endif

#endif //ESURFINGCLIENT_XMLPARSER_H