#include "cipher/CipherInterface.h"
#include "cipher/KeyData.h"

#include "core/States.h"

#include "utils/log/Logger.h"

/**
 * @brief 按 algo_id 造出对应的加解密器
 *
 * 这里故意保持成一串 if: 80 个算法分支分布在 20 个工厂函数上, 每个分支带的
 * key / iv 常量都不一样, 参数个数也不同 —— 做成"algo_id → 工厂"的表反而要为
 * 每个分支再写一个包装函数, 代码更多、还多一层间接。加算法时照下面抄一段即可。
 */
static cipher_interface_t* create_cipher_factory(const char* algo_id)
{
    if (!algo_id) return NULL;
    LOG_DEBUG("命中 %s", algo_id);

    /* --------------------------------------------------- Windows --------------------------------------------------- */
    // triple modified-TEA CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "03F8A638-5C23-418B-972C-A2BA6927EF77") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_03F8A638_5C23_418B_972C_A2BA6927EF77,
            iv_03F8A638_5C23_418B_972C_A2BA6927EF77
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "066474E5-503E-4B82-98C4-DF4483DAF0B5") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_066474E5_503E_4B82_98C4_DF4483DAF0B5,
            iv_066474E5_503E_4B82_98C4_DF4483DAF0B5
        );
    }
    // triple modified-TEA CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "079637D7-A2A2-41CE-A50D-4CAD3B2334E7") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_079637D7_A2A2_41CE_A50D_4CAD3B2334E7,
            iv_079637D7_A2A2_41CE_A50D_4CAD3B2334E7
        );
    }
    // triple modified-TEA CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "0A2375CB-1F91-4064-B00F-1CF3A1AF6E4A") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_0A2375CB_1F91_4064_B00F_1CF3A1AF6E4A,
            iv_0A2375CB_1F91_4064_B00F_1CF3A1AF6E4A
        );
    }
    // triple modified-TEA CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "11734889-14D8-48FA-ACEC-36452CA3FE8D") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_11734889_14D8_48FA_ACEC_36452CA3FE8D,
            iv_11734889_14D8_48FA_ACEC_36452CA3FE8D
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "1401F41B-06EA-4236-8E68-7AA1555A7C28") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_1401F41B_06EA_4236_8E68_7AA1555A7C28,
            iv_1401F41B_06EA_4236_8E68_7AA1555A7C28
        );
    }
    // triple modified-TEA CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "466FBBBF-E526-4A13-A306-6AF402688F5E") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_466FBBBF_E526_4A13_A306_6AF402688F5E,
            iv_466FBBBF_E526_4A13_A306_6AF402688F5E
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "46D3C896-2B45-41B2-91B6-A5C9718EF7CD") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_46D3C896_2B45_41B2_91B6_A5C9718EF7CD,
            iv_46D3C896_2B45_41B2_91B6_A5C9718EF7CD
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "47C78640-9E7D-4115-B140-BE2DF6C49BF9") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_47C78640_9E7D_4115_B140_BE2DF6C49BF9,
            iv_47C78640_9E7D_4115_B140_BE2DF6C49BF9
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "50495282-EDFD-4371-B4FE-6706326B2F12") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_50495282_EDFD_4371_B4FE_6706326B2F12,
            iv_50495282_EDFD_4371_B4FE_6706326B2F12
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "5678F2AA-C270-4F0A-9CB3-AE7A5D912FCD") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_5678F2AA_C270_4F0A_9CB3_AE7A5D912FCD,
            iv_5678F2AA_C270_4F0A_9CB3_AE7A5D912FCD
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "6067E3B6-3AB5-45AE-8CEC-E8DACB3C76DE") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_6067E3B6_3AB5_45AE_8CEC_E8DACB3C76DE,
            iv_6067E3B6_3AB5_45AE_8CEC_E8DACB3C76DE
        );
    }
    // Triple modified-TEA ECB (Windows, 14.146.227.141)
    if (strcmp(algo_id, "68361562-0864-4EB3-8508-9CDE34269D8C") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_68361562_0864_4EB3_8508_9CDE34269D8C
        );
    }
    // triple modified-TEA CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "7809936E-EA7E-4BA2-AD3B-5621259530FC") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_7809936E_EA7E_4BA2_AD3B_5621259530FC,
            iv_7809936E_EA7E_4BA2_AD3B_5621259530FC
        );
    }
    // Triple modified-TEA ECB (Windows, 14.146.227.141)
    if (strcmp(algo_id, "7EDCC3AB-5B1B-409A-A5E3-E9623A5EC1B7") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_7EDCC3AB_5B1B_409A_A5E3_E9623A5EC1B7
        );
    }
    // Triple modified-TEA ECB (Windows, 14.146.227.141)
    if (strcmp(algo_id, "B453C03E-871D-4B13-8AD1-EB61162F2EED") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_B453C03E_871D_4B13_8AD1_EB61162F2EED
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "CA582532-4E58-448A-976B-4E1E9809966B") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_CA582532_4E58_448A_976B_4E1E9809966B,
            iv_CA582532_4E58_448A_976B_4E1E9809966B
        );
    }
    // triple modified-TEA CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "CF750526-3D99-44BE-A0DE-09DEADC97D52") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_CF750526_3D99_44BE_A0DE_09DEADC97D52,
            iv_CF750526_3D99_44BE_A0DE_09DEADC97D52
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "DDE9BF30-FBD3-4184-9B38-8855C1F47FD1") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_DDE9BF30_FBD3_4184_9B38_8855C1F47FD1,
            iv_DDE9BF30_FBD3_4184_9B38_8855C1F47FD1
        );
    }
    // Triple modified-TEA ECB (Windows, 14.146.227.141)
    if (strcmp(algo_id, "EB353E96-94B2-486C-9FA6-31A37C222EA7") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_EB353E96_94B2_486C_9FA6_31A37C222EA7
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "EBD34028-0E59-4B21-86EB-CF892E75F6FF") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_EBD34028_0E59_4B21_86EB_CF892E75F6FF,
            iv_EBD34028_0E59_4B21_86EB_CF892E75F6FF
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "ECE4A1E8-A530-48B2-B968-F503CB1CBAAC") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_ECE4A1E8_A530_48B2_B968_F503CB1CBAAC,
            iv_ECE4A1E8_A530_48B2_B968_F503CB1CBAAC
        );
    }
    // double AES-128-CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "EE0D4296-FC76-4BBB-8E0D-B03DB50776F2") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_EE0D4296_FC76_4BBB_8E0D_B03DB50776F2,
            iv_EE0D4296_FC76_4BBB_8E0D_B03DB50776F2
        );
    }
    // Triple modified-TEA ECB (Windows, 14.146.227.141)
    if (strcmp(algo_id, "F1C01068-FFAB-4E4E-A043-0B06B2299AF4") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_F1C01068_FFAB_4E4E_A043_0B06B2299AF4
        );
    }
    // triple modified-TEA CBC (Windows, 14.146.227.141)
    if (strcmp(algo_id, "FC05D786-59A7-4469-B276-0D9B89EAD057") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_FC05D786_59A7_4469_B276_0D9B89EAD057,
            iv_FC05D786_59A7_4469_B276_0D9B89EAD057
        );
    }
    // triple modified-TEA CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "023BD049-E130-4FBF-AB73-800FA2E38948") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_023BD049_E130_4FBF_AB73_800FA2E38948,
            iv_023BD049_E130_4FBF_AB73_800FA2E38948
        );
    }
    // Triple modified-TEA ECB (Windows, 121.8.177.212)
    if (strcmp(algo_id, "0A4E0BB5-A13D-4FB1-A5C5-D78F1D92CE64") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_0A4E0BB5_A13D_4FB1_A5C5_D78F1D92CE64
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "0E045477-EF58-46D8-BEE5-8D9D577C91F3") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_0E045477_EF58_46D8_BEE5_8D9D577C91F3,
            iv_0E045477_EF58_46D8_BEE5_8D9D577C91F3
        );
    }
    // triple modified-TEA CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "23CBE797-3BC2-4FCF-A589-23C3D31BCED7") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_23CBE797_3BC2_4FCF_A589_23C3D31BCED7,
            iv_23CBE797_3BC2_4FCF_A589_23C3D31BCED7
        );
    }
    // Triple modified-TEA ECB (Windows, 121.8.177.212)
    if (strcmp(algo_id, "262A30B9-FE27-4206-B57A-3CB44E2A1138") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_262A30B9_FE27_4206_B57A_3CB44E2A1138
        );
    }
    // Triple modified-TEA ECB (Windows, 121.8.177.212)
    if (strcmp(algo_id, "4C0EAF45-8E70-45C2-BD82-52C834005758") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_4C0EAF45_8E70_45C2_BD82_52C834005758
        );
    }
    // triple modified-TEA CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "4FE47EF4-1BDC-43DE-AF53-4D6C30DA9ACD") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_4FE47EF4_1BDC_43DE_AF53_4D6C30DA9ACD,
            iv_4FE47EF4_1BDC_43DE_AF53_4D6C30DA9ACD
        );
    }
    // triple modified-TEA CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "51A47203-4DCB-4A5A-9838-C452FD2774DC") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_51A47203_4DCB_4A5A_9838_C452FD2774DC,
            iv_51A47203_4DCB_4A5A_9838_C452FD2774DC
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "52116A19-3222-40F0-A117-DC377BED0D3E") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_52116A19_3222_40F0_A117_DC377BED0D3E,
            iv_52116A19_3222_40F0_A117_DC377BED0D3E
        );
    }
    // triple modified-TEA CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "5D79B368-0B90-4194-8437-AB0571DF0BD5") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_5D79B368_0B90_4194_8437_AB0571DF0BD5,
            iv_5D79B368_0B90_4194_8437_AB0571DF0BD5
        );
    }
    // triple modified-TEA CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "6C5CF775-8F43-4CAA-856E-2845431B4CC0") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_6C5CF775_8F43_4CAA_856E_2845431B4CC0,
            iv_6C5CF775_8F43_4CAA_856E_2845431B4CC0
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "7A16C88E-1E0C-4C4E-8702-E76D7C743C01") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_7A16C88E_1E0C_4C4E_8702_E76D7C743C01,
            iv_7A16C88E_1E0C_4C4E_8702_E76D7C743C01
        );
    }
    // Triple modified-TEA ECB (Windows, 121.8.177.212)
    if (strcmp(algo_id, "8F950701-C9FC-45A5-9780-7305FE025D4E") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_8F950701_C9FC_45A5_9780_7305FE025D4E
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "A7984530-FDA0-455C-ACAC-19854FB7A659") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_A7984530_FDA0_455C_ACAC_19854FB7A659,
            iv_A7984530_FDA0_455C_ACAC_19854FB7A659
        );
    }
    // Triple modified-TEA ECB (Windows, 121.8.177.212)
    if (strcmp(algo_id, "AA4D098D-336C-4523-AD31-C99D3F1514CA") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_AA4D098D_336C_4523_AD31_C99D3F1514CA
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "BE5EC2E8-248A-4868-9A34-B53BE5FDB887") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_BE5EC2E8_248A_4868_9A34_B53BE5FDB887,
            iv_BE5EC2E8_248A_4868_9A34_B53BE5FDB887
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "C12422B4-A6B9-4977-952B-D6627D482627") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_C12422B4_A6B9_4977_952B_D6627D482627,
            iv_C12422B4_A6B9_4977_952B_D6627D482627
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "CA9A519E-AD98-401B-8111-83EFC3C76F7C") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_CA9A519E_AD98_401B_8111_83EFC3C76F7C,
            iv_CA9A519E_AD98_401B_8111_83EFC3C76F7C
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "D245DD7B-900F-456C-9ED5-8C094E8E240C") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_D245DD7B_900F_456C_9ED5_8C094E8E240C,
            iv_D245DD7B_900F_456C_9ED5_8C094E8E240C
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "DA2B057E-5D9E-48BC-B2BC-EA43D8D2A65E") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_DA2B057E_5D9E_48BC_B2BC_EA43D8D2A65E,
            iv_DA2B057E_5D9E_48BC_B2BC_EA43D8D2A65E
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "EC4D2CF4-EC20-4DCA-B35D-097F9C2370FC") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_EC4D2CF4_EC20_4DCA_B35D_097F9C2370FC,
            iv_EC4D2CF4_EC20_4DCA_B35D_097F9C2370FC
        );
    }
    // triple modified-TEA CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "F0DA8287-3EF6-4C7C-BD59-B9542BC16898") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_F0DA8287_3EF6_4C7C_BD59_B9542BC16898,
            iv_F0DA8287_3EF6_4C7C_BD59_B9542BC16898
        );
    }
    // triple modified-TEA CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "F3557D5A-9F79-4A63-A152-4A939FCC96F9") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_F3557D5A_9F79_4A63_A152_4A939FCC96F9,
            iv_F3557D5A_9F79_4A63_A152_4A939FCC96F9
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "F9A253A7-4D9E-4ED3-BA0F-118E56D34267") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_F9A253A7_4D9E_4ED3_BA0F_118E56D34267,
            iv_F9A253A7_4D9E_4ED3_BA0F_118E56D34267
        );
    }
    // double AES-128-CBC (Windows, 121.8.177.212)
    if (strcmp(algo_id, "FF25BFEC-B03C-4EF0-8487-71D41E04FF65") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_FF25BFEC_B03C_4EF0_8487_71D41E04FF65,
            iv_FF25BFEC_B03C_4EF0_8487_71D41E04FF65
        );
    }
    /* --------------------------------------------------- Linux --------------------------------------------------- */
    // 3DES-CBC (Linux, BOTH)
    if (strcmp(algo_id, "1A7343EC-7F9B-4570-BF58-16279A81116B") == 0)
    {
        return create_desede_cbc_cipher(
            key1_1A7343EC_7F9B_4570_BF58_16279A81116B,
            key2_1A7343EC_7F9B_4570_BF58_16279A81116B,
            iv1_1A7343EC_7F9B_4570_BF58_16279A81116B,
            iv2_1A7343EC_7F9B_4570_BF58_16279A81116B
        );
    }
    // AES-ECB (Linux, BOTH)
    if (strcmp(algo_id, "4BA5496A-2123-46A7-85F2-35956EA7BE39") == 0)
    {
        return create_aes_ecb_cipher(
            key1_4BA5496A_2123_46A7_85F2_35956EA7BE39,
            key2_4BA5496A_2123_46A7_85F2_35956EA7BE39
        );
    }
    // AES-CBC (Linux, BOTH)
    if (strcmp(algo_id, "45433DCF-9ECA-4BE5-83F2-F92BA0B4F291") == 0)
    {
        return create_aes_cbc_cipher(
            key1_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291,
            key2_45433DCF_9ECA_4BE5_83F2_F92BA0B4F291,
            NULL
        );
    }
    // ModXTEA (Linux, BOTH)
    if (strcmp(algo_id, "60639D8B-272E-4A4D-976E-AA270987A169") == 0)
    {
        return create_mod_xtea_cipher(
            key1_60639D8B_272E_4A4D_976E_AA270987A169,
            key2_60639D8B_272E_4A4D_976E_AA270987A169,
            key3_60639D8B_272E_4A4D_976E_AA270987A169,
            true
        );
    }
    // MOD_XTEA_CBC 三层 (Linux, BOTH)
    if (strcmp(algo_id, "AB6C8EBE-B8F8-4C08-8222-69A3B5E86A91") == 0)
    {
        return create_mod_xtea_cbc_triple_cipher(
            key1_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            key2_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            key3_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91,
            iv_AB6C8EBE_B8F8_4C08_8222_69A3B5E86A91
        );
    }
    // DES-ECB 6 层, 内联密钥 (Linux, BOTH)
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
    /* --------------------------------------------------- Android --------------------------------------------------- */
    // AES-CBC (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "CAFBCBAD-B6E7-4CAB-8A67-14D39F00CE1E") == 0)
    {
        return create_aes_cbc_cipher(
            key1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
            key2_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E,
            iv1_CAFBCBAD_B6E7_4CAB_8A67_14D39F00CE1E
        );
    }
    // AES-ECB (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "A474B1C2-3DE0-4EA2-8C5F-7093409CE6C4") == 0)
    {
        return create_aes_ecb_cipher(
            key2_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4,
            key1_A474B1C2_3DE0_4EA2_8C5F_7093409CE6C4
        );
    }
    // 3DES-CBC (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "5BFBA864-BBA9-42DB-8EAD-49B5F412BD81") == 0)
    {
        return create_desede_cbc_cipher(
            key2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            key1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            iv2_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81,
            iv1_5BFBA864_BBA9_42DB_8EAD_49B5F412BD81
        );
    }
    // 3DES-ECB (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "6E0B65FF-0B5B-459C-8FCE-EC7F2BEA9FF5") == 0)
    {
        return create_desede_ecb_cipher(
            key1_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5,
            key2_6E0B65FF_0B5B_459C_8FCE_EC7F2BEA9FF5
        );
    }
    // ZUC-128 (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "B809531F-0007-4B5B-923B-4BD560398113") == 0)
    {
        return create_zuc_cipher(
            key_B809531F_0007_4B5B_923B_4BD560398113,
            iv_B809531F_0007_4B5B_923B_4BD560398113
        );
    }
    // SM4-CBC (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "F3974434-C0DD-4C20-9E87-DDB6814A1C48") == 0)
    {
        return create_sm4_cbc_cipher(
            key_F3974434_C0DD_4C20_9E87_DDB6814A1C48,
            iv_F3974434_C0DD_4C20_9E87_DDB6814A1C48
        );
    }
    // SM4-ECB (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "ED382482-F72C-4C41-A76D-28EEA0F1F2AF") == 0)
    {
        return create_sm4_ecb_cipher(
            key_ED382482_F72C_4C41_A76D_28EEA0F1F2AF
        );
    }
    // ModXTEA (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "B3047D4E-67DF-4864-A6A5-DF9B9E525C79") == 0)
    {
        return create_mod_xtea_cipher(
            key1_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
            key2_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
            key3_B3047D4E_67DF_4864_A6A5_DF9B9E525C79,
            false
        );
    }
    // ModXTEA-IV (Android_Old, 14.146.227.141)
    if (strcmp(algo_id, "C32C68F9-CA81-4260-A329-BBAFD1A9CCD1") == 0)
    {
        return create_mod_xtea_iv_cipher(
            key1_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            key2_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            key3_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1,
            iv_C32C68F9_CA81_4260_A329_BBAFD1A9CCD1
        );
    }
    // SNOW3G-variant (Android, 14.146.227.141)
    if (strcmp(algo_id, "07E824B2-9E5C-4D1B-BBB0-5E07C251E4AA") == 0)
    {
        return create_snow3g_variant_cipher(
            key_07E824B2_9E5C_4D1B_BBB0_5E07C251E4AA,
            iv_07E824B2_9E5C_4D1B_BBB0_5E07C251E4AA
        );
    }
    // Triple modified-TEA ECB (Android, 14.146.227.141)
    if (strcmp(algo_id, "319FC5AB-EC0E-46B9-A252-2285F9DAE813") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_319FC5AB_EC0E_46B9_A252_2285F9DAE813
        );
    }
    // SM4-variant CBC (Android, 14.146.227.141)
    if (strcmp(algo_id, "D6544CFE-F2DE-459B-9B77-0F2B367EF169") == 0)
    {
        return create_sm4_variant_cbc_cipher(
            key_D6544CFE_F2DE_459B_9B77_0F2B367EF169,
            iv_D6544CFE_F2DE_459B_9B77_0F2B367EF169
        );
    }
    // SM4-variant ECB (Android, 14.146.227.141)
    if (strcmp(algo_id, "D755A536-B551-468C-BD87-322182B223D4") == 0)
    {
        return create_sm4_variant_ecb_cipher(
            key_D755A536_B551_468C_BD87_322182B223D4
        );
    }
    // double AES-128-CBC (Android, 14.146.227.141)
    if (strcmp(algo_id, "BB2EA626-590B-4C42-82BE-E052FCBBB88E") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_BB2EA626_590B_4C42_82BE_E052FCBBB88E,
            iv_BB2EA626_590B_4C42_82BE_E052FCBBB88E
        );
    }
    // double AES-128-ECB (Android, 14.146.227.141)
    if (strcmp(algo_id, "DEABB8C8-A2BC-48CA-8ED0-8CDF1BD62F61") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_DEABB8C8_A2BC_48CA_8ED0_8CDF1BD62F61
        );
    }
    // double 3DES-CBC (Android, 14.146.227.141)
    if (strcmp(algo_id, "9ABF4D29-34DB-4CE9-BB8C-7E371D637758") == 0)
    {
        return create_desede_double_cbc_cipher(
            key_9ABF4D29_34DB_4CE9_BB8C_7E371D637758,
            iv_9ABF4D29_34DB_4CE9_BB8C_7E371D637758
        );
    }
    // DES-ECB (6 层) (Android, 14.146.227.141)
    if (strcmp(algo_id, "AD8BB5B0-0E72-4198-A362-96D52C1B7ED1") == 0)
    {
        return create_des_ecb_six_cipher(
            key_AD8BB5B0_0E72_4198_A362_96D52C1B7ED1
        );
    }
    // triple modified-TEA CBC (Android, 14.146.227.141)
    if (strcmp(algo_id, "35101415-A20F-4DFE-B00B-0B4F3B2F8C66") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_35101415_A20F_4DFE_B00B_0B4F3B2F8C66,
            iv_35101415_A20F_4DFE_B00B_0B4F3B2F8C66
        );
    }
    // SM4-variant ECB (Android, 121.8.177.212)
    if (strcmp(algo_id, "D522E657-2CC3-46BE-BBE9-9CF1DF896C41") == 0)
    {
        return create_sm4_variant_ecb_cipher(
            key_D522E657_2CC3_46BE_BBE9_9CF1DF896C41
        );
    }
    // Triple modified-TEA ECB (Android, 121.8.177.212)
    if (strcmp(algo_id, "3084D519-54D7-4434-9564-26A8615BCEF2") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_3084D519_54D7_4434_9564_26A8615BCEF2
        );
    }
    // double AES-128-CBC (Android, 121.8.177.212)
    if (strcmp(algo_id, "3D74E736-AB35-47CD-B934-77EBEF2B42D3") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_3D74E736_AB35_47CD_B934_77EBEF2B42D3,
            iv_3D74E736_AB35_47CD_B934_77EBEF2B42D3
        );
    }
    // double 3DES-CBC (Android, 121.8.177.212)
    if (strcmp(algo_id, "64BD1BB8-49F4-448F-9D63-C4E96FC068CC") == 0)
    {
        return create_desede_double_cbc_cipher(
            key_64BD1BB8_49F4_448F_9D63_C4E96FC068CC,
            iv_64BD1BB8_49F4_448F_9D63_C4E96FC068CC
        );
    }
    // DES-ECB (6 层) (Android, 121.8.177.212)
    if (strcmp(algo_id, "252F0396-5254-4A42-9390-FA2EF1EB8D6E") == 0)
    {
        return create_des_ecb_six_cipher(
            key_252F0396_5254_4A42_9390_FA2EF1EB8D6E
        );
    }
    // SM4-variant CBC (Android, 121.8.177.212)
    if (strcmp(algo_id, "73B3F4AE-643F-471F-B08C-A5893D7B3601") == 0)
    {
        return create_sm4_variant_cbc_cipher(
            key_73B3F4AE_643F_471F_B08C_A5893D7B3601,
            iv_73B3F4AE_643F_471F_B08C_A5893D7B3601
        );
    }
    // double AES-128-ECB (Android, 121.8.177.212)
    if (strcmp(algo_id, "7E6A7DE0-4C56-47E6-A25A-1B4DCD33B950") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_7E6A7DE0_4C56_47E6_A25A_1B4DCD33B950
        );
    }
    // SNOW3G-variant (Android, 121.8.177.212)
    if (strcmp(algo_id, "5038A11D-75EA-4F75-AF8D-2DAD8E0E6A19") == 0)
    {
        return create_snow3g_variant_cipher(
            key_5038A11D_75EA_4F75_AF8D_2DAD8E0E6A19,
            iv_5038A11D_75EA_4F75_AF8D_2DAD8E0E6A19
        );
    }
    // triple modified-TEA CBC (Android, 121.8.177.212)
    if (strcmp(algo_id, "406BD7C5-C48A-4F48-B072-82D5948B537B") == 0)
    {
        return create_tea_triple_cbc_cipher(
            key_406BD7C5_C48A_4F48_B072_82D5948B537B,
            iv_406BD7C5_C48A_4F48_B072_82D5948B537B
        );
    }
    /* --------------------------------------------------- iOS --------------------------------------------------- */
    // double AES-128-CBC (iOS, 14.146.227.141)
    if (strcmp(algo_id, "2E75BFA5-76C5-4691-BBF5-A49C94908464") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_2E75BFA5_76C5_4691_BBF5_A49C94908464,
            iv_2E75BFA5_76C5_4691_BBF5_A49C94908464
        );
    }
    // Triple modified-TEA ECB (iOS, 14.146.227.141)
    if (strcmp(algo_id, "4D10D93E-5A68-46D4-83EA-49E7C4933C91") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_4D10D93E_5A68_46D4_83EA_49E7C4933C91
        );
    }
    // double AES-128-ECB (iOS, 14.146.227.141)
    if (strcmp(algo_id, "E2F8C4B2-2742-4690-9BD5-BBAB926FB9E8") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_E2F8C4B2_2742_4690_9BD5_BBAB926FB9E8
        );
    }
    // Triple modified-TEA ECB (iOS, 121.8.177.212)
    if (strcmp(algo_id, "928CE71E-6D12-4887-ABB8-500CDD18FBF2") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_928CE71E_6D12_4887_ABB8_500CDD18FBF2
        );
    }
    // double AES-128-ECB (iOS, 121.8.177.212)
    if (strcmp(algo_id, "674E4CDD-5556-441B-AAAD-A69FB14F49BC") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_674E4CDD_5556_441B_AAAD_A69FB14F49BC
        );
    }
    // double AES-128-CBC (iOS, 121.8.177.212)
    if (strcmp(algo_id, "89E57D42-9B77-4CBB-B08A-D9CD602E4A67") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_89E57D42_9B77_4CBB_B08A_D9CD602E4A67,
            iv_89E57D42_9B77_4CBB_B08A_D9CD602E4A67
        );
    }
    /* --------------------------------------------------- macOS --------------------------------------------------- */
    // double AES-128-ECB (macOS, 14.146.227.141)
    if (strcmp(algo_id, "634D7C6F-581F-47CC-959C-D9265EDAB9B4") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_634D7C6F_581F_47CC_959C_D9265EDAB9B4
        );
    }
    // Triple modified-TEA ECB (macOS, 14.146.227.141)
    if (strcmp(algo_id, "7781847B-446E-4CF7-A418-79989EFA1B00") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_7781847B_446E_4CF7_A418_79989EFA1B00
        );
    }
    // double AES-128-CBC (macOS, 14.146.227.141)
    if (strcmp(algo_id, "E366503B-B6E5-40A5-BCE1-39001D18D90E") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_E366503B_B6E5_40A5_BCE1_39001D18D90E,
            iv_E366503B_B6E5_40A5_BCE1_39001D18D90E
        );
    }
    // Triple modified-TEA ECB (macOS, 121.8.177.212)
    if (strcmp(algo_id, "E7BE4937-4069-4096-9437-B2B9C1AE9021") == 0)
    {
        return create_tea_triple_ecb_cipher(
            key_E7BE4937_4069_4096_9437_B2B9C1AE9021
        );
    }
    // double AES-128-ECB (macOS, 121.8.177.212)
    if (strcmp(algo_id, "064C7B38-20B9-4CE4-9760-66822A3BF0E7") == 0)
    {
        return create_aes_double_ecb_cipher(
            key_064C7B38_20B9_4CE4_9760_66822A3BF0E7
        );
    }
    // double AES-128-CBC (macOS, 121.8.177.212)
    if (strcmp(algo_id, "AED79B90-C27B-4D4F-8E53-C522FA955CFE") == 0)
    {
        return create_aes_double_cbc_cipher(
            key_AED79B90_C27B_4D4F_8E53_C522FA955CFE,
            iv_AED79B90_C27B_4D4F_8E53_C522FA955CFE
        );
    }

    return NULL;
}

void destroy_cipher_factory(void)
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

bool cipher_supported(const char* algo_id)
{
    if (algo_id == NULL) return false;

    cipher_interface_t* cipher = create_cipher_factory(algo_id);
    if (cipher == NULL) return false;

    if (cipher->destroy != NULL) cipher->destroy(cipher);
    return true;
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
