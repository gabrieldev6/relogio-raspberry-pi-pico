#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "ssd1306_font.h"


#define SSD1306_HEIGHT 64
#define SSD1306_WIDTH 128

#define SSD1306_I2C_ADDR _u(0x3C)

#define SSD1306_I2C_CLK 400
// #define SSD1306_I2C_CLK             1000

// commands (see datasheet)
#define SSD1306_SET_MEM_MODE _u(0x20)
#define SSD1306_SET_COL_ADDR _u(0x21)
#define SSD1306_SET_PAGE_ADDR _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL _u(0x26)
#define SSD1306_SET_SCROLL _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST _u(0x81)
#define SSD1306_SET_CHARGE_PUMP _u(0x8D)

#define SSD1306_SET_SEG_REMAP _u(0xA0)
#define SSD1306_SET_ENTIRE_ON _u(0xA4)
#define SSD1306_SET_ALL_ON _u(0xA5)
#define SSD1306_SET_NORM_DISP _u(0xA6)
#define SSD1306_SET_INV_DISP _u(0xA7)
#define SSD1306_SET_MUX_RATIO _u(0xA8)
#define SSD1306_SET_DISP _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV _u(0xD5)
#define SSD1306_SET_PRECHARGE _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG _u(0xDA)
#define SSD1306_SET_VCOM_DESEL _u(0xDB)

#define SSD1306_PAGE_HEIGHT _u(8)
#define SSD1306_NUM_PAGES (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE _u(0xFE)
#define SSD1306_READ_MODE _u(0xFF)

#define i2c_PORT i2c1
#define SDA 14
#define SCL 15
#define BT_A 5
#define BT_B 6

struct render_area
{
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
};

void calc_render_area_buflen(struct render_area *area)
{
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

#ifdef i2c_default

void SSD1306_send_cmd(uint8_t cmd)
{

    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_PORT, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num)
{
    for (int i = 0; i < num; i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen)
{

    uint8_t *temp_buf = malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf + 1, buf, buflen);

    i2c_write_blocking(i2c_PORT, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);

    free(temp_buf);
}

void SSD1306_init()
{

    uint8_t cmds[] = {
        SSD1306_SET_DISP, // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE, // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                 // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number.
                                        // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV, // set display clock divide ratio
        0x80,                     // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,    // set pre-charge period
        0xF1,                     // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,   // set VCOMH deselect level
        0x30,                     // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST, // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,     // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,     // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,   // set charge pump
        0x14,                      // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00, // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01,   // turn display on
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_scroll(bool on)
{
    // configure horizontal scrolling
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00,                                // dummy byte
        0x00,                                // start page 0
        0x00,                                // time interval
        0x03,                                // end page 3 SSD1306_NUM_PAGES ??
        0x00,                                // dummy byte
        0xFF,                                // dummy byte
        SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void render(uint8_t *buf, struct render_area *area)
{
    // update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page};

    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

static void SetPixel(uint8_t *buf, int x, int y, bool on)
{
    assert(x >= 0 && x < SSD1306_WIDTH && y >= 0 && y < SSD1306_HEIGHT);

    

    const int BytesPerRow = SSD1306_WIDTH; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |= 1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}
// Basic Bresenhams.
static void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on)
{

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (true)
    {
        SetPixel(buf, x0, y0, on);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static inline int GetFontIndex(uint8_t ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return ch - 'A' + 1;
    }
    else if (ch >= '0' && ch <= '9')
    {
        return ch - '0' + 27;
    }
    else
        return 0; // Not got that char so space.
}

static void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch)
{
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    // For the moment, only write on Y row boundaries (every 8 vertical pixels)
    y = y / 8;

    ch = toupper(ch);
    int idx = GetFontIndex(ch);
    int fb_idx = y * 128 + x;

    for (int i = 0; i < 8; i++)
    {
        buf[fb_idx++] = font[idx * 8 + i];
    }
}

static void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str)
{
    // Cull out any string off the screen
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    while (*str)
    {
        WriteChar(buf, x, y, *str++);
        x += 8;
    }
}

char *update_time()
{
    static int seconds = 0;
    static int minutes = 0;
    static int hours = 0;
    char *text[1];
    // Incrementar segundos
    seconds++;
    if (seconds >= 60)
    {
        seconds = 0;
        minutes++;
        if (minutes >= 60)
        {
            minutes = 0;
            hours++;
            if (hours >= 24)
            {
                hours = 0;
            }
        }
    }

    // Atualizar os valores no array text
    // snprintf(text[1], 11, "%02d/%02d/2025", 30, 2); // Data fixa (exemplo)
    snprintf(text[0], 9, "%02d:%02d:%02d", hours, minutes, seconds); // Hora dinâmica

    return text[0]; // Retorna o array de strings atualizado
}

#endif


int main()
{
    stdio_init_all();

#if !defined(i2c_PORT) || !defined(SDA) || !defined(SCL)
#warning i2c / SSD1306_i2d example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else
    // useful information for picotool
    bi_decl(bi_2pins_with_func(14, 15, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("SSD1306 OLED driver I2C example for the Raspberry Pi Pico"));

    // I2C is "open drain", pull ups to keep signal high when no data is being
    // sent
    i2c_init(i2c_PORT, SSD1306_I2C_CLK * 1000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    // run through the complete initialization process
    SSD1306_init();

    stdio_init_all();

    gpio_init(BT_A);
    gpio_set_dir(BT_A, GPIO_IN);
    gpio_pull_up(BT_A);

    gpio_init(BT_B);
    gpio_set_dir(BT_B, GPIO_IN);
    gpio_pull_up(BT_B);

    // Initialize render area for entire frame (SSD1306_WIDTH pixels by SSD1306_NUM_PAGES pages)
    struct render_area frame_area = {
        start_col : 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);

    // zero the entire display
    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    // render 3 cute little raspberries

    int dates[6] = {0, 0, 0, 0, 0, 0};

    int count = 0;

    int countB = 0; // é incrementado toda vez que o botao b é apertado
    int countA = 0; // é incrementado toda vez que o botao a é apertado

    int y = 0;

    char hours[12] = {};
    char date[24] = {};

    char *menu[] = {
        "   botao b",
        "   proximo",
        "",
        "   botao a",
        "   confirmar",
        "",
        "   por favor",
        "   confirme"};

    while (true)
    {
        for (uint i = 0; i < count_of(menu); i++)
        {
            WriteString(buf, 5, y, menu[i]);
            y += 8;
        }
        render(buf, &frame_area);
        y = 0;

        if (!gpio_get(BT_A))
        {
            sleep_ms(200);
            memset(buf, 0, SSD1306_BUF_LEN);
            render(buf, &frame_area);

            while (true)
            {
                char *configHD[] = {
                    " configuracao",
                    "",
                    "data",
                    date,
                    "hora",
                    hours,
                    "aperte o botao",
                    "b para iniciar"};

                for (uint i = 0; i < count_of(configHD); i++)
                {
                    WriteString(buf, 5, y, configHD[i]);
                    y += 8;
                }

                render(buf, &frame_area);
                memset(buf, 0, SSD1306_BUF_LEN);
                y = 0;

                if (!gpio_get(BT_B))
                {
                    sleep_ms(500);

                    dates[countA] = ++countB;

                    memset(buf, 0, SSD1306_BUF_LEN);
                    char *configHD[] = {
                        " configuracao",
                        "",
                        "data",
                        date,
                        "hora",
                        hours,

                    };

                    for (uint i = 0; i < 5; i++)
                    {
                        WriteString(buf, 5, y, configHD[i]);
                        y += 8;
                    }

                    render(buf, &frame_area);
                    memset(buf, 0, SSD1306_BUF_LEN);
                    y = 0;
                    snprintf(hours, sizeof(hours), "%d %d %d", dates[3], dates[4], dates[5]);
                    snprintf(date, sizeof(date), "%d %d %d", dates[0], dates[1], dates[2] + 2000);
                }
                else if (!gpio_get(BT_A))
                {
                    sleep_ms(500);

                    countB = 0;
                    ++countA;
                    dates[countA];

                    memset(buf, 0, SSD1306_BUF_LEN);
                    render(buf, &frame_area);

                    for (uint i = 0; i < count_of(configHD); i++)
                    {
                        WriteString(buf, 5, y, configHD[i]);
                        y += 8;
                    }
                    render(buf, &frame_area);
                    y = 0;
                    if (countA >= 6)
                    {

                        memset(buf, 0, SSD1306_BUF_LEN);
                        render(buf, &frame_area);
                    // fluxo de execucao principal
                    restart:

                        char *text[] = {
                            "      data",
                            "",
                            date,
                            "",
                            "      hora",
                            "",
                            hours,

                        };
                        snprintf(hours, sizeof(hours), "   %02d %02d %02d", dates[3], dates[4], dates[5]);
                        snprintf(date, sizeof(date), "   %02d %02d %04d", dates[0], dates[1], dates[2] + 2000);

                        y = 0;
                        for (uint i = 0; i < count_of(text); i++)
                        {
                            WriteString(buf, 5, y, text[i]);
                            y += 8;
                        }
                        render(buf, &frame_area);
                        sleep_ms(950);

                        memset(buf, 0, SSD1306_BUF_LEN);
                        render(buf, &frame_area);
                        // segundos
                        dates[5] = ++dates[5];
                        if (dates[5] == 60)
                        {
                            dates[5] = 0;

                            dates[4] = ++dates[4];
                        }
                        // minutos
                        if (dates[4] == 60)
                        {
                            dates[4] = 0;

                            dates[3] = ++dates[3];
                        }
                        // horas
                        if (dates[3] == 24)
                        {
                            dates[3] = 0;

                            dates[2] = ++dates[2];
                        }
                        // dias
                        if (dates[2] == 30)
                        {
                            dates[2] = 1;

                            dates[1] = ++dates[1];
                        }
                        // meses
                        if (dates[1] == 12)
                        {
                            dates[1] = 1;

                            dates[0] = ++dates[0];
                        }
                        // sleep_ms(1);

                        goto restart;
                    }
                }
            }
        }
        sleep_ms(10);
    }

#endif
    return 0;
}
