#include <stdint.h>

typedef struct {
    uint32_t flags, mem_lower, mem_upper, boot_device, cmdline, mods_count, mods_addr;
    uint32_t syms[4], mmap_length, mmap_addr, drives_length, drives_addr, config_table;
    uint32_t boot_loader_name, apm_table, vbe_control_info, vbe_mode_info;
    uint16_t vbe_mode, vbe_interface_seg, vbe_interface_off, vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch, framebuffer_width, framebuffer_height;
    uint8_t framebuffer_bpp, framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

uint32_t *real_fb;
uint32_t backbuffer[800 * 600];
uint32_t *fb = backbuffer;
uint32_t pitch;
int mx = 400, my = 300;
uint8_t m_btn = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

const uint8_t font8x8[64][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},{0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},{0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},{0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},{0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},{0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},{0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00},{0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},{0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},{0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},{0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},{0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},{0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00},
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00},{0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},{0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00},
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},{0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},{0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},{0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},{0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},{0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},{0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},{0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},{0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},{0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},{0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},{0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},{0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},{0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00},{0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},{0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},{0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}
};


uint8_t read_rtc(uint8_t reg) {
    outb(0x70, reg);
    uint8_t val = inb(0x71);
    return (val & 0x0F) + ((val / 16) * 10);
}

void draw_cursor(int x, int y) {
    const uint8_t arrow[12][8] = {
        {1,0,0,0,0,0,0,0},{1,1,0,0,0,0,0,0},{1,2,1,0,0,0,0,0},{1,2,2,1,0,0,0,0},
        {1,2,2,2,1,0,0,0},{1,2,2,2,2,1,0,0},{1,2,2,2,2,2,1,0},{1,2,2,2,2,2,2,1},
        {1,2,2,2,1,1,1,1},{1,2,1,2,1,0,0,0},{1,1,0,1,2,1,0,0},{0,0,0,0,1,1,0,0}
    };
    for (int r = 0; r < 12; r++)
        for (int c = 0; c < 8; c++) {
            if (arrow[r][c] == 1) put_pixel(x + c, y + r, 0x000000);
            else if (arrow[r][c] == 2) put_pixel(x + c, y + r, 0xFFFFFF);
        }
}

void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < 800 && y >= 0 && y < 600) fb[y * 800 + x] = color;
}
void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++) put_pixel(i, j, color);
}
void draw_char(int x, int y, char c, uint32_t color) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 32 || c > 95) c = '?';
    const uint8_t *g = font8x8[c - 32];
    for (int r = 0; r < 8; r++)
        for (int col = 0; col < 8; col++)
            if (g[r] & (1 << col)) put_pixel(x + col, y + r, color);
}
void draw_str(int x, int y, const char *s, uint32_t color) {
    while (*s) { draw_char(x, y, *s++, color); x += 9; }
}
void mouse_wait(uint8_t t) {
    uint32_t to = 100000;
    if (t == 0) { while (to--) if ((inb(0x64) & 1) == 1) return; }
    else { while (to--) if ((inb(0x64) & 2) == 0) return; }
}
void mouse_write(uint8_t w) { mouse_wait(1); outb(0x64, 0xD4); mouse_wait(1); outb(0x60, w); }
uint8_t mouse_read() { mouse_wait(0); return inb(0x60); }
void init_mouse() {
    uint8_t s;
    mouse_wait(1); outb(0x64, 0xA8);
    mouse_wait(1); outb(0x64, 0x20);
    mouse_wait(0); s = (inb(0x60) | 2);
    mouse_wait(1); outb(0x64, 0x60);
    mouse_wait(1); outb(0x60, s);
    mouse_write(0xF6); mouse_read();
    mouse_write(0xF4); mouse_read();
}

const char kbd_map[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','[',']','\n',0,
    'A','S','D','F','G','H','J','K','L',';','\'','`',0,'\\',
    'Z','X','C','V','B','N','M',',','.','/',0,'*',0,' '
};

void int_to_str(int n, char *buf) {
    int i = 0, neg = 0;
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    if (n < 0) { neg = 1; n = -n; }
    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    if (neg) buf[i++] = '-';
    buf[i] = 0;
    for (int j = 0; j < i / 2; j++) { char t = buf[j]; buf[j] = buf[i - j - 1]; buf[i - j - 1] = t; }
}

void kernel_main(multiboot_info_t *mbi) {
    real_fb = (uint32_t*)(uint32_t)mbi->framebuffer_addr;
    pitch = mbi->framebuffer_pitch;
    init_mouse();

    char calc_disp[32] = "0";
    int calc_val = 0, calc_op = 0, calc_new = 1;
    char note_buf[256] = "HOLA! ESCRIBE AQUI CON EL TECLADO...";
    int note_len = 36;
    int show_calc = 1, show_note = 1;
    uint8_t prev_btn = 0, m_cycle = 0;
    int8_t m_byte[3];

    int redraw = 1;
    while (1) {
        uint8_t status = inb(0x64);
        if (status & 0x01) {
            redraw = 1;
            uint8_t data = inb(0x60);
            if (status & 0x20) {
                m_byte[m_cycle++] = (int8_t)data;
                if (m_cycle == 3) {
                    m_cycle = 0;
                    m_btn = m_byte[0] & 0x07;
                    mx += m_byte[1]; my -= m_byte[2];
                    if (mx < 0) mx = 0; if (mx > 790) mx = 790;
                    if (my < 0) my = 0; if (my > 590) my = 590;
                }
            } else if (!(data & 0x80)) {
                char c = kbd_map[data];
                if (c == '\b' && note_len > 0) note_buf[--note_len] = 0;
                else if (c >= 32 && note_len < 250) { note_buf[note_len++] = c; note_buf[note_len] = 0; }
            }
        }

        if ((m_btn & 1) && !(prev_btn & 1)) {
            if (mx >= 115 && mx <= 195 && my >= 570 && my <= 595) show_calc = !show_calc;
            if (mx >= 205 && mx <= 285 && my >= 570 && my <= 595) show_note = !show_note;
            if (show_calc && mx >= 245 && mx <= 268 && my >= 82 && my <= 105) show_calc = 0;
            if (show_note && mx >= 725 && mx <= 748 && my >= 82 && my <= 105) show_note = 0;
        }
        if (show_calc && (m_btn & 1) && !(prev_btn & 1) && mx >= 60 && mx <= 260 && my >= 180 && my <= 380) {
            int col = (mx - 60) / 50, row = (my - 180) / 50;
            const char keys[4][4] = {{'7','8','9','/'},{'4','5','6','*'},{'1','2','3','-'},{'C','0','=','+'}};
            char k = keys[row][col];
            if (k >= '0' && k <= '9') {
                if (calc_new) { calc_disp[0] = k; calc_disp[1] = 0; calc_new = 0; }
                else { int l = 0; while (calc_disp[l]) l++; if (l < 12) { calc_disp[l] = k; calc_disp[l+1] = 0; } }
            } else if (k == 'C') {
                calc_disp[0] = '0'; calc_disp[1] = 0; calc_val = 0; calc_op = 0; calc_new = 1;
            } else if (k == '+' || k == '-' || k == '*' || k == '/') {
                int v = 0; for (int i = 0; calc_disp[i]; i++) v = v * 10 + (calc_disp[i] - '0');
                calc_val = v; calc_op = k; calc_new = 1;
            } else if (k == '=') {
                int v = 0; for (int i = 0; calc_disp[i]; i++) v = v * 10 + (calc_disp[i] - '0');
                if (calc_op == '+') calc_val += v;
                if (calc_op == '-') calc_val -= v;
                if (calc_op == '*') calc_val *= v;
                if (calc_op == '/' && v != 0) calc_val /= v;
                int_to_str(calc_val, calc_disp); calc_new = 1;
            }
        }
        prev_btn = m_btn;

        if (redraw) {
        redraw = 0;
        draw_rect(0, 0, 800, 565, 0x1B365D);
        // Barra de tareas con botones de apps y reloj RTC
        draw_rect(0, 565, 800, 35, 0x0D1726);
        draw_rect(10, 570, 90, 25, 0x2B5B84);
        draw_str(24, 578, "MI OS", 0xFFFFFF);

        draw_rect(115, 570, 80, 25, show_calc ? 0x3A7CA5 : 0x1E3A5F);
        draw_str(135, 578, "CALC", 0xFFFFFF);

        draw_rect(205, 570, 80, 25, show_note ? 0x3A7CA5 : 0x1E3A5F);
        draw_str(225, 578, "BLOC", 0xFFFFFF);

        uint8_t hr = read_rtc(0x04), min = read_rtc(0x02);
        char clk[6] = {(hr/10)+'0', (hr%10)+'0', ':', (min/10)+'0', (min%10)+'0', 0};
        draw_rect(720, 570, 70, 25, 0x16263E);
        draw_str(732, 578, clk, 0x00FFAA);

        if (show_calc) {
            draw_rect(50, 80, 220, 320, 0xE2E8F0);
            draw_rect(50, 80, 220, 28, 0x2B5B84);
            draw_str(60, 90, "CALCULADORA", 0xFFFFFF);
            draw_rect(245, 82, 23, 23, 0xE53E3E);
            draw_str(252, 89, "X", 0xFFFFFF);
            draw_rect(60, 120, 200, 40, 0xFFFFFF);
            draw_str(70, 135, calc_disp, 0x000000);

            const char *labels[16] = {"7","8","9","/","4","5","6","*","1","2","3","-","C","0","=","+"};
            for (int i = 0; i < 16; i++) {
                int bx = 60 + (i % 4) * 50, by = 180 + (i / 4) * 50;
                int hover = (mx >= bx && mx <= bx + 50 && my >= by && my <= by + 50);
                draw_rect(bx + 2, by + 2, 46, 46, hover ? 0x90CDF4 : 0xCBD5E0);
                draw_str(bx + 20, by + 20, labels[i], 0x000000);
            }
        }

        if (show_note) {
            draw_rect(310, 80, 440, 420, 0xFFFFFF);
            draw_rect(310, 80, 440, 28, 0x2B5B84);
            draw_str(320, 90, "BLOC DE NOTAS", 0xFFFFFF);
            draw_rect(725, 82, 23, 23, 0xE53E3E);
            draw_str(732, 89, "X", 0xFFFFFF);
            int tx = 325, ty = 125;
            for (int i = 0; note_buf[i]; i++) {
                if (note_buf[i] == '\n' || tx > 720) { tx = 325; ty += 14; }
                if (note_buf[i] != '\n') { draw_char(tx, ty, note_buf[i], 0x000000); tx += 9; }
            }
            draw_rect(tx, ty, 8, 10, 0x2B5B84);
        }

        draw_cursor(mx, my);
        for (int y = 0; y < 600; y++) {
            uint32_t *dst = &real_fb[y * (pitch / 4)];
            uint32_t *src = &backbuffer[y * 800];
            for (int x = 0; x < 800; x++) dst[x] = src[x];
        }
        } else { __asm__ volatile ("pause"); }
    }
}
