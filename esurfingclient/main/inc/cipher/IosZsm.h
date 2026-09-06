#ifndef ESURFINGCLIENT_IOSZSM_H
#define ESURFINGCLIENT_IOSZSM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * 从 iOS PacketTunnel ZSM 解包密钥并初始化加解密工厂.
 * ZSM 不是 Android/Linux 那种 UUID→硬编码密钥表, 正文是 TEA + LZMA 后的 JS,
 * 密钥在 cdckey/cdciv, 算法由 JS 的 var codex = 0xNN 或 cdy(type, ...) 决定.
 *
 * @param data ZSM 原始字节
 * @param length ZSM 长度
 * @param algo_id_out 可选, 写入头部 str2 的 Algo-ID (大写 UUID)
 * @return 是否成功
 */
bool init_ios_cipher_from_zsm(const uint8_t* data, size_t length, char* algo_id_out);

/**
 * 判断 ticket 响应是否为 PacketTunnel IZsmModLoad 动态模块.
 * 头部是两个 Pascal 字符串, 随后 LZMA packed type nibble == 2.
 * 这种 ZSM 的 AID 不在 Android/Linux CipherFactory 里.
 */
bool looks_like_ios_zsm(const uint8_t* data, size_t length);

#endif
