#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

#define PIN_DHT22      2
#define PIN_OLED_SDA   4
#define PIN_OLED_SCL   5
#define PIN_LCD_RS     8
#define PIN_LCD_E      9
#define PIN_LCD_D4     10
#define PIN_LCD_D5     11
#define PIN_LCD_D6     12
#define PIN_LCD_D7     13
#define PIN_LED_V      14
#define PIN_LED_R      15
#define PIN_RELEU      16
#define PIN_BUZZER     17
#define PIN_ADC_BTNS   26

#define I2C_PORT       i2c0
#define OLED_ADDR      0x3C

float temp_minima = 20.0f;
float temp_maxima = 26.0f;
float umid_maxima = 60.0f;

const uint8_t font_basic[17][8] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00, 0x00}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46, 0x00, 0x00, 0x00}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31, 0x00, 0x00, 0x00}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00, 0x00}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00, 0x00}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00, 0x00}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00, 0x00}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00, 0x00}, // 9
    {0x00, 0x00, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00}, // :
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00}, // . (punct)
    {0x01, 0x01, 0x7F, 0x01, 0x01, 0x00, 0x00, 0x00}, // T
    {0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00, 0x00, 0x00}, // M
    {0x23, 0x13, 0x08, 0x64, 0x62, 0x00, 0x00, 0x00}, // %
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // Spațiu gol
};

int get_font_idx(char c) {
    if (c >= '0' && c <= '9') 
        return c - '0';
    if (c == ':') 
        return 10;
    if (c == '-') 
        return 11;
    if (c == '.') 
        return 12;
    if (c == 'T') 
        return 13;
    if (c == 'M') 
        return 14;
    if (c == '%') 
        return 15;
    return 16;
}

void oled_cmd(uint8_t cmd) 
{
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

void oled_init() 
{
    uint8_t init_cmds[] = {
        0xAE, 0x20, 0x00, 0x40, 0xA1, 0xA8, 31, 0xC8, 
        0xD3, 0x00, 0xDA, 0x02, 0xD5, 0x80, 0xD9, 0xF1, 
        0xDB, 0x30, 0x81, 0xFF, 0xA6, 0xA4, 0x8D, 0x14, 0xAF
    };
    for (int i = 0; i < sizeof(init_cmds); i++) 
        oled_cmd(init_cmds[i]);
}

void oled_clear_pages() 
{
    uint8_t clear_buf[129];
    clear_buf[0] = 0x40;
    for(int i=1; i<=128; i++) 
        clear_buf[i] = 0x00;
    
    for (uint8_t p = 0; p < 4; p++) 
    {
        oled_cmd(0xB0 | p);
        oled_cmd(0x00);
        oled_cmd(0x10);
        i2c_write_blocking(I2C_PORT, OLED_ADDR, clear_buf, 129, false);
    }
}

void oled_print_char_8x8(uint8_t page, uint8_t col, int font_idx) 
{
    oled_cmd(0xB0 | page);
    oled_cmd(col & 0x0F);
    oled_cmd(0x10 | ((col >> 4) & 0x0F));
    
    uint8_t data_buf[9];
    data_buf[0] = 0x40;
    for(int i=0; i<8; i++) data_buf[i+1] = font_basic[font_idx][i];
    i2c_write_blocking(I2C_PORT, OLED_ADDR, data_buf, 9, false);
}

void oled_putstr(uint8_t page, uint8_t col, const char *str) 
{
    int x = col;
    while (*str) {
        int idx = get_font_idx(*str);
        oled_print_char_8x8(page, x, idx);
        x += 8; 
        str++;
    }
}

void actualizeaza_ecran_oled() 
{
    oled_clear_pages();
    
    char buf1[16];
    char buf2[16];
    
    sprintf(buf1, "T:%.1f-%.1f", temp_minima, temp_maxima);
    sprintf(buf2, "M:%.1f%%", umid_maxima);
    
    oled_putstr(0, 0, buf1); 
    oled_putstr(2, 0, buf2); 
}

void lcd_pulse_enable() 
{
    gpio_put(PIN_LCD_E, 1);
    sleep_us(1);
    gpio_put(PIN_LCD_E, 0);
    sleep_us(100);
}

void lcd_write_nibble(uint8_t nibble) 
{
    gpio_put(PIN_LCD_D4, (nibble >> 0) & 0x01);
    gpio_put(PIN_LCD_D5, (nibble >> 1) & 0x01);
    gpio_put(PIN_LCD_D6, (nibble >> 2) & 0x01);
    gpio_put(PIN_LCD_D7, (nibble >> 3) & 0x01);
    lcd_pulse_enable();
}

void lcd_send(uint8_t value, bool mode) 
{
    gpio_put(PIN_LCD_RS, mode);
    lcd_write_nibble(value >> 4);
    lcd_write_nibble(value & 0x0F);
}

void lcd_command(uint8_t cmd) 
{
    lcd_send(cmd, false);
    sleep_ms(2);
}

void lcd_write_char(uint8_t ch) 
{
    lcd_send(ch, true);
}

void lcd_clear() 
{
    lcd_command(0x01);
}

void lcd_move_to(uint8_t col, uint8_t row) 
{
    uint8_t offsets[] = {0x00, 0x40};
    lcd_command(0x80 | (col + offsets[row]));
}

void lcd_putstr(const char *str) 
{
    while (*str) 
    {
        lcd_write_char(*str++);
    }
}

void lcd_init() 
{
    gpio_init(PIN_LCD_RS); gpio_set_dir(PIN_LCD_RS, GPIO_OUT);
    gpio_init(PIN_LCD_E);  gpio_set_dir(PIN_LCD_E, GPIO_OUT);
    gpio_init(PIN_LCD_D4); gpio_set_dir(PIN_LCD_D4, GPIO_OUT);
    gpio_init(PIN_LCD_D5); gpio_set_dir(PIN_LCD_D5, GPIO_OUT);
    gpio_init(PIN_LCD_D6); gpio_set_dir(PIN_LCD_D6, GPIO_OUT);
    gpio_init(PIN_LCD_D7); gpio_set_dir(PIN_LCD_D7, GPIO_OUT);
    
    sleep_ms(50);
    lcd_write_nibble(0x03);
    sleep_ms(5);
    lcd_write_nibble(0x03);
    sleep_us(150);
    lcd_write_nibble(0x03);
    lcd_write_nibble(0x02);
    
    lcd_command(0x28); 
    lcd_command(0x0C); 
    lcd_command(0x06); 
    lcd_clear();
}

const char* citeste_buton() 
{
    uint16_t valoare = adc_read();
    if (valoare < 187)   
        return "RIGHT";
    if (valoare < 750)   
        return "UP";
    if (valoare < 2000)  
        return "DOWN";
    if (valoare < 3000)  
        return "LEFT";
    if (valoare < 4050)     
        return "SELECT";
    return "NONE";
}

bool dht22_read(float *temp, float *humidity) 
{
    uint8_t data[5] = {0,0,0,0,0};
    
    gpio_set_dir(PIN_DHT22, GPIO_OUT);
    gpio_put(PIN_DHT22, 0);
    sleep_ms(18);
    gpio_put(PIN_DHT22, 1);
    sleep_us(30);
    
    gpio_set_dir(PIN_DHT22, GPIO_IN);
    
    uint32_t timeout = 10000;
    while(gpio_get(PIN_DHT22) == 1) 
    { 
        if(--timeout == 0) 
        return false; 
    }
    while(gpio_get(PIN_DHT22) == 0) 
    { 
        if(--timeout == 0) 
        return false; 
    }
    while(gpio_get(PIN_DHT22) == 1) 
    { 
        if(--timeout == 0) 
        return false; 
    }
    
    for (int i = 0; i < 40; i++) 
    {
        while(gpio_get(PIN_DHT22) == 0);
        uint32_t start_time = time_us_32();
        while(gpio_get(PIN_DHT22) == 1);
        if ((time_us_32() - start_time) > 40) 
        {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) 
    {
        *humidity = ((data[0] << 8) | data[1]) * 0.1f;
        int16_t raw_temp = ((data[2] & 0x7F) << 8) | data[3];
        if (data[2] & 0x80) raw_temp = -raw_temp;
        *temp = raw_temp * 0.1f;
        return true;
    }
    return false;
}

void set_buzzer(bool active) 
{
    uint slice_num = pwm_gpio_to_slice_num(PIN_BUZZER);
    if(active) 
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_B, 31250); // 50% Duty Cycle
    } 
    else {
        pwm_set_chan_level(slice_num, PWM_CHAN_B, 0); // Oprit
    }
}

int main() {
    stdio_init_all();
    
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(PIN_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_OLED_SDA);
    gpio_pull_up(PIN_OLED_SCL);
    oled_init();
    
    lcd_init();

    gpio_init(PIN_DHT22);
    gpio_pull_up(PIN_DHT22);

    adc_init();
    adc_gpio_init(PIN_ADC_BTNS);
    adc_select_input(0);
    
    gpio_init(PIN_LED_V); 
    gpio_set_dir(PIN_LED_V, GPIO_OUT);
    gpio_init(PIN_LED_R); 
    gpio_set_dir(PIN_LED_R, GPIO_OUT);
    gpio_init(PIN_RELEU); 
    gpio_set_dir(PIN_RELEU, GPIO_OUT);
    
    gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PIN_BUZZER);
    pwm_set_clkdiv(slice_num, 2.0f); 
    pwm_set_wrap(slice_num, 62500);  // 125MHz / (2 * 62500) = 1000Hz
    pwm_set_enabled(slice_num, true);
    set_buzzer(false);
    
    lcd_putstr("Statie Climatica");
    actualizeaza_ecran_oled();
    sleep_ms(500);

    while (true) 
    {
        for (int i = 0; i < 20; i++) 
        {
            const char* buton = citeste_buton();
            if (buton != "NONE") 
            {
                if (buton == "UP")        
                    temp_maxima += 0.5f;
                else if (buton == "DOWN")  
                    temp_maxima -= 0.5f;
                else if (buton == "RIGHT") 
                    temp_minima += 0.5f;
                else if (buton == "LEFT")  
                    temp_minima -= 0.5f;
                else if (buton == "SELECT") 
                {
                    temp_minima = 20.0f;
                    temp_maxima = 26.0f;
                    umid_maxima = 60.0f;
                    gpio_put(PIN_LED_V, 0);
                    gpio_put(PIN_LED_R, 0);
                    gpio_put(PIN_RELEU, 0);
                }
                
                if (temp_minima >= temp_maxima) 
                    temp_minima = temp_maxima - 0.5f;
                actualizeaza_ecran_oled();
                sleep_ms(500);
                break;
            }
            sleep_ms(10);
        }

        float t_actuala = 0, u_actuala = 0;
        if (dht22_read(&t_actuala, &u_actuala)) 
        {
            lcd_clear();
            lcd_move_to(0, 0);
            char buffer_lcd[16];
            sprintf(buffer_lcd, "T:%.1fC  U:%.1f%%", t_actuala, u_actuala);
            lcd_putstr(buffer_lcd);
            lcd_move_to(0, 1);
            lcd_putstr("Sistem Activ");
            
            bool temp_ok = (t_actuala >= temp_minima && t_actuala <= temp_maxima);
            bool umid_ok = (u_actuala <= umid_maxima);
            
            if (temp_ok && umid_ok) 
            {
                gpio_put(PIN_LED_V, 1);
                gpio_put(PIN_LED_R, 0);
                gpio_put(PIN_RELEU, 0);
                set_buzzer(false);
            } 
            else 
            {
                gpio_put(PIN_LED_V, 0);
                gpio_put(PIN_LED_R, 1);
                gpio_put(PIN_RELEU, 1);
                
                if (t_actuala >= (temp_maxima + 5.0f) || t_actuala <= (temp_minima - 5.0f)) 
                {
                    set_buzzer(true);
                } 
                else 
                {
                    set_buzzer(false);
                }
            }
        } 
        else 
        {
            lcd_clear();
            lcd_move_to(0, 0);
            lcd_putstr("Eroare Senzor!");
        }
    }
}