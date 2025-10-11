#include "../headFiles/cipher/cipher_interface.h"
#include "../headFiles/cipher/key_data.h"
#include <stdlib.h>
#include <string.h>

#include "../headFiles/utils/Logger.h"

/**
 * 加解密工厂实现 - 对应Kotlin的CipherFactory
 * 
 * 根据算法ID（UUID）创建对应的加解密实例
 */

// 各算法实现的创建函数声明
extern cipher_interface_t* create_aes_cbc_cipher(const uint8_t* key1, const uint8_t* key2,
                                                 const uint8_t* iv1, const uint8_t* iv2);
extern cipher_interface_t* create_aes_ecb_cipher(const uint8_t* key1, const uint8_t* key2);
extern cipher_interface_t* create_aes_ecb_pc_cipher(const uint8_t* key1, const uint8_t* key2);
extern cipher_interface_t* create_aes_cbc_pc_cipher(const uint8_t* key1, const uint8_t* key2);
extern cipher_interface_t* create_desede_cbc_cipher(const uint8_t* key1, const uint8_t* key2,
                                                    const uint8_t* iv1, const uint8_t* iv2);
extern cipher_interface_t* create_desede_ecb_cipher(const uint8_t* key1, const uint8_t* key2);
extern cipher_interface_t* create_zuc_cipher(const uint8_t* key, const uint8_t* iv);
extern cipher_interface_t* create_sm4_cbc_cipher(const uint8_t* key, const uint8_t* iv);
extern cipher_interface_t* create_sm4_ecb_cipher(const uint8_t* key);
extern cipher_interface_t* create_mod_xtea_cipher(const uint32_t* key1, const uint32_t* key2, 
                                                  const uint32_t* key3);
extern cipher_interface_t* create_mod_xtea_iv_cipher(const uint32_t* key1, const uint32_t* key2, 
                                                     const uint32_t* key3, const uint32_t* iv);
extern cipher_interface_t* create_mod_xtea_pc_cipher(const uint32_t* key1, const uint32_t* key2,
                                                     const uint32_t* key3);
extern cipher_interface_t* create_desede_cbc_pc_cipher(const uint8_t* key1, const uint8_t* key2,
                                                       const uint8_t* iv1, const uint8_t* iv2);
cipher_interface_t* cipher_factory_create(const char* algorithm_id) {
    if (!algorithm_id) {
        return NULL;
    }

    // 2222222
    // AES-CBC
    if (strcmp(algorithm_id, "CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E") == 0) {
        LOG_DEBUG("Hit CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E");
        return create_aes_cbc_cipher(
            key1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
            key2_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
            iv1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
            iv2_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E
        );
    }

    // 2222222
    // AES-ECB
    if (strcmp(algorithm_id, "A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4") == 0) {
        LOG_DEBUG("Hit A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4");
        return create_aes_ecb_cipher(
            key1_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4,
            key2_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4
        );
    }

    // 22222222
    // 3DES-CBC
    if (strcmp(algorithm_id, "5BFBA864-BBA9-42DB-8EAD-49B5F412BD81") == 0) {
        LOG_DEBUG("Hit 5BFBA864-BBA9-42DB-8EAD-49B5F412BD81");
        return create_desede_cbc_cipher(
            key1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            key2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            iv1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            iv2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81
        );
    }

    // 22222222
    // 3DES-ECB
    if (strcmp(algorithm_id, "6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5") == 0) {
        LOG_DEBUG("Hit 6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5");
        return create_desede_ecb_cipher(
            key1_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5,
            key2_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5
        );
    }

    // 1111111
    // ZUC-128
    if (strcmp(algorithm_id, "B809531F-0007-4B5B-923B-4BD560398113") == 0) {
        LOG_DEBUG("Hit B809531F-0007-4B5B-923B-4BD560398113");
        return create_zuc_cipher(
            key_B809531F_0007_4B5B_923B_4BD560398113,
            iv_B809531F_0007_4B5B_923B_4BD560398113
        );
    }

    // 1111111
    // SM4-CBC
    if (strcmp(algorithm_id, "F3974434-C0DD-4C20-9E87-DDB6814A1C48") == 0) {
        LOG_DEBUG("Hit F3974434-C0DD-4C20-9E87-DDB6814A1C48");
        return create_sm4_cbc_cipher(
            key_F3974434_C0DD_4C20_9E87_DDB6814A1C48,
            iv_F3974434_C0DD_4C20_9E87_DDB6814A1C48
        );
    }

    // 2222222
    // SM4-ECB
    if (strcmp(algorithm_id, "ED382482-F72C-4C41-A76D-28EEA0F1F2AF") == 0) {
        LOG_DEBUG("Hit ED382482-F72C-4C41-A76D-28EEA0F1F2AF");
        return create_sm4_ecb_cipher(
            key_ED382482_F72C_4C41_A76D_28EEA0F1F2AF
        );
    }

    // 2222222
    // ModXTEA
    if (strcmp(algorithm_id, "B3047D4E-67DF-4864-A6A5-DF9B9E525C79") == 0) {
        LOG_DEBUG("Hit B3047D4E-67DF-4864-A6A5-DF9B9E525C79");
        return create_mod_xtea_cipher(
            key1_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
            key2_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
            key3_B3047D4E_67DF_4864_A6A5_DF9B9E525C79
        );
    }

    // 222222222
    // ModXTEAIV
    if (strcmp(algorithm_id, "C32C68F9-CA81-4260-A329-BBAFD1A9CCD1") == 0) {
        LOG_DEBUG("Hit C32C68F9-CA81-4260-A329-BBAFD1A9CCD1");
        return create_mod_xtea_iv_cipher(
            key1_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            key2_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            key3_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            iv_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1
        );
    }

    // 自实现 3DES-CBC 两层（PC）
    if (strcmp(algorithm_id, "1A7343EC-7F9B-4570-BF58-16279A81116B") == 0) {
        LOG_DEBUG("Hit 1A7343EC-7F9B-4570-BF58-16279A81116B");
        return create_desede_cbc_pc_cipher(
            key1_1A7343EC_7F9B_4570_BF58_16279A81116B,
            key2_1A7343EC_7F9B_4570_BF58_16279A81116B,
            iv1_1A7343EC_7F9B_4570_BF58_16279A81116B,
            iv2_1A7343EC_7F9B_4570_BF58_16279A81116B
        );
    }

    // AES-ECB 两层（PC）
    if (strcmp(algorithm_id, "4BA5496A-2123-46A7-85F2-35956EA7BE39") == 0) {
        LOG_DEBUG("Hit 4BA5496A-2123-46A7-85F2-35956EA7BE39");
        return create_aes_ecb_pc_cipher(
            key1_4BA5496A_2123_46A7_85F2_35956EA7BE39,
            key2_4BA5496A_2123_46A7_85F2_35956EA7BE39
        );
    }
    
    // AES-CBC 两层（PC，密文带双层IV）
    if (strcmp(algorithm_id, "45433DCF-9ECA-4BE5-83F2-F92BA0B4F291") == 0) {
        LOG_DEBUG("Hit 45433DCF-9ECA-4BE5-83F2-F92BA0B4F291");
        return create_aes_cbc_pc_cipher(
            key1_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291,
            key2_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291
        );
    }
    
    // XTEA 三层（PC变体）60639D8B-272E-4A4D-976E-AA270987A169
    if (strcmp(algorithm_id, "60639D8B-272E-4A4D-976E-AA270987A169") == 0) {
        LOG_DEBUG("Hit 60639D8B-272E-4A4D-976E-AA270987A169");
        return create_mod_xtea_pc_cipher(
            key1_60639D8B_272E_4A4D_976E_AA270987A169,
            key2_60639D8B_272E_4A4D_976E_AA270987A169,
            key3_60639D8B_272E_4A4D_976E_AA270987A169
        );
    }
    return NULL;
}

/**
 * 销毁加解密实例
 */
void cipher_factory_destroy(cipher_interface_t* cipher) {
    if (cipher && cipher->destroy) {
        cipher->destroy(cipher);
    }
}