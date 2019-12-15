#define SPG_HBLANK_INT (*(unsigned volatile*)0xa05f80c8)
#define SPG_VBLANK_INT (*(unsigned volatile*)0xa05f80cc)
#define SPG_CONTROL    (*(unsigned volatile*)0xa05f80d0)
#define SPG_HBLANK     (*(unsigned volatile*)0xa05f80d4)
#define SPG_LOAD       (*(unsigned volatile*)0xa05f80d8)
#define SPG_VBLANK     (*(unsigned volatile*)0xa05f80dc)
#define SPG_WIDTH      (*(unsigned volatile*)0xa05f80e0)

#define VO_CONTROL     (*(unsigned volatile*)0xa05f80e8)
#define VO_STARTX      (*(unsigned volatile*)0xa05f80ec)
#define VO_STARTY      (*(unsigned volatile*)0xa05f80f0)

#define FB_R_CTRL      (*(unsigned volatile*)0xa05f8044)
#define FB_R_SOF1      (*(unsigned volatile*)0xa05f8050)
#define FB_R_SOF2      (*(unsigned volatile*)0xa05f8054)
#define FB_R_SIZE      (*(unsigned volatile*)0xa05f805c)

// basic framebuffer parameters
#define LINESTRIDE_PIXELS  640
#define BYTES_PER_PIXEL      2
#define FRAMEBUFFER_WIDTH  640
#define FRAMEBUFFER_HEIGHT 476

// TODO: I want to implement double-buffering and VBLANK interrupts, but I don't have that yet.
#define FRAMEBUFFER_1 ((void volatile*)0xa5200000)
#define FRAMEBUFFER_2 ((void volatile*)0xa5600000)

#define FB_R_SOF1_FRAME1 0x00200000
#define FB_R_SOF2_FRAME1 0x00200500
#define FB_R_SOF1_FRAME2 0x00600000
#define FB_R_SOF2_FRAME2 0x00600500

void *get_romfont_pointer(void);

static unsigned get_controller_buttons(void);
static int check_controller(void);

static void volatile *cur_framebuffer;

static void configure_video(void) {
    // Hardcoded for 640x476i NTSC video
    SPG_HBLANK_INT = 0x03450000;
    SPG_VBLANK_INT = 0x00150104;
    SPG_CONTROL = 0x00000150;
    SPG_HBLANK = 0x007E0345;
    SPG_LOAD = 0x020C0359;
    SPG_VBLANK = 0x00240204;
    SPG_WIDTH = 0x07d6c63f;
    VO_CONTROL = 0x00160000;
    VO_STARTX = 0x000000a4;
    VO_STARTY = 0x00120012;
    FB_R_CTRL = 0x00000005;
    FB_R_SOF1 = FB_R_SOF1_FRAME1;
    FB_R_SOF2 = FB_R_SOF2_FRAME1;
    FB_R_SIZE = 0x1413b53f;

    cur_framebuffer = FRAMEBUFFER_1;
}

static void clear_screen(void volatile* fb, unsigned short color) {
    unsigned color_2pix = ((unsigned)color) | (((unsigned)color) << 16);

    unsigned volatile *row_ptr = (unsigned volatile*)fb;

    unsigned row, col;
    for (row = 0; row < FRAMEBUFFER_HEIGHT; row++)
        for (col = 0; col < (FRAMEBUFFER_WIDTH / 2); col++)
            *row_ptr++ = color_2pix;
}

static unsigned short make_color(unsigned red, unsigned green, unsigned blue) {
    if (red > 255)
        red = 255;
    if (green > 255)
        green = 255;
    if (blue > 255)
        blue = 255;

    red >>= 3;
    green >>= 2;
    blue >>= 3;

    return blue | (green << 5) | (red << 11);
}

static void
create_font(unsigned short *font,
            unsigned short foreground, unsigned short background) {
    get_romfont_pointer();
    char const *romfont = get_romfont_pointer();

    unsigned glyph;
    for (glyph = 0; glyph < 288; glyph++) {
        unsigned short *glyph_out = font + glyph * 24 * 12;
        char const *glyph_in = romfont + (12 * 24 / 8) * glyph;

        unsigned row, col;
        for (row = 0; row < 24; row++) {
            for (col = 0; col < 12; col++) {
                unsigned idx = row * 12 + col;
                char const *inp = glyph_in + idx / 8;
                char mask = 0x80 >> (idx % 8);
                unsigned short *outp = glyph_out + idx;
                if (*inp & mask)
                    *outp = foreground;
                else
                    *outp = background;
            }
        }
    }
}

#define MAX_CHARS_X (FRAMEBUFFER_WIDTH / 12)
#define MAX_CHARS_Y (FRAMEBUFFER_HEIGHT / 24)

static void draw_glyph(void volatile *fb, unsigned short const *font,
                       unsigned glyph_no, unsigned x, unsigned y) {
    if (glyph_no > 287)
        glyph_no = 0;
    unsigned short volatile *outp = ((unsigned short volatile*)fb) +
        y * LINESTRIDE_PIXELS + x;
    unsigned short const *glyph = font + glyph_no * 24 * 12;

    unsigned row;
    for (row = 0; row < 24; row++) {
        unsigned col;
        for (col = 0; col < 12; col++) {
            outp[col] = glyph[row * 12 + col];
        }
        outp += LINESTRIDE_PIXELS;
    }
}

static void draw_char(void volatile *fb, unsigned short const *font,
                      char ch, unsigned row, unsigned col) {
    if (row >= MAX_CHARS_Y || col >= MAX_CHARS_X)
        return;

    unsigned x = col * 12;
    unsigned y = row * 24;

    unsigned glyph;
    if (ch >= 33 && ch <= 126)
        glyph = ch - 33 + 1;
    else
        return;

    draw_glyph(fb, font, glyph, x, y);
}

static void drawstring(void volatile *fb, unsigned short const *font,
                       char const *msg, unsigned row, unsigned col) {
    while (*msg) {
        if (col >= MAX_CHARS_X) {
            col = 0;
            row++;
        }
        if (*msg == '\n') {
            col = 0;
            row++;
            msg++;
            continue;
        }
        draw_char(fb, font, *msg++, row, col++);
    }
}

#define ARM7_RESET (*(volatile unsigned*)0xa0702c00)

static void disable_arm(void) {
    ARM7_RESET |=1;
}

static void enable_arm(void) {
    ARM7_RESET &= ~1;
}

#include "arm_prog.h"

#define MSG_SEQNO       (*(unsigned volatile*)0xa0900000)
#define MSG_SEQNO_ACK   (*(unsigned volatile*)0xa0900004)
#define MSG_RET_OUT     (*(unsigned volatile*)0xa0900008)
#define MSG_OPCODE      (*(unsigned volatile*)0xa090000c)
#define MSG_DATA_P      ((unsigned volatile*)0xa0900010)

#define DATA_LEN 112

struct msg {
    unsigned opcode;
    char msg[DATA_LEN];
};

static int check_msg(struct msg *msgp);

static int validate_fibonacci(char const dat[DATA_LEN]) {
    unsigned const *dat32 = (unsigned const*)dat;

    if (dat32[0] != 1 || dat32[1] != 1)
        return 0;

    unsigned idx;
    for (idx = 2; idx < DATA_LEN/4; idx++)
        if (dat32[idx] != dat32[idx - 1] + dat32[idx - 2])
            return 0;
    return 1;
}

#define ARM7_OPCODE_FIBONACCI 69
#define ARM7_OPCODE_PRINT 70
#define ARM7_OPCODE_GET_BTNS 71
#define ARM7_OPCODE_CLEAR_SCREEN 72
#define ARM7_OPCODE_EXCEPTION 73

static int arm7_operational;
static unsigned last_seqno;

static void init_arm_cpu(void) {

    disable_arm();

    MSG_SEQNO = 0;
    MSG_SEQNO_ACK = 0;
    arm7_operational = 0;
    last_seqno = 0;

    unsigned volatile *outp = (unsigned volatile*)0xa0800000;
    unsigned const *inp = (unsigned const*)arm7_program;

    unsigned const *inp_end = inp + sizeof(arm7_program) / 4;
    while (inp < inp_end)
        *outp++ = *inp++;

    enable_arm();
}

// returns 1 if there's a message, else 0
static int check_msg(struct msg *msgp) {
    int i;

    unsigned seqno = MSG_SEQNO;

    if (seqno == last_seqno)
        return 0;

    msgp->opcode = MSG_OPCODE;
    last_seqno = seqno;

    unsigned *dstp = (unsigned*)msgp->msg;
    unsigned idx;
    for (idx = 0; idx < DATA_LEN/4; idx++) {
        dstp[idx] = MSG_DATA_P[idx];
    }

    return 1;
}

static int return_msg(unsigned ret_val) {
    MSG_RET_OUT = ret_val;
    MSG_SEQNO_ACK = last_seqno;
}


#define REG_ISTNRM (*(unsigned volatile*)0xA05F6900)

static void swap_buffers(void) {
    if (cur_framebuffer == FRAMEBUFFER_1) {
        FB_R_SOF1 = FB_R_SOF1_FRAME2;
        FB_R_SOF2 = FB_R_SOF2_FRAME2;
        cur_framebuffer = FRAMEBUFFER_2;
    } else {
        FB_R_SOF1 = FB_R_SOF1_FRAME1;
        FB_R_SOF2 = FB_R_SOF2_FRAME1;
        cur_framebuffer = FRAMEBUFFER_1;
    }
}

static void volatile *get_backbuffer(void) {
    if (cur_framebuffer == FRAMEBUFFER_1)
        return FRAMEBUFFER_2;
    else
        return FRAMEBUFFER_1;
}

static int check_vblank(void) {
    int ret = (REG_ISTNRM & (1 << 3)) ? 1 : 0;
    if (ret)
        REG_ISTNRM = (1 << 3);
    return ret;
}

#define REG_MDSTAR (*(unsigned volatile*)0xa05f6c04)
#define REG_MDTSEL (*(unsigned volatile*)0xa05f6c10)
#define REG_MDEN   (*(unsigned volatile*)0xa05f6c14)
#define REG_MDST   (*(unsigned volatile*)0xa05f6c18)
#define REG_MSYS   (*(unsigned volatile*)0xa05f6c80)
#define REG_MDAPRO (*(unsigned volatile*)0xa05f6c8c)
#define REG_MMSEL  (*(unsigned volatile*)0xa05f6ce8)

static void volatile *align32(void volatile *inp) {
    char volatile *as_ch = (char volatile*)inp;
    while (((unsigned)as_ch) & 31)
        as_ch++;
    return (void volatile*)as_ch;
}
#define MAKE_PHYS(addr) ((void*)((((unsigned)addr) & 0x1fffffff) | 0xa0000000))

static void wait_maple(void) {
    while (!(REG_ISTNRM & (1 << 12)))
           ;

    // clear the interrupt
    REG_ISTNRM |= (1 << 12);
}

static int check_controller(void) {
    // clear any pending interrupts (there shouldn't be any but do it anyways)
    REG_ISTNRM |= (1 << 12);

    // disable maple DMA
    REG_MDEN = 0;

    // make sure nothing else is going on
    while (REG_MDST)
        ;

    // 2mpbs transfer, timeout after 1ms
    REG_MSYS = 0xc3500000;

    // trigger via CPU (as opposed to vblank)
    REG_MDTSEL = 0;

    // let it write wherever it wants, i'm not too worried about rogue DMA xfers
    REG_MDAPRO = 0x6155407f;

    // construct packet
    static char volatile devinfo0[1024];
    static unsigned volatile frame[36 + 31];

    unsigned volatile *framep = (unsigned*)MAKE_PHYS(align32(frame));
    char volatile *devinfo0p = (char*)MAKE_PHYS(align32(devinfo0));

    framep[0] = 0x80000000;
    framep[1] = ((unsigned)devinfo0p) & 0x1fffffff;
    framep[2] = 0x2001;

    // set SB_MDSTAR to the address of the packet
    REG_MDSTAR = ((unsigned)framep) & 0x1fffffff;

    // enable maple DMA
    REG_MDEN = 1;

    // begin the transfer
    REG_MDST = 1;

    wait_maple();

    // transfer is now complete, receive data
    if (devinfo0p[0] == 0xff || devinfo0p[4] != 0 || devinfo0p[5] != 0 ||
        devinfo0p[6] != 0 || devinfo0p[7] != 1)
        return 0;

    char const *expect = "Dreamcast Controller         ";
    char const volatile *devname = devinfo0p + 22;

    while (*expect)
        if (*devname++ != *expect++)
            return 0;
    return 1;
}

static unsigned get_controller_buttons(void) {
    if (!check_controller())
        return ~0;

    // clear any pending interrupts (there shouldn't be any but do it anyways)
    REG_ISTNRM |= (1 << 12);

    // disable maple DMA
    REG_MDEN = 0;

    // make sure nothing else is going on
    while (REG_MDST)
        ;

    // 2mpbs transfer, timeout after 1ms
    REG_MSYS = 0xc3500000;

    // trigger via CPU (as opposed to vblank)
    REG_MDTSEL = 0;

    // let it write wherever it wants, i'm not too worried about rogue DMA xfers
    REG_MDAPRO = 0x6155407f;

    // construct packet
    static char unsigned volatile cond[1024];
    static unsigned volatile frame[36 + 31];

    unsigned volatile *framep = (unsigned*)MAKE_PHYS(align32(frame));
    char unsigned volatile *condp = (char unsigned*)MAKE_PHYS(align32(cond));

    framep[0] = 0x80000001;
    framep[1] = ((unsigned)condp) & 0x1fffffff;
    framep[2] = 0x01002009;
    framep[3] = 0x01000000;

    // set SB_MDSTAR to the address of the packet
    REG_MDSTAR = ((unsigned)framep) & 0x1fffffff;

    // enable maple DMA
    REG_MDEN = 1;

    // begin the transfer
    REG_MDST = 1;

    wait_maple();

    // transfer is now complete, receive data
    return ((unsigned)condp[8]) | (((unsigned)condp[9]) << 8);
}

/*
 * our entry point (after _start).
 *
 * I had to call this dcmain because the linker kept wanting to put main at the
 * entry instead of _start, and this was the only thing I tried that actually
 * fixed it.
 */

#define N_CHAR_ROWS MAX_CHARS_Y
#define N_CHAR_COLS MAX_CHARS_X

static char const *hexstr(unsigned val) {
    static char txt[8];
    unsigned nib_no;
    for (nib_no = 0; nib_no < 8; nib_no++) {
        unsigned shift_amt = (7 - nib_no) * 4;
        unsigned nibble = (val >> shift_amt) & 0xf;
        switch (nibble) {
        case 0:
            txt[nib_no] = '0';
            break;
        case 1:
            txt[nib_no] = '1';
            break;
        case 2:
            txt[nib_no] = '2';
            break;
        case 3:
            txt[nib_no] = '3';
            break;
        case 4:
            txt[nib_no] = '4';
            break;
        case 5:
            txt[nib_no] = '5';
            break;
        case 6:
            txt[nib_no] = '6';
            break;
        case 7:
            txt[nib_no] = '7';
            break;
        case 8:
            txt[nib_no] = '8';
            break;
        case 9:
            txt[nib_no] = '9';
            break;
        case 10:
            txt[nib_no] = 'A';
            break;
        case 11:
            txt[nib_no] = 'B';
            break;
        case 12:
            txt[nib_no] = 'C';
            break;
        case 13:
            txt[nib_no] = 'D';
            break;
        case 14:
            txt[nib_no] = 'E';
            break;
        default:
            txt[nib_no] = 'F';
            break;
        }
    }
    txt[8] = '\0';
    return txt;
}

int dcmain(int argc, char **argv) {
    /*
     * The reason why these big arrays are static is to save on stack space.
     * We only have 4KB!
     */
    struct msg msg;
    static char arm_msg[DATA_LEN];
    arm_msg[0] = 0;
    char const* arm7_exception = (char const*)0x0;
    unsigned arm7_excp_pc;

    static char txt_buf[N_CHAR_ROWS][N_CHAR_COLS];
    static int txt_colors[N_CHAR_ROWS][N_CHAR_COLS];

#define N_FONTS 6
    static unsigned short fonts[N_FONTS][288 * 24 * 12];

    int row, col;
    for (row = 0; row < N_CHAR_ROWS; row++)
        for (col = 0; col < N_CHAR_COLS; col++) {
            txt_buf[row][col] = '\0';
            txt_colors[row][col] = 0;
        }

    create_font(fonts[0], make_color(0, 0, 0), make_color(0, 0, 0));
    create_font(fonts[1], make_color(0, 197, 0), make_color(0, 0, 0));
    create_font(fonts[2], make_color(197, 0, 9), make_color(0, 0, 0));
    create_font(fonts[3], make_color(255, 255, 255), make_color(0, 0, 0));
    create_font(fonts[4], make_color(197, 197, 230), make_color(0, 0, 0));
    create_font(fonts[5], make_color(255, 131, 24), make_color(0, 0, 0));

    init_arm_cpu();

    configure_video();

    for (;;) {
        clear_screen(get_backbuffer(), make_color(0, 0, 0));

        void volatile *fb = get_backbuffer();
        if (arm7_exception) {
            drawstring(fb, fonts[2], "ERROR - ARM7 CPU EXCEPTION RAISED", 0, 0);
            drawstring(fb, fonts[2], arm7_exception, 1, 0);
            drawstring(fb, fonts[2], "PC=", 2, 0);
            drawstring(fb, fonts[2], hexstr(arm7_excp_pc), 2, 3);
        } else {
            int row, col;
            for (row = 0; row < N_CHAR_ROWS; row++)
                for (col = 0; col < N_CHAR_COLS; col++)
                    if (txt_buf[row][col] && txt_buf[row][col] != ' ') {
                        int col_idx = txt_colors[row][col];
                        if (col_idx >= N_FONTS)
                            col_idx = N_FONTS - 1;
                        draw_char(fb, fonts[col_idx], txt_buf[row][col], row, col);
                    }
        }

        unsigned btns = ~get_controller_buttons();

        while (!check_vblank()) {
            if (arm7_exception)
                continue;
            int idx, x_pos, y_pos, color;
            char const *inp;
            unsigned ret_val = 0;
            if (check_msg(&msg)) {
                switch (msg.opcode) {
                case ARM7_OPCODE_FIBONACCI:
                    arm7_operational = validate_fibonacci(msg.msg);
                    return_msg(0);
                    break;
                case ARM7_OPCODE_PRINT:
                    x_pos = ((int*)msg.msg)[0];
                    y_pos = ((int*)msg.msg)[1];
                    color = ((int*)msg.msg)[2];
                    for (idx = 12; idx < (DATA_LEN - 1); idx++)
                        arm_msg[idx - 12] = msg.msg[idx];
                    arm_msg[(DATA_LEN - 1)] = '\0';

                    x_pos /= 8;
                    y_pos /= 8;

                    if (y_pos >= N_CHAR_ROWS) {
                        return_msg(0);
                        break;
                    }

                    inp = arm_msg;
                    while (*inp && x_pos < N_CHAR_COLS) {
                        txt_colors[y_pos][x_pos] = color;
                        txt_buf[y_pos][x_pos++] = *inp++;
                    }
                    return_msg(0);
                    break;
                case ARM7_OPCODE_GET_BTNS:
                    ret_val = ~0;
                    // d-pad down
                    if (btns & (1 << 5))
                        ret_val &= ~0x80;

                    // d-pad up
                    if (btns & (1 << 4))
                        ret_val &= ~0x40;

                    // A button
                    if (btns & (1 << 2))
                        ret_val &= ~0x08;

                    // B button
                    if (btns & (1 << 1))
                        ret_val &= ~0x04;

                    return_msg(ret_val);
                    break;
                case ARM7_OPCODE_CLEAR_SCREEN:
                    for (y_pos = 0; y_pos < N_CHAR_ROWS; y_pos++)
                        for (x_pos = 0; x_pos < N_CHAR_COLS; x_pos++)
                            txt_buf[y_pos][x_pos] = '\0';
                    return_msg(0);
                    break;
                case ARM7_OPCODE_EXCEPTION:
                    switch (((int*)msg.msg)[0]) {
                    case 0:
                        arm7_exception = "UNDEFINED";
                        break;
                    case 1:
                        arm7_exception = "SWI";
                        break;
                    case 2:
                        arm7_exception = "PREF ABORT";
                        break;
                    case 3:
                        arm7_exception = "DATA ABORT";
                        break;
                    case 5:
                        arm7_exception = "IRQ";
                        break;
                    case 6:
                        arm7_exception = "FIQ";
                        break;
                    default:
                        arm7_exception = "UNKNOWN EXCEPTION";
                    }
                    arm7_excp_pc = ((int*)msg.msg)[1];
                    return_msg(0);
                    break;
                }
            }
        }
        swap_buffers();
    }

    return 0;
}
