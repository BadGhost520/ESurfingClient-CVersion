#include "cipher/CipherInterface.h"
#include "cipher/KeyData.h"

#include "core/States.h"

#include "utils/log/Logger.h"

static cipher_interface_t* create_cipher_factory(const char* algo_id)
{
    if (!algo_id) return NULL;
    LOG_DEBUG("命中 %s", algo_id);


    // AES-CBC (legacy, external IV)
    if (strcmp(algo_id, "CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E") == 0)
    {
        return create_aes_cbc_cipher(
            key1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
            key2_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
            iv1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E
        );
    }
    // AES-ECB (Android old)
    if (strcmp(algo_id, "A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4") == 0)
    {
        return create_aes_ecb_cipher(
            key2_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4,
            key1_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4
        );
    }
    // 3DES-CBC (Android old)
    if (strcmp(algo_id, "5BFBA864-BBA9-42DB-8EAD-49B5F412BD81") == 0)
    {
        return create_desede_cbc_cipher(
            key2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            key1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            iv2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            iv1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81
        );
    }
    // 3DES-ECB (Android old)
    if (strcmp(algo_id, "6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5") == 0)
    {
        return create_desede_ecb_cipher(
            key1_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5,
            key2_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5
        );
    }
    // ZUC-128 (Android old)
    if (strcmp(algo_id, "B809531F-0007-4B5B-923B-4BD560398113") == 0)
    {
        return create_zuc_cipher(
            key_B809531F_0007_4B5B_923B_4BD560398113,
            iv_B809531F_0007_4B5B_923B_4BD560398113
        );
    }
    // SM4-CBC (Android old)
    if (strcmp(algo_id, "F3974434-C0DD-4C20-9E87-DDB6814A1C48") == 0)
    {
        return create_sm4_cbc_cipher(
            key_F3974434_C0DD_4C20_9E87_DDB6814A1C48,
            iv_F3974434_C0DD_4C20_9E87_DDB6814A1C48
        );
    }
    // SM4-ECB (Android old)
    if (strcmp(algo_id, "ED382482-F72C-4C41-A76D-28EEA0F1F2AF") == 0)
    {
        return create_sm4_ecb_cipher(
            key_ED382482_F72C_4C41_A76D_28EEA0F1F2AF
        );
    }
    // ModXTEA (Android old, no key endian swap)
    if (strcmp(algo_id, "B3047D4E-67DF-4864-A6A5-DF9B9E525C79") == 0)
    {
        return create_mod_xtea_cipher(
            key1_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
            key2_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
            key3_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
            false
        );
    }
    // ModXTEA-IV (Android old)
    if (strcmp(algo_id, "C32C68F9-CA81-4260-A329-BBAFD1A9CCD1") == 0)
    {
        return create_mod_xtea_iv_cipher(
            key1_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            key2_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            key3_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            iv_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1
        );
    }

    // 3DES-CBC (Linux)
    if (strcmp(algo_id, "1A7343EC-7F9B-4570-BF58-16279A81116B") == 0)
    {
        return create_desede_cbc_cipher(
            key1_1A7343EC_7F9B_4570_BF58_16279A81116B,
            key2_1A7343EC_7F9B_4570_BF58_16279A81116B,
            iv1_1A7343EC_7F9B_4570_BF58_16279A81116B,
            iv2_1A7343EC_7F9B_4570_BF58_16279A81116B
        );
    }
    // AES-ECB (Linux)
    if (strcmp(algo_id, "4BA5496A-2123-46A7-85F2-35956EA7BE39") == 0)
    {
        return create_aes_ecb_cipher(
            key1_4BA5496A_2123_46A7_85F2_35956EA7BE39,
            key2_4BA5496A_2123_46A7_85F2_35956EA7BE39
        );
    }
    // AES-CBC (zero IV)
    if (strcmp(algo_id, "45433DCF-9ECA-4BE5-83F2-F92BA0B4F291") == 0)
    {
        return create_aes_cbc_cipher(
            key1_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291,
            key2_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291,
            NULL
        );
    }
    // XTEA (Linux, key endian swap)
    if (strcmp(algo_id, "60639D8B-272E-4A4D-976E-AA270987A169") == 0)
    {
        return create_mod_xtea_cipher(
            key1_60639D8B_272E_4A4D_976E_AA270987A169,
            key2_60639D8B_272E_4A4D_976E_AA270987A169,
            key3_60639D8B_272E_4A4D_976E_AA270987A169,
            true
        );
    }
    // MOD_XTEA_CBC (Linux)
    if (strcmp(algo_id, "AB6C8EBE-B8F8-4C08-8222-69A3B5E86A91") == 0)
    {
        return create_mod_xtea_cbc_triple_cipher(
            key1_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            key2_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            key3_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            iv_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91
        );
    }
    // DES-ECB (Linux)
    if (strcmp(algo_id, "B306E770-B7D5-49F2-A574-BCE2C5C650ED") == 0)
    {
        return create_des_ecb_six_keys_cipher(
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
        return create_snow3g_variant_cipher(
            key_07E824B2_9E5C_4D1B_BBB0_5E07C251E4AA,
            iv_07E824B2_9E5C_4D1B_BBB0_5E07C251E4AA
        );
    }
    // Triple modified-TEA ECB (Android new)
    if (strcmp(algo_id, "319FC5AB-EC0E-46B9-A252-2285F9DAE813") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_319FC5AB_EC0E_46B9_A252_2285F9DAE813
        );
    }
    // SM4-variant CBC (Android new)
    if (strcmp(algo_id, "D6544CFE-F2DE-459B-9B77-0F2B367EF169") == 0)
    {
        return create_sm4_variant_cbc_cipher(
            key_D6544CFE_F2DE_459B_9B77_0F2B367EF169,
            iv_D6544CFE_F2DE_459B_9B77_0F2B367EF169
        );
    }
    // SM4-variant ECB (Android new)
    if (strcmp(algo_id, "D755A536-B551-468C-BD87-322182B223D4") == 0)
    {
        return create_sm4_variant_ecb_cipher(
            key_D755A536_B551_468C_BD87_322182B223D4
        );
    }
    // double AES-128-CBC (Android new)
    if (strcmp(algo_id, "BB2EA626-590B-4C42-82BE-E052FCBBB88E") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_BB2EA626_590B_4C42_82BE_E052FCBBB88E,
            iv_BB2EA626_590B_4C42_82BE_E052FCBBB88E
        );
    }
    // double AES-128-ECB (Android new)
    if (strcmp(algo_id, "DEABB8C8-A2BC-48CA-8ED0-8CDF1BD62F61") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_DEABB8C8_A2BC_48CA_8ED0_8CDF1BD62F61
        );
    }
    // double 3DES-CBC (Android new)
    if (strcmp(algo_id, "9ABF4D29-34DB-4CE9-BB8C-7E371D637758") == 0)
    {
        return create_desede_double_cbc_cipher(
            key_9ABF4D29_34DB_4CE9_BB8C_7E371D637758,
            iv_9ABF4D29_34DB_4CE9_BB8C_7E371D637758
        );
    }
    // six-layer DES-ECB (Android new)
    if (strcmp(algo_id, "AD8BB5B0-0E72-4198-A362-96D52C1B7ED1") == 0)
    {
        return create_des_ecb_six_cipher(
            key_AD8BB5B0_0E72_4198_A362_96D52C1B7ED1
        );
    }
    // triple modified-TEA CBC (Android new)
    if (strcmp(algo_id, "35101415-A20F-4DFE-B00B-0B4F3B2F8C66") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_35101415_A20F_4DFE_B00B_0B4F3B2F8C66,
            iv_35101415_A20F_4DFE_B00B_0B4F3B2F8C66
        );
    }


    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "03F8A638-5C23-418B-972C-A2BA6927EF77") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_03F8A638_5C23_418B_972C_A2BA6927EF77,
            iv_03F8A638_5C23_418B_972C_A2BA6927EF77
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "054DDD03-911E-49F5-89D6-EFBF5055FBFF") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_054DDD03_911E_49F5_89D6_EFBF5055FBFF,
            iv_054DDD03_911E_49F5_89D6_EFBF5055FBFF
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "066474E5-503E-4B82-98C4-DF4483DAF0B5") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_066474E5_503E_4B82_98C4_DF4483DAF0B5,
            iv_066474E5_503E_4B82_98C4_DF4483DAF0B5
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "079637D7-A2A2-41CE-A50D-4CAD3B2334E7") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_079637D7_A2A2_41CE_A50D_4CAD3B2334E7,
            iv_079637D7_A2A2_41CE_A50D_4CAD3B2334E7
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "083B005A-7ACA-419A-AC00-6929C0AADB55") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_083B005A_7ACA_419A_AC00_6929C0AADB55,
            iv_083B005A_7ACA_419A_AC00_6929C0AADB55
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "08BDB042-5D25-4397-875F-357E9F7700C8") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_08BDB042_5D25_4397_875F_357E9F7700C8,
            iv_08BDB042_5D25_4397_875F_357E9F7700C8
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "0A2375CB-1F91-4064-B00F-1CF3A1AF6E4A") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_0A2375CB_1F91_4064_B00F_1CF3A1AF6E4A,
            iv_0A2375CB_1F91_4064_B00F_1CF3A1AF6E4A
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "11734889-14D8-48FA-ACEC-36452CA3FE8D") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_11734889_14D8_48FA_ACEC_36452CA3FE8D,
            iv_11734889_14D8_48FA_ACEC_36452CA3FE8D
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "1401F41B-06EA-4236-8E68-7AA1555A7C28") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_1401F41B_06EA_4236_8E68_7AA1555A7C28,
            iv_1401F41B_06EA_4236_8E68_7AA1555A7C28
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "1B652785-1153-4741-8A1C-2D9E752BE0E5") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_1B652785_1153_4741_8A1C_2D9E752BE0E5
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "2C2C4070-6644-4E84-BDBD-1760A2DA875C") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_2C2C4070_6644_4E84_BDBD_1760A2DA875C,
            iv_2C2C4070_6644_4E84_BDBD_1760A2DA875C
        );
    }
    // Dynamic ZSM (iOS, cdy 2)
    if (strcmp(algo_id, "2E75BFA5-76C5-4691-BBF5-A49C94908464") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_2E75BFA5_76C5_4691_BBF5_A49C94908464,
            iv_2E75BFA5_76C5_4691_BBF5_A49C94908464
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "37620C35-7A97-4A4F-9EF3-822DD38FE211") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_37620C35_7A97_4A4F_9EF3_822DD38FE211,
            iv_37620C35_7A97_4A4F_9EF3_822DD38FE211
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "376412D4-ACA3-4452-8A79-4589B3817760") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_376412D4_ACA3_4452_8A79_4589B3817760
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "42184E84-A844-486E-83C3-4CB9FD874D67") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_42184E84_A844_486E_83C3_4CB9FD874D67,
            iv_42184E84_A844_486E_83C3_4CB9FD874D67
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "466FBBBF-E526-4A13-A306-6AF402688F5E") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_466FBBBF_E526_4A13_A306_6AF402688F5E,
            iv_466FBBBF_E526_4A13_A306_6AF402688F5E
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "46D3C896-2B45-41B2-91B6-A5C9718EF7CD") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_46D3C896_2B45_41B2_91B6_A5C9718EF7CD,
            iv_46D3C896_2B45_41B2_91B6_A5C9718EF7CD
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "47C78640-9E7D-4115-B140-BE2DF6C49BF9") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_47C78640_9E7D_4115_B140_BE2DF6C49BF9,
            iv_47C78640_9E7D_4115_B140_BE2DF6C49BF9
        );
    }
    // Dynamic ZSM (iOS, cdy 5)
    if (strcmp(algo_id, "4D10D93E-5A68-46D4-83EA-49E7C4933C91") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_4D10D93E_5A68_46D4_83EA_49E7C4933C91
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "50495282-EDFD-4371-B4FE-6706326B2F12") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_50495282_EDFD_4371_B4FE_6706326B2F12,
            iv_50495282_EDFD_4371_B4FE_6706326B2F12
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "52271930-386B-4EC7-B5E9-7DEF7C1C11A8") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_52271930_386B_4EC7_B5E9_7DEF7C1C11A8,
            iv_52271930_386B_4EC7_B5E9_7DEF7C1C11A8
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "5678F2AA-C270-4F0A-9CB3-AE7A5D912FCD") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_5678F2AA_C270_4F0A_9CB3_AE7A5D912FCD,
            iv_5678F2AA_C270_4F0A_9CB3_AE7A5D912FCD
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "578FDB96-AF47-461D-AC36-D08EADF10130") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_578FDB96_AF47_461D_AC36_D08EADF10130,
            iv_578FDB96_AF47_461D_AC36_D08EADF10130
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "6067E3B6-3AB5-45AE-8CEC-E8DACB3C76DE") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_6067E3B6_3AB5_45AE_8CEC_E8DACB3C76DE,
            iv_6067E3B6_3AB5_45AE_8CEC_E8DACB3C76DE
        );
    }
    // Dynamic ZSM (macOS, cdy 1)
    if (strcmp(algo_id, "634D7C6F-581F-47CC-959C-D9265EDAB9B4") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_634D7C6F_581F_47CC_959C_D9265EDAB9B4
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "68361562-0864-4EB3-8508-9CDE34269D8C") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_68361562_0864_4EB3_8508_9CDE34269D8C
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "70CDBBAB-C9C9-49DE-A8D9-068579EAFFD0") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_70CDBBAB_C9C9_49DE_A8D9_068579EAFFD0,
            iv_70CDBBAB_C9C9_49DE_A8D9_068579EAFFD0
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "76CD0763-8C5F-49B0-85CD-549BB32886A2") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_76CD0763_8C5F_49B0_85CD_549BB32886A2
        );
    }
    // Dynamic ZSM (macOS, cdy 5)
    if (strcmp(algo_id, "7781847B-446E-4CF7-A418-79989EFA1B00") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_7781847B_446E_4CF7_A418_79989EFA1B00
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "7809936E-EA7E-4BA2-AD3B-5621259530FC") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_7809936E_EA7E_4BA2_AD3B_5621259530FC,
            iv_7809936E_EA7E_4BA2_AD3B_5621259530FC
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "78AD4214-E71F-421A-A13B-C65EC32890C3") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_78AD4214_E71F_421A_A13B_C65EC32890C3,
            iv_78AD4214_E71F_421A_A13B_C65EC32890C3
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "79F1CC4F-A213-4DE0-AAED-A276D2E1F21F") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_79F1CC4F_A213_4DE0_AAED_A276D2E1F21F
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "7DE2123D-3868-4D5E-86A1-687FE2683476") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_7DE2123D_3868_4D5E_86A1_687FE2683476,
            iv_7DE2123D_3868_4D5E_86A1_687FE2683476
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "7EDCC3AB-5B1B-409A-A5E3-E9623A5EC1B7") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_7EDCC3AB_5B1B_409A_A5E3_E9623A5EC1B7
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "895E36A7-0175-4EF2-A00D-7F3482D70D72") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_895E36A7_0175_4EF2_A00D_7F3482D70D72
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "9E6620CB-628D-49CA-80DD-9E6BCA401A31") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_9E6620CB_628D_49CA_80DD_9E6BCA401A31
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "ABA03E9C-E7E3-4AD0-94F9-4224DAA975C4") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_ABA03E9C_E7E3_4AD0_94F9_4224DAA975C4,
            iv_ABA03E9C_E7E3_4AD0_94F9_4224DAA975C4
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "AF69A651-96D0-4B88-B388-644CB6822159") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_AF69A651_96D0_4B88_B388_644CB6822159
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "B453C03E-871D-4B13-8AD1-EB61162F2EED") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_B453C03E_871D_4B13_8AD1_EB61162F2EED
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "BFAC1582-AA39-4CBC-AD0E-6D37ACDA369B") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_BFAC1582_AA39_4CBC_AD0E_6D37ACDA369B
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "CA582532-4E58-448A-976B-4E1E9809966B") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_CA582532_4E58_448A_976B_4E1E9809966B,
            iv_CA582532_4E58_448A_976B_4E1E9809966B
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "CAC870CA-C03D-475A-8CB0-F4E9C80B9153") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_CAC870CA_C03D_475A_8CB0_F4E9C80B9153,
            iv_CAC870CA_C03D_475A_8CB0_F4E9C80B9153
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "CF750526-3D99-44BE-A0DE-09DEADC97D52") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_CF750526_3D99_44BE_A0DE_09DEADC97D52,
            iv_CF750526_3D99_44BE_A0DE_09DEADC97D52
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "DDE9BF30-FBD3-4184-9B38-8855C1F47FD1") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_DDE9BF30_FBD3_4184_9B38_8855C1F47FD1,
            iv_DDE9BF30_FBD3_4184_9B38_8855C1F47FD1
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "E2BC8BAC-D7E0-4DAD-9400-789C63D6A0DC") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_E2BC8BAC_D7E0_4DAD_9400_789C63D6A0DC
        );
    }
    // Dynamic ZSM (iOS, cdy 1)
    if (strcmp(algo_id, "E2F8C4B2-2742-4690-9BD5-BBAB926FB9E8") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_E2F8C4B2_2742_4690_9BD5_BBAB926FB9E8
        );
    }
    // Dynamic ZSM (macOS, cdy 2)
    if (strcmp(algo_id, "E366503B-B6E5-40A5-BCE1-39001D18D90E") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_E366503B_B6E5_40A5_BCE1_39001D18D90E,
            iv_E366503B_B6E5_40A5_BCE1_39001D18D90E
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "E9747EF1-6849-4F87-957B-3B93E6A90393") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_E9747EF1_6849_4F87_957B_3B93E6A90393
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "EB353E96-94B2-486C-9FA6-31A37C222EA7") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_EB353E96_94B2_486C_9FA6_31A37C222EA7
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "EBD34028-0E59-4B21-86EB-CF892E75F6FF") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_EBD34028_0E59_4B21_86EB_CF892E75F6FF,
            iv_EBD34028_0E59_4B21_86EB_CF892E75F6FF
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "ECE4A1E8-A530-48B2-B968-F503CB1CBAAC") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_ECE4A1E8_A530_48B2_B968_F503CB1CBAAC,
            iv_ECE4A1E8_A530_48B2_B968_F503CB1CBAAC
        );
    }
    // Dynamic ZSM (Windows, cdy 2)
    if (strcmp(algo_id, "EE0D4296-FC76-4BBB-8E0D-B03DB50776F2") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_EE0D4296_FC76_4BBB_8E0D_B03DB50776F2,
            iv_EE0D4296_FC76_4BBB_8E0D_B03DB50776F2
        );
    }
    // Dynamic ZSM (Windows, cdy 5)
    if (strcmp(algo_id, "F1C01068-FFAB-4E4E-A043-0B06B2299AF4") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_F1C01068_FFAB_4E4E_A043_0B06B2299AF4
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "F6B5CFB0-17FD-453C-9791-B2887E2C9859") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_F6B5CFB0_17FD_453C_9791_B2887E2C9859,
            iv_F6B5CFB0_17FD_453C_9791_B2887E2C9859
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "FC05D786-59A7-4469-B276-0D9B89EAD057") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_FC05D786_59A7_4469_B276_0D9B89EAD057,
            iv_FC05D786_59A7_4469_B276_0D9B89EAD057
        );
    }
    // Dynamic ZSM (Windows, cdy 6)
    if (strcmp(algo_id, "FEE5E4DA-50A0-4ECD-8392-92E09CB1B775") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_FEE5E4DA_50A0_4ECD_8392_92E09CB1B775,
            iv_FEE5E4DA_50A0_4ECD_8392_92E09CB1B775
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
    if (strstr(text, "userid") == NULL)
    {
        LOG_VERBOSE("要加密的文本:\n%s", text);
    }
    cipher_interface_t* cipher = g_prog_status[tl_thread_idx].auth_cfg.cipher;
    return cipher->encrypt(cipher, text);
}

char* session_decrypt(const char* text)
{
    LOG_VERBOSE("要解密的文本:\n%s", text);
    cipher_interface_t* cipher = g_prog_status[tl_thread_idx].auth_cfg.cipher;
    return cipher->decrypt(cipher, text);
}
