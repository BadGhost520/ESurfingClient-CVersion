#include "cipher/CipherInterface.h"
#include "cipher/KeyData.h"
#include "utils/Logger.h"
#include "States.h"

#include <stdlib.h>
#include <string.h>

static cipher_interface_t* create_cipher_factory(const char* algo_id)
{
    if (!algo_id) return NULL;
    // AES-CBC (Android old)(已弃用)
    // if (strcmp(algo_id, "CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E") == 0)
    // {
    //     LOG_DEBUG("命中 CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E");
    //     return create_aes_cbc_cipher(
    //         key1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
    //         key2_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
    //         iv1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E
    //     );
    // }
    // AES-ECB (Android old)(已弃用)
    // if (strcmp(algo_id, "A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4") == 0)
    // {
    //     LOG_DEBUG("命中 A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4");
    //     return create_aes_ecb_cipher(
    //         key1_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4,
    //         key2_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4
    //     );
    // }
    // 3DES-CBC (Android old)(已弃用)
    // if (strcmp(algo_id, "5BFBA864-BBA9-42DB-8EAD-49B5F412BD81") == 0)
    // {
    //     LOG_DEBUG("命中 5BFBA864-BBA9-42DB-8EAD-49B5F412BD81");
    //     return create_desede_cbc_cipher(
    //         key1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
    //         key2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
    //         iv1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
    //         iv2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81
    //     );
    // }
    // 3DES-ECB (Android old)(已弃用)
    // if (strcmp(algo_id, "6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5") == 0)
    // {
    //     LOG_DEBUG("命中 6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5");
    //     return create_desede_ecb_cipher(
    //         key1_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5,
    //         key2_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5
    //     );
    // }
    // ZUC-128 (Android old)(已弃用)
    // if (strcmp(algo_id, "B809531F-0007-4B5B-923B-4BD560398113") == 0)
    // {
    //     LOG_DEBUG("命中 B809531F-0007-4B5B-923B-4BD560398113");
    //     return create_zuc_cipher(
    //         key_B809531F_0007_4B5B_923B_4BD560398113,
    //         iv_B809531F_0007_4B5B_923B_4BD560398113
    //     );
    // }
    // SM4-CBC (Android old)(已弃用)
    // if (strcmp(algo_id, "F3974434-C0DD-4C20-9E87-DDB6814A1C48") == 0)
    // {
    //     LOG_DEBUG("命中 F3974434-C0DD-4C20-9E87-DDB6814A1C48");
    //     return create_sm4_cbc_cipher(
    //         key_F3974434_C0DD_4C20_9E87_DDB6814A1C48,
    //         iv_F3974434_C0DD_4C20_9E87_DDB6814A1C48
    //     );
    // }
    // SM4-ECB (Android old)(已弃用)
    // if (strcmp(algo_id, "ED382482-F72C-4C41-A76D-28EEA0F1F2AF") == 0)
    // {
    //     LOG_DEBUG("命中 ED382482-F72C-4C41-A76D-28EEA0F1F2AF");
    //     return create_sm4_ecb_cipher(
    //         key_ED382482_F72C_4C41_A76D_28EEA0F1F2AF
    //     );
    // }
    // ModXTEA (Android old)(已弃用)
    // if (strcmp(algo_id, "B3047D4E-67DF-4864-A6A5-DF9B9E525C79") == 0)
    // {
    //     LOG_DEBUG("命中 B3047D4E-67DF-4864-A6A5-DF9B9E525C79");
    //     return create_mod_xtea_cipher(
    //         key1_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
    //         key2_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
    //         key3_B3047D4E_67DF_4864_A6A5_DF9B9E525C79
    //     );
    // }
    // ModXTEAIV (Android old)(已弃用)
    // if (strcmp(algo_id, "C32C68F9-CA81-4260-A329-BBAFD1A9CCD1") == 0)
    // {
    //     LOG_DEBUG("命中 C32C68F9-CA81-4260-A329-BBAFD1A9CCD1");
    //     return create_mod_xtea_iv_cipher(
    //         key1_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
    //         key2_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
    //         key3_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
    //         iv_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1
    //     );
    // }
    // 3DES-CBC (Linux)
    if (strcmp(algo_id, "1A7343EC-7F9B-4570-BF58-16279A81116B") == 0)
    {
        LOG_DEBUG("命中 1A7343EC-7F9B-4570-BF58-16279A81116B");
        return create_desede_cbc_linux_cipher(
            key1_1A7343EC_7F9B_4570_BF58_16279A81116B,
            key2_1A7343EC_7F9B_4570_BF58_16279A81116B,
            iv1_1A7343EC_7F9B_4570_BF58_16279A81116B,
            iv2_1A7343EC_7F9B_4570_BF58_16279A81116B
        );
    }
    // AES-ECB (Linux)
    if (strcmp(algo_id, "4BA5496A-2123-46A7-85F2-35956EA7BE39") == 0)
    {
        LOG_DEBUG("命中 4BA5496A-2123-46A7-85F2-35956EA7BE39");
        return create_aes_ecb_linux_cipher(
            key1_4BA5496A_2123_46A7_85F2_35956EA7BE39,
            key2_4BA5496A_2123_46A7_85F2_35956EA7BE39
        );
    }
    // AES-CBC (Linux)
    if (strcmp(algo_id, "45433DCF-9ECA-4BE5-83F2-F92BA0B4F291") == 0)
    {
        LOG_DEBUG("命中 45433DCF-9ECA-4BE5-83F2-F92BA0B4F291");
        return create_aes_cbc_linux_cipher(
            key1_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291,
            key2_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291,
            iv_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291
        );
    }
    // XTEA (Linux)
    if (strcmp(algo_id, "60639D8B-272E-4A4D-976E-AA270987A169") == 0)
    {
        LOG_DEBUG("命中 60639D8B-272E-4A4D-976E-AA270987A169");
        return create_mod_xtea_linux_cipher(
            key1_60639D8B_272E_4A4D_976E_AA270987A169,
            key2_60639D8B_272E_4A4D_976E_AA270987A169,
            key3_60639D8B_272E_4A4D_976E_AA270987A169
        );
    }
    // MOD_XTEA_CBC (Linux)
    if (strcmp(algo_id, "AB6C8EBE-B8F8-4C08-8222-69A3B5E86A91") == 0)
    {
        LOG_DEBUG("命中 AB6C8EBE-B8F8-4C08-8222-69A3B5E86A91");
        return create_ab6c8_linux_cipher(
            key1_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            key2_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            key3_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            iv_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91
        );
    }
    // DES-ECB (Linux)
    if (strcmp(algo_id, "B306E770-B7D5-49F2-A574-BCE2C5C650ED") == 0)
    {
        LOG_DEBUG("命中 B306E770-B7D5-49F2-A574-BCE2C5C650ED");
        return create_des_ecb_six_linux_cipher(
            key1_B306E770_B7D5_49F2_A574_BCE2C5C650ED,
            key2_B306E770_B7D5_49F2_A574_BCE2C5C650ED,
            key3_B306E770_B7D5_49F2_A574_BCE2C5C650ED,
            key4_B306E770_B7D5_49F2_A574_BCE2C5C650ED,
            key5_B306E770_B7D5_49F2_A574_BCE2C5C650ED,
            key6_B306E770_B7D5_49F2_A574_BCE2C5C650ED
        );
    }
    // SNOW3G-variant (Android new)
    if (strcmp(algo_id, "07E824B2-9E5C-4D1B-BBB0-5E07C251E4AA") == 0)
    {
        LOG_DEBUG("命中 07E824B2-9E5C-4D1B-BBB0-5E07C251E4AA");
        return create_snow3g_variant_android_cipher(
            key_07E824B2_9E5C_4D1B_BBB0_5E07C251E4AA,
            iv_07E824B2_9E5C_4D1B_BBB0_5E07C251E4AA
        );
    }
    // Triple modified-TEA ECB (Android new)
    if (strcmp(algo_id, "319FC5AB-EC0E-46B9-A252-2285F9DAE813") == 0)
    {
        LOG_DEBUG("命中 319FC5AB-EC0E-46B9-A252-2285F9DAE813");
        return create_tea_triple_ecb_android_cipher(
            key_319FC5AB_EC0E_46B9_A252_2285F9DAE813
        );
    }
    // SM4-variant CBC (Android new)
    if (strcmp(algo_id, "D6544CFE-F2DE-459B-9B77-0F2B367EF169") == 0)
    {
        LOG_DEBUG("命中 D6544CFE-F2DE-459B-9B77-0F2B367EF169");
        return create_sm4_variant_cbc_android_cipher(
            key_D6544CFE_F2DE_459B_9B77_0F2B367EF169,
            iv_D6544CFE_F2DE_459B_9B77_0F2B367EF169
        );
    }
    // SM4-variant ECB (Android new)
    if (strcmp(algo_id, "D755A536-B551-468C-BD87-322182B223D4") == 0)
    {
        LOG_DEBUG("命中 D755A536-B551-468C-BD87-322182B223D4");
        return create_sm4_variant_ecb_android_cipher(
            key_D755A536_B551_468C_BD87_322182B223D4
        );
    }
    // double AES-128-CBC (Android new)
    if (strcmp(algo_id, "BB2EA626-590B-4C42-82BE-E052FCBBB88E") == 0)
    {
        LOG_DEBUG("命中 BB2EA626-590B-4C42-82BE-E052FCBBB88E");
        return create_aes_double_cbc_android_cipher(
            key_BB2EA626_590B_4C42_82BE_E052FCBBB88E,
            iv_BB2EA626_590B_4C42_82BE_E052FCBBB88E
        );
    }
    // double AES-128-ECB (Android new)
    if (strcmp(algo_id, "DEABB8C8-A2BC-48CA-8ED0-8CDF1BD62F61") == 0)
    {
        LOG_DEBUG("命中 DEABB8C8-A2BC-48CA-8ED0-8CDF1BD62F61");
        return create_aes_double_ecb_android_cipher(
            key_DEABB8C8_A2BC_48CA_8ED0_8CDF1BD62F61
        );
    }
    // double 3DES-CBC (Android new)
    if (strcmp(algo_id, "9ABF4D29-34DB-4CE9-BB8C-7E371D637758") == 0)
    {
        LOG_DEBUG("命中 9ABF4D29-34DB-4CE9-BB8C-7E371D637758");
        return create_desede_double_cbc_android_cipher(
            key_9ABF4D29_34DB_4CE9_BB8C_7E371D637758,
            iv_9ABF4D29_34DB_4CE9_BB8C_7E371D637758
        );
    }
    // six-layer DES-ECB (Android new)
    if (strcmp(algo_id, "AD8BB5B0-0E72-4198-A362-96D52C1B7ED1") == 0)
    {
        LOG_DEBUG("命中 AD8BB5B0-0E72-4198-A362-96D52C1B7ED1");
        return create_des_ecb_six_android_cipher(
            key_AD8BB5B0_0E72_4198_A362_96D52C1B7ED1
        );
    }
    // triple modified-TEA CBC (Android new)
    if (strcmp(algo_id, "35101415-A20F-4DFE-B00B-0B4F3B2F8C66") == 0)
    {
        LOG_DEBUG("命中 35101415-A20F-4DFE-B00B-0B4F3B2F8C66");
        return create_tea_triple_cbc_android_cipher(
            key_35101415_A20F_4DFE_B00B_0B4F3B2F8C66,
            iv_35101415_A20F_4DFE_B00B_0B4F3B2F8C66
        );
    }

    // triple modified-TEA CBC (Windows)
    if (strcmp(algo_id, "03F8A638-5C23-418B-972C-A2BA6927EF77") == 0)
    {
        LOG_DEBUG("命中 03F8A638-5C23-418B-972C-A2BA6927EF77");
        return create_xtea_triple_cbc_windows_cipher(
            key_03F8A638_5C23_418B_972C_A2BA6927EF77,
            iv_03F8A638_5C23_418B_972C_A2BA6927EF77
        );
    }

    // 3DES-CBC (Windows)
    if (strcmp(algo_id, "054DDD03-911E-49F5-89D6-EFBF5055FBFF") == 0)
    {
        LOG_DEBUG("命中 054DDD03-911E-49F5-89D6-EFBF5055FBFF");
        return create_desede_cbc_linux_cipher(
            key_054DDD03_911E_49F5_89D6_EFBF5055FBFF,
            key_054DDD03_911E_49F5_89D6_EFBF5055FBFF + 24,
            iv_054DDD03_911E_49F5_89D6_EFBF5055FBFF,
            iv_054DDD03_911E_49F5_89D6_EFBF5055FBFF
        );
    }

    // AES-CBC (Windows)
    if (strcmp(algo_id, "066474E5-503E-4B82-98C4-DF4483DAF0B5") == 0)
    {
        LOG_DEBUG("命中 066474E5-503E-4B82-98C4-DF4483DAF0B5");
        return create_aes_cbc_linux_cipher(
            key1_066474E5_503E_4B82_98C4_DF4483DAF0B5,
            key2_066474E5_503E_4B82_98C4_DF4483DAF0B5,
            iv_066474E5_503E_4B82_98C4_DF4483DAF0B5
        );
    }

    // triple modified-TEA CBC (Windows)
    if (strcmp(algo_id, "079637D7-A2A2-41CE-A50D-4CAD3B2334E7") == 0)
    {
        LOG_DEBUG("命中 079637D7-A2A2-41CE-A50D-4CAD3B2334E7");
        return create_xtea_triple_cbc_windows_cipher(
            key_079637D7_A2A2_41CE_A50D_4CAD3B2334E7,
            iv_079637D7_A2A2_41CE_A50D_4CAD3B2334E7
        );
    }

    // AES-ECB (Windows)
    if (strcmp(algo_id, "083B005A-7ACA-419A-AC00-6929C0AADB55") == 0)
    {
        LOG_DEBUG("命中 083B005A-7ACA-419A-AC00-6929C0AADB55");
        return create_aes_ecb_linux_cipher(
            key1_083B005A_7ACA_419A_AC00_6929C0AADB55,
            key2_083B005A_7ACA_419A_AC00_6929C0AADB55
        );
    }

    // AES-ECB (Windows)
    if (strcmp(algo_id, "08BDB042-5D25-4397-875F-357E9F7700C8") == 0)
    {
        LOG_DEBUG("命中 08BDB042-5D25-4397-875F-357E9F7700C8");
        return create_aes_ecb_linux_cipher(
            key1_08BDB042_5D25_4397_875F_357E9F7700C8,
            key2_08BDB042_5D25_4397_875F_357E9F7700C8
        );
    }

    // triple modified-TEA CBC (Windows)
    if (strcmp(algo_id, "0A2375CB-1F91-4064-B00F-1CF3A1AF6E4A") == 0)
    {
        LOG_DEBUG("命中 0A2375CB-1F91-4064-B00F-1CF3A1AF6E4A");
        return create_xtea_triple_cbc_windows_cipher(
            key_0A2375CB_1F91_4064_B00F_1CF3A1AF6E4A,
            iv_0A2375CB_1F91_4064_B00F_1CF3A1AF6E4A
        );
    }

    // triple modified-TEA CBC (Windows)
    if (strcmp(algo_id, "11734889-14D8-48FA-ACEC-36452CA3FE8D") == 0)
    {
        LOG_DEBUG("命中 11734889-14D8-48FA-ACEC-36452CA3FE8D");
        return create_xtea_triple_cbc_windows_cipher(
            key_11734889_14D8_48FA_ACEC_36452CA3FE8D,
            iv_11734889_14D8_48FA_ACEC_36452CA3FE8D
        );
    }

    // triple modified-TEA CBC (Windows)
    if (strcmp(algo_id, "CF750526-3D99-44BE-A0DE-09DEADC97D52") == 0)
    {
        LOG_DEBUG("命中 CF750526-3D99-44BE-A0DE-09DEADC97D52");
        return create_xtea_triple_cbc_windows_cipher(
            key_CF750526_3D99_44BE_A0DE_09DEADC97D52,
            iv_CF750526_3D99_44BE_A0DE_09DEADC97D52
        );
    }

    // triple modified-TEA CBC (Windows)
    if (strcmp(algo_id, "FC05D786-59A7-4469-B276-0D9B89EAD057") == 0)
    {
        LOG_DEBUG("命中 FC05D786-59A7-4469-B276-0D9B89EAD057");
        return create_xtea_triple_cbc_windows_cipher(
            key_FC05D786_59A7_4469_B276_0D9B89EAD057,
            iv_FC05D786_59A7_4469_B276_0D9B89EAD057
        );
    }

    return NULL;
}

void destroy_cipher_factory()
{
    cipher_interface_t* cipher = g_prog_status[tl_thread_idx].auth_cfg.cipher;
    if (cipher == NULL)
    {
        LOG_DEBUG("cipher 已经是 NULL, 无需销毁");
        return;
    }
    if (cipher->destroy == NULL)
    {
        LOG_ERROR("cipher->destroy 为 NULL, 无法销毁");
        return;
    }
    LOG_DEBUG("销毁加解密工厂");
    cipher->destroy(cipher);
    g_prog_status[tl_thread_idx].auth_cfg.cipher = NULL;
    cipher = NULL;
    LOG_DEBUG("销毁完成");
}

bool init_cipher(const char* algo_id)
{
    LOG_DEBUG("开始初始化加解密工厂");
    cipher_interface_t* cipher = NULL;
    LOG_VERBOSE("创建加解密工厂, 使用 algo_id: %s", algo_id);
    cipher = create_cipher_factory(algo_id);
    if (cipher == NULL)
    {
        LOG_ERROR("初始化加密工厂失败");
        return false;
    }
    g_prog_status[tl_thread_idx].auth_cfg.cipher = cipher;
    LOG_DEBUG("初始化加解密工厂成功");
    return true;
}

char* session_encrypt(const char* text)
{
    LOG_VERBOSE("要加密的文本:\n%s", text);
    cipher_interface_t* cipher = g_prog_status[tl_thread_idx].auth_cfg.cipher;
    return cipher->encrypt(cipher, text);
}

char* session_decrypt(const char* text)
{
    LOG_VERBOSE("要解密的文本:\n%s", text);
    cipher_interface_t* cipher = g_prog_status[tl_thread_idx].auth_cfg.cipher;
    return cipher->decrypt(cipher, text);
}
