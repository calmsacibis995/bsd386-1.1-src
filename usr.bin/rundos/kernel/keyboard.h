/* Commands from keyboard to system */

#define KB_SELF_TEST_PASS 0xAA
#define KB_SELF_TEST_FAIL_1 0xFC /* Apricot */
#define KB_SELF_TEST_FAIL_2 0xFD /* IBM PC/AT */
#define KB_RESEND 0xFE
#define KB_ACK 0xFA
#define KB_ECHO 0xEE
#define KB_OVERRUN 0xFF
#define KB_OVERRUN_2 0x00
#define KB_ID_1ST 0xAB
#define KB_ID_2ND_1 0x83   /* Apricot */
#define KB_ID_2ND_2 0x54   /* American Megatrends */
#define KB_ID_2ND_3 0x41   /* IBM PC/AT ; American Megatrends */

/* Commands from system to keyboard */

#define KB_CMD_VALID_LIMIT 0xED
#define KB_CMD_SET_LED 0xED
#define KB_CMD_ECHO 0xEE
#define KB_CMD_NOP_1 0xEF
#define KB_CMD_SELECT 0xF0    /* Apricot */
#define KB_CMD_NOP_2 0xF1
#define KB_CMD_READ_ID 0xF2
#define KB_CMD_SET_RATE 0xF3
#define KB_CMD_ENABLE 0xF4
#define KB_CMD_DEF_DISABLE 0xF5
#define KB_CMD_SET_DEFAULT 0xF6
#define KB_CMD_SET_ALL_TM 0xF7      /* Apricot */
#define KB_CMD_SET_ALL_MB 0xF8      /* Apricot */
#define KB_CMD_SET_ALL_MO 0xF9      /* Apricot */
#define KB_CMD_SET_ALL_TMMB 0xFA    /* Apricot */
#define KB_CMD_SET_KEY_TM 0xFB      /* Apricot */
#define KB_CMD_SET_KEY_MB 0xFC      /* Apricot */
#define KB_CMD_SET_KEY_TMMB 0xFD    /* Apricot */
#define KB_CMD_RESEND 0xFE
#define KB_CMD_RESET 0xFF

/* Commands to keyboard controller */

#define KB_CTL_READ_CMD 0x20
#define KB_CTL_WRITE_CMD 0x60
#define KB_CTL_SELF_TEST 0xAA
#define KB_CTL_INTERF_TEST 0xAB
#define KB_CTL_DIAG_DUMP 0xAC
#define KB_CTL_DISABLE 0xAD
#define KB_CTL_ENABLE 0xAE
#define KB_CTL_READ_IN_PORT 0xC0
#define KB_CTL_READ_OUT_PORT 0xD0
#define KB_CTL_WRITE_OUT_PORT 0xD1
#define KB_CTL_READ_TEST 0xE0
#define KB_CTL_PULSE_OUTP 0xF0

/* State of keyboard controller */

#define KB_CTS_OUT_BUF_FULL 0x01
#define KB_CTS_IN_BUF_FULL 0x02
#define KB_CTS_SYS_FLAG 0x04  /* 0 => reset by "power on", 1 after self-test */
#define KB_CTS_CMD_DATA_FLAG 0x08   /* last write to 64 => 1, to 60 => 0 */
#define KB_CTS_INHIBIT_SW 0x10   /* IBM PC/AT (inhibit switch -- 0 = inhibited) */
#define KB_CTS_TRANS_TOUT 0x20
#define KB_CTS_RECIE_TOUT 0x40
#define KB_CTS_PARITY_ERR 0x80

/* bit definitions for keyboard command byte */

#define CMD_REG_RESERV_1 0x80
#define CMD_REG_PC_COMPATIB 0x40
#define CMD_REG_PC_MODE 0x20
#define CMD_REG_DISABLE 0x10
#define CMD_REG_INHIB_OVERR 0x08
#define CMD_REG_SYS_FLAG 0x04
#define CMD_REG_RESERV_2 0x02
#define CMD_REG_ENABLE_INT 0x01

/* bit definitions for keyboard coontroller's output port */

#define OUT_P_KB_DATA 0x80
#define OUT_P_KB_CLOCK 0x40
#define OUT_P_INP_BUF_EMPT 0x20
#define OUT_P_OUT_BUF_FULL 0x10
#define OUT_P_RESERV_1 0x08
#define OUT_P_RESERV_2 0x04
#define OUT_P_GATE_A20 0x02
#define OUT_P_RESET 0x01

/***************************************************************/

/* System scan codes */

#define MCOD_E0 0xE0
#define MCOD_E1 0xE1

#define MCOD_TAB 0x0F
#define MCOD_ENTER 0x1C
#define MCOD_CTRL 0x1D
#define MCOD_LSHFT 0x2A
#define MCOD_SLASH 0x35
#define MCOD_ALPHA_LIMIT 0x35
#define MCOD_RSHFT 0x36
#define MCOD_STAR 0x37
#define MCOD_ALT 0x38
#define MCOD_SPACE 0x39
#define MCOD_CAPS_LOCK 0x3A
#define MCOD_NUM_LOCK 0x45
#define MCOD_SCRL_LOCK 0x46
#define MCOD_LEFT_PART_LIMIT 0x46
#define MCOD_GREY_MINUS 0x4A
#define MCOD_INS 0x52
#define MCOD_DEL 0x53
#define MCOD_SYS_REQ 0x54
#define MCOD_LEFT_PART_LIMIT_1 0x56

#define MCOD_MAXIMUM 0x56

/* Keyboard controller's ports */

#define KB_PORT_DATA 0x60
#define KB_PORT_CTRL 0x64
#define KB_PORT_STAT 0x64

/* KB_FLAG (40:17) */

#define INS_ST_MASK 0x80
#define CAPS_ST_MASK 0x40
#define NUM_ST_MASK 0x20
#define SCRL_ST_MASK 0x10
#define ALT_ST_MASK 0x08
#define CTRL_ST_MASK 0x04
#define L_SHFT_MASK 0x02
#define R_SHFT_MASK 0x01
#define SHFT_ST_MASK (L_SHFT_MASK | R_SHFT_MASK)
#define ALL_LOCKS_ST_MASK (CAPS_ST_MASK | NUM_ST_MASK | SCRL_ST_MASK)

/* KB_FLAG_1 (40:18) */

#define INS_PR_MASK 0x80   /* must = INS_ST_MASK !!! */
#define CAPS_PR_MASK 0x40  /* must = CAPS_ST_MASK !!! */
#define NUM_PR_MASK 0x20   /* must = NUM_ST_MASK !!! */
#define SCRL_PR_MASK 0x10  /* must = SCRL_ST_MASK !!! */
#define HOLD_STATE_MASK 0x08
#define SYS_REQ_MASK 0x04
#define L_ALT_MASK 0x02    /* must = ALT_ST_MASK >> 2 !!! */
#define L_CTRL_MASK 0x01   /* must = CTRL_ST_MASK >> 2 !!! */

/* KB_FLAG_3 (40:96) */

#define RD_ID_MASK 0x80
#define ID_1ST_MASK 0x40
#define FORCE_NUMLC_MASK 0x20
#define ENH_KB_MASK 0x10
#define R_ALT_MASK 0x08    /* must = ALT_ST_MASK !!! */
#define R_CTRL_MASK 0x04   /* must = CTRL_ST_MASK !!! */
#define LAST_E0_MASK 0x02
#define LAST_E1_MASK 0x01

/* KB_FLAG_2 (40:97) */

#define KB_ERROR_MASK 0x80
#define LED_UPDT_MASK 0x40
#define LAST_RESEND_MASK 0x20
#define LAST_ACK_MASK 0x10
#define CAPS_LED_MASK 0x04 /* must = CAPS_ST_MASK >> 4 !!! */
#define NUM_LED_MASK 0x02  /* must = NUM_ST_MASK >> 4 !!! */
#define SCRL_LED_MASK 0x01 /* must = SCRL_ST_MASK >> 4 !!! */
#define ALL_LED_MASK (CAPS_LED_MASK | NUM_LED_MASK | SCRL_LED_MASK)

