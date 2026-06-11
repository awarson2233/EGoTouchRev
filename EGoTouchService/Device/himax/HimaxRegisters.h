#pragma once
#include <cstdint>

class ic_operation {
public:
    uint8_t addr_ahb_addr_byte_0;				// 0x00 - AHB address byte 0
    uint8_t addr_ahb_rdata_byte_0;				// 0x08 - AHB read data byte 0
    uint8_t addr_ahb_access_direction;			// 0x0C - AHB access direction
    uint8_t addr_conti;							// 0x13 - Continuous mode address
    uint8_t addr_incr4;							// 0x0D - Increment 4 address
    uint8_t adr_i2c_psw_lb;						// 0x31 - I2C password lower byte
    uint8_t adr_i2c_psw_ub;						// 0x32 - I2C password upper byte
    uint8_t data_ahb_access_direction_read;		// 0x00 - AHB read access direction
    uint8_t data_conti;							// 0x31 - Continuous mode data
    uint8_t data_incr4;							// 0x12 - Increment 4 data
    uint8_t data_i2c_psw_lb;					// 0x27 - I2C password lower byte data
    uint8_t data_i2c_psw_ub;					// 0x95 - I2C password upper byte data
    uint32_t addr_tcon_on_rst;					// 0x80020020 - TCON on reset address
    uint32_t addr_adc_on_rst;					// 0x80020094 - ADC on reset address
    uint32_t addr_psl;							// 0x900000A0 - Power saving level address
    uint32_t addr_cs_central_state;				// 0x900000A8 - Central state address
    uint32_t data_rst;							// 0x00000000 - Reset data
    uint32_t adr_osc_en;						// 0x900880A8 - Oscillator enable address
    uint32_t adr_osc_pw;						// 0x900880E0 - Oscillator power address
};

class fw_operation {
public:
    uint32_t addr_system_reset;					// 0x90000018 
    uint32_t addr_ctrl_fw_isr;					// 0x9000005C
    uint32_t addr_flag_reset_event;				// 0x900000E4
    uint32_t addr_hsen_enable;					// 0x10007F14
    uint32_t addr_smwp_enable;					// 0x10007F10
    uint32_t addr_program_reload_from;			// 0x00000000
    uint32_t addr_program_reload_to;			// 0x08000000
    uint32_t addr_program_reload_page_write;    // 0x0000FB00
    uint32_t addr_raw_out_sel;                  // 0x100072EC
    uint32_t addr_reload_status;                // 0x80050000
    uint32_t addr_reload_crc32_result;          // 0x80050018
    uint32_t addr_reload_addr_from;             // 0x80050020
    uint32_t addr_reload_addr_cmd_beat;         // 0x80050028
    uint32_t addr_selftest_addr_en;             // 0x10007F18
    uint32_t addr_criteria_addr;                // 0x10007F1C
    uint32_t addr_set_frame_addr;               // 0x10007294
    uint32_t addr_selftest_result_addr;         // 0x10007F24
    uint32_t addr_sorting_mode_en;              // 0x10007F04
    uint32_t addr_fw_mode_status;               // 0x10007088
    uint32_t addr_icid_addr;                    // 0x900000D0
    uint32_t addr_fw_ver_addr;                  // 0x10007004
    uint32_t addr_fw_cfg_addr;                  // 0x10007084
    uint32_t addr_fw_vendor_addr;               // 0x10007000
    uint32_t addr_cus_info;                     // 0x10007008
    uint32_t addr_proj_info;                    // 0x10007014
    uint32_t addr_fw_state_addr;                // 0x900000F8
    uint32_t addr_fw_dbg_msg_addr;              // 0x10007F40
    uint32_t addr_chk_fw_status;                // 0x900000A8
    uint32_t addr_chk_dd_status;                // 0x900000E8
    uint32_t addr_dd_handshak_addr;             // 0x900000FC
    uint32_t addr_dd_data_addr;                 // 0x10007F80
    uint32_t addr_clr_fw_record_dd_sts;         // 0x10007FCC
    uint32_t addr_ap_notify_fw_sus;             // 0x10007FD0
    uint32_t data_ap_notify_fw_sus_en;          // 0xA55AA55A
    uint32_t data_ap_notify_fw_sus_dis;         // 0x00000000
    uint32_t data_system_reset;                 // 0x00000055
    uint32_t data_safe_mode_release_pw_active;  // 0x00000053
    uint32_t data_safe_mode_release_pw_reset;   // 0x00000000
    uint32_t data_clear;                        // 0x00000000
    uint32_t data_fw_stop;                      // 0x000000A5
    uint32_t data_program_reload_start;         // 0x0A3C3000
    uint32_t data_program_reload_compare;       // 0x04663000
    uint32_t data_program_reload_break;         // 0x15E75678
    uint32_t data_selftest_request;             // 0x00006AA6
    uint32_t data_set_frame;                    // 0x0000000A
    uint32_t addr_usb_detect;                   // 0x10007F38
    uint32_t data_normal_mode;                  // 0x00000000  
    uint32_t data_open_mode;                    // 0x00007777  
    uint32_t data_short_mode;                   // 0x00001111
    uint32_t data_sorting_mode;                 // 0x0000AAAA
    uint8_t data_criteria_aa_top;               // 0x64
    uint8_t data_criteria_aa_bot;               // 0x00
    uint8_t data_criteria_key_top;              // 0x64
    uint8_t data_criteria_key_bot;              // 0x00
    uint8_t data_criteria_avg_top;              // 0x64
    uint8_t data_criteria_avg_bot;              // 0x00
    uint8_t data_selftest_ack_hb;               // 0xA6
    uint8_t data_selftest_ack_lb;               // 0x6A
    uint8_t data_selftest_pass;                 // 0xAA
    uint8_t data_normal_cmd;                    // 0x00
    uint8_t data_normal_status;                 // 0x99
    uint8_t data_sorting_cmd;                   // 0xAA
    uint8_t data_sorting_status;                // 0xCC
    uint8_t data_dd_request;                    // 0xAA
    uint8_t data_dd_ack;                        // 0xBB
    uint8_t data_idle_dis_pwd;                  // 0x17
    uint8_t data_idle_en_pwd;                   // 0x1F
    uint8_t data_rawdata_ready_hb;              // 0xA3
    uint8_t data_rawdata_ready_lb;              // 0x3A
    uint8_t addr_ahb_addr;                      // 0x11
    uint8_t data_ahb_dis;                       // 0x00
    uint8_t data_ahb_en;                        // 0x01
    uint8_t addr_event_addr;                    // 0x30
    uint8_t addr_ulpm_33;                       // 0x33
    uint8_t addr_ulpm_34;                       // 0x34
    uint8_t data_ulpm_11;                       // 0x11
    uint8_t data_ulpm_22;                       // 0x22
    uint8_t data_ulpm_33;                       // 0x33
    uint8_t data_ulpm_aa;                       // 0xAA
};

class flash_operation {
public:
    uint32_t addr_spi200_trans_fmt;             // 0x80000010
    uint32_t addr_spi200_trans_ctrl;            // 0x80000020
    uint32_t addr_spi200_fifo_rst;              // 0x80000030
    uint32_t addr_spi200_rst_status;            // 0x80000034
    uint32_t addr_spi200_flash_speed;           // 0x80000040
    uint32_t addr_spi200_cmd;                   // 0x80000024
    uint32_t addr_spi200_addr;                  // 0x80000028
    uint32_t addr_spi200_data;                  // 0x8000002C
    uint32_t addr_spi200_bt_num;                // 0x800000E8

    uint32_t data_spi200_txfifo_rst;            // 0x00000004
    uint32_t data_spi200_rxfifo_rst;            // 0x00000002
    uint32_t data_spi200_trans_fmt;             // 0x00020780
    uint32_t data_spi200_trans_ctrl_1;          // 0x42000003
    uint32_t data_spi200_trans_ctrl_2;          // 0x47000000
    uint32_t data_spi200_trans_ctrl_3;          // 0x67000000
    uint32_t data_spi200_trans_ctrl_4;          // 0x610FF000
    uint32_t data_spi200_trans_ctrl_5;          // 0x694002FF
    uint32_t data_spi200_trans_ctrl_6;          // 0x42000000
    uint32_t data_spi200_trans_ctrl_7;          // 0x6940020F
    uint32_t data_spi200_cmd_1;                 // 0x00000005
    uint32_t data_spi200_cmd_2;                 // 0x00000006
    uint32_t data_spi200_cmd_3;                 // 0x000000C7
    uint32_t data_spi200_cmd_4;                 // 0x000000D8
    uint32_t data_spi200_cmd_5;                 // 0x00000020
    uint32_t data_spi200_cmd_6;                 // 0x00000002
    uint32_t data_spi200_cmd_7;                 // 0x0000003B
    uint32_t data_spi200_cmd_8;                 // 0x00000003
    uint32_t data_spi200_addr;                  // 0x00000000
};

class sram_operation {
public:
    uint32_t addr_mkey;                         // 0x100070E8
    uint32_t addr_rawdata_addr;                 // 0x10000000
    uint32_t addr_rawdata_end;                  // 0x00000000
    uint32_t passwrd;                           // 0x0000A55A
};

class driver_operation {
public:
    uint32_t addr_fw_define_flash_reload;               // 0x10007F00
    uint32_t addr_fw_define_2nd_flash_reload;           // 0x100072C0
    uint32_t addr_fw_define_int_is_edge;                // 0x10007088
    uint32_t addr_fw_define_rxnum_txnum;                // 0x100070F4
    uint32_t addr_fw_define_maxpt_xyrvs;                // 0x100070F8
    uint32_t addr_fw_define_x_y_res;                    // 0x100070FC
    uint32_t data_fw_define_flash_reload_dis;           // 0x0000A55A
    uint32_t data_fw_define_flash_reload_en;            // 0x00000000
    uint32_t data_fw_define_rxnum_txnum_maxpt_sorting;  // 0x00000008
    uint32_t data_fw_define_rxnum_txnum_maxpt_normal;   // 0x00000014
    uint8_t data_df_rx;                                 // 60
    uint8_t data_df_tx;                                 // 40
    uint8_t data_df_pt;                                 // 10
};

class zf_operation {
public:
    uint32_t data_dis_flash_reload;             // 0x00009AA9
    uint32_t addr_system_reset;                 // 0x90000018
    uint32_t data_system_reset;                 // 0x00000055
    uint32_t data_sram_start_addr;              // 0x08000000
    uint32_t data_sram_clean;                   // (runtime filled)
    uint32_t data_cfg_info;                     // 0x10007000
    uint32_t data_fw_cfg_1;                     // 0x10007084
    uint32_t data_fw_cfg_2;                     // 0x10007264
    uint32_t data_fw_cfg_3;                     // 0x10007300
    uint32_t data_adc_cfg_1;                    // 0x10006A00
    uint32_t data_adc_cfg_2;                    // 0x10007B28
    uint32_t data_adc_cfg_3;                    // 0x10007AF0
    uint32_t data_map_table;                    // 0x10007500
    uint32_t addr_sts_chk;                      // 0x900000A8
    uint32_t addr_activ_relod;                  // 0x90000048
    uint8_t data_activ_sts;                     // 0x05
    uint8_t data_activ_in;                      // 0xEC
};

namespace Himax {
    ic_operation InitIcOperation();
    fw_operation InitFwOperation();
    flash_operation InitFlashOperation();
    sram_operation InitSramOperation();
    driver_operation InitDriverOperation();
    zf_operation InitZfOperation();
}
