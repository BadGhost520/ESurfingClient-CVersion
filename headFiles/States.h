//
// Created by bad_g on 2025/9/22.
//

#ifndef ESURFINGCLIENT_STATES_H
#define ESURFINGCLIENT_STATES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * C语言版本的HashMap<String, String>实现
 *
 * 对应Kotlin代码：
 * var extraCfgUrl = HashMap<String, String>()
 *
 * 功能：
 * - 存储字符串键值对
 * - 动态扩容
 * - 哈希冲突处理（链地址法）
 * - 内存自动管理
 */

// 默认配置
#define HASHMAP_DEFAULT_CAPACITY 16
#define HASHMAP_MAX_LOAD_FACTOR 0.75
#define HASHMAP_MAX_KEY_LENGTH 256
#define HASHMAP_MAX_VALUE_LENGTH 1024

// 哈希表节点结构
typedef struct HashMapNode {
    char* key;                      // 键
    char* value;                    // 值
    struct HashMapNode* next;       // 链表下一个节点
} HashMapNode;

// 哈希表结构
typedef struct HashMap {
    HashMapNode** buckets;          // 桶数组
    size_t capacity;                // 容量
    size_t size;                    // 当前元素数量
    double load_factor;             // 负载因子阈值
} HashMap;

// 迭代器结构
typedef struct HashMapIterator {
    HashMap* map;                   // 指向的哈希表
    size_t bucket_index;            // 当前桶索引
    HashMapNode* current_node;      // 当前节点
} HashMapIterator;

// 核心函数声明

extern char* clientId;
extern char* algoId;
extern char* macAddress;
extern char* ticket;
extern char* userIp;
extern char* acIp;

extern bool isRunning;

extern char* schoolId;
extern char* domain;
extern char* area;
extern char* ticketUrl;
extern char* authUrl;
extern bool isLogged;

void initExtraCfgUrl();

/**
 * 创建新的HashMap
 *
 * @param initial_capacity 初始容量，0表示使用默认值
 * @return 新创建的HashMap指针，失败返回NULL
 */
HashMap* hashmap_create(size_t initial_capacity);

/**
 * 销毁HashMap并释放所有内存
 *
 * @param map 要销毁的HashMap指针
 */
void hashmap_destroy(HashMap* map);

/**
 * 插入或更新键值对
 *
 * @param map HashMap指针
 * @param key 键字符串
 * @param value 值字符串
 * @return 成功返回0，失败返回-1
 */
int hashmap_put(HashMap* map, const char* key, const char* value);

/**
 * 获取指定键的值
 *
 * @param map HashMap指针
 * @param key 键字符串
 * @return 找到返回值字符串，未找到返回NULL
 */
const char* hashmap_get(HashMap* map, const char* key);

/**
 * 删除指定键的键值对
 *
 * @param map HashMap指针
 * @param key 键字符串
 * @return 成功返回0，未找到返回-1
 */
int hashmap_remove(HashMap* map, const char* key);

/**
 * 检查是否包含指定键
 *
 * @param map HashMap指针
 * @param key 键字符串
 * @return 包含返回1，不包含返回0
 */
int hashmap_contains_key(HashMap* map, const char* key);

/**
 * 获取HashMap大小
 *
 * @param map HashMap指针
 * @return 元素数量
 */
size_t hashmap_size(HashMap* map);

/**
 * 检查HashMap是否为空
 *
 * @param map HashMap指针
 * @return 为空返回1，不为空返回0
 */
int hashmap_is_empty(HashMap* map);

/**
 * 清空HashMap中的所有元素
 *
 * @param map HashMap指针
 */
void hashmap_clear(HashMap* map);

// 迭代器函数

/**
 * 创建迭代器
 *
 * @param map HashMap指针
 * @return 迭代器指针，失败返回NULL
 */
HashMapIterator* hashmap_iterator_create(HashMap* map);

/**
 * 销毁迭代器
 *
 * @param iter 迭代器指针
 */
void hashmap_iterator_destroy(HashMapIterator* iter);

/**
 * 检查是否有下一个元素
 *
 * @param iter 迭代器指针
 * @return 有下一个返回1，没有返回0
 */
int hashmap_iterator_has_next(HashMapIterator* iter);

/**
 * 获取下一个键值对
 *
 * @param iter 迭代器指针
 * @param key 输出参数，键字符串指针
 * @param value 输出参数，值字符串指针
 * @return 成功返回0，失败返回-1
 */
int hashmap_iterator_next(HashMapIterator* iter, const char** key, const char** value);

// 工具函数

/**
 * 打印HashMap内容（调试用）
 *
 * @param map HashMap指针
 */
void hashmap_print(HashMap* map);

/**
 * 获取HashMap统计信息
 *
 * @param map HashMap指针
 * @param stats 输出统计信息的字符串缓冲区
 * @param buffer_size 缓冲区大小
 */
void hashmap_get_stats(HashMap* map, char* stats, size_t buffer_size);

// 便利宏定义

// 创建默认HashMap
#define HASHMAP_CREATE() hashmap_create(0)

// 安全获取值（带默认值）
#define HASHMAP_GET_OR_DEFAULT(map, key, default_value) \
    ({ const char* _val = hashmap_get(map, key); _val ? _val : default_value; })

// 检查操作结果
#define HASHMAP_CHECK_RESULT(result, operation) \
    do { \
        if ((result) != 0) { \
            fprintf(stderr, "HashMap operation failed: %s at %s:%d\n", \
                    operation, __FILE__, __LINE__); \
        } \
    } while(0)

// 遍历HashMap的宏
#define HASHMAP_FOREACH(map, key_var, value_var) \
    HashMapIterator* _iter = hashmap_iterator_create(map); \
    const char* key_var; \
    const char* value_var; \
    while (_iter && hashmap_iterator_has_next(_iter) && \
           hashmap_iterator_next(_iter, &key_var, &value_var) == 0)

// 结束遍历的宏
#define HASHMAP_FOREACH_END() \
    if (_iter) hashmap_iterator_destroy(_iter);

#ifdef __cplusplus
}
#endif

#endif //ESURFINGCLIENT_STATES_H