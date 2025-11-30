
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB8000

class VGATerminal {
private:
    uint16_t* buffer;
    uint8_t color;
    int cursor_x, cursor_y;
public:
    VGATerminal() : buffer((uint16_t*)VGA_BUFFER), color(0x07), cursor_x(0), cursor_y(0) {}
    void set_color(uint8_t fg, uint8_t bg) {
        color = (bg << 4) | fg;
    }
    void clear() {
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            buffer[i] = (color << 8) | ' ';
        }
        cursor_x = cursor_y = 0;
    }
    void putchar(char c) {
        if (c == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else if (c == '\r') {
            cursor_x = 0;
        } else if (c == '\t') {
            cursor_x = (cursor_x + 8) & ~7;
        } else {
            if (cursor_x >= VGA_WIDTH) {
                cursor_x = 0;
                cursor_y++;
            }
            if (cursor_y >= VGA_HEIGHT) {
                scroll();
            }
            buffer[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | c;
            cursor_x++;
        }
    }
    void write(const char* str) {
        for (int i = 0; str[i] != '\0'; i++) {
            putchar(str[i]);
        }
    }
    void set_cursor(int x, int y) {
        cursor_x = x;
        cursor_y = y;
    }
    void draw_box(int x, int y, int w, int h, uint8_t box_color) {
        uint8_t old_color = color;
        set_color(box_color & 0x0F, (box_color >> 4) & 0x0F);
        set_cursor(x, y); putchar(0xC9);
        set_cursor(x + w - 1, y); putchar(0xBB);
        set_cursor(x, y + h - 1); putchar(0xC8);
        set_cursor(x + w - 1, y + h - 1); putchar(0xBC);
        for (int i = x + 1; i < x + w - 1; i++) {
            set_cursor(i, y); putchar(0xCD);
            set_cursor(i, y + h - 1); putchar(0xCD);
        }
        for (int i = y + 1; i < y + h - 1; i++) {
            set_cursor(x, i); putchar(0xBA);
            set_cursor(x + w - 1, i); putchar(0xBA);
        }
        color = old_color;
    }
    void fill_line(int y, uint8_t line_color, char fill_char = ' ') {
        uint8_t old_color = color;
        set_color(line_color & 0x0F, (line_color >> 4) & 0x0F);
        set_cursor(0, y);
        for (int x = 0; x < VGA_WIDTH; x++) {
            putchar(fill_char);
        }
        color = old_color;
    }
    void write_at(int x, int y, const char* str, uint8_t text_color = 0x07) {
        uint8_t old_color = color;
        set_color(text_color & 0x0F, (text_color >> 4) & 0x0F);
        set_cursor(x, y);
        write(str);
        color = old_color;
    }
    void fill_rect(int x, int y, int w, int h, uint8_t rect_color, char fill_char = ' ') {
        uint8_t old_color = color;
        set_color(rect_color & 0x0F, (rect_color >> 4) & 0x0F);
        for (int row = y; row < y + h; row++) {
            set_cursor(x, row);
            for (int col = 0; col < w; col++) {
                putchar(fill_char);
            }
        }
        color = old_color;
    }
private:
    void scroll() {
        for (int y = 0; y < VGA_HEIGHT - 1; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                buffer[y * VGA_WIDTH + x] = buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
        for (int x = 0; x < VGA_WIDTH; x++) {
            buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (color << 8) | ' ';
        }
        cursor_y--;
    }
};

class Keyboard {
private:
    static uint8_t inb(uint16_t port) {
        uint8_t result;
        asm volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
        return result;
    }
    static bool left_shift, right_shift, left_ctrl, caps_lock;
public:
    static bool is_key_pressed() {
        return (inb(0x64) & 1) != 0;
    }
    static uint8_t get_scancode() {
        return inb(0x60);
    }
    static char get_char() {
        uint8_t sc = get_scancode();
        
        if (sc == 0x2A) { left_shift = true; return 0; }
        if (sc == 0xAA) { left_shift = false; return 0; }
        if (sc == 0x36) { right_shift = true; return 0; }
        if (sc == 0xB6) { right_shift = false; return 0; }
        if (sc == 0x1D) { left_ctrl = true; return 0; }
        if (sc == 0x9D) { left_ctrl = false; return 0; }
        if (sc == 0x3A) { caps_lock = !caps_lock; return 0; }
        if (sc & 0x80) return 0;

        bool is_shifted = (left_shift || right_shift) ^ caps_lock;

        static const char normal[58] = {
            0, 0,
            '1','2','3','4','5','6','7','8','9','0','-','=',
            '\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
            0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
            0, '\\','z','x','c','v','b','n','m',',','.','/',
            0, '*', 0, ' '
        };
        static const char shifted_layout[58] = {
            0, 0,
            '!','@','#','$','%','^','&','*','(',')','_','+',
            '\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
            0, 'A','S','D','F','G','H','J','K','L',':','"','~',
            0, '|','Z','X','C','V','B','N','M','<','>','?',
            0, '*', 0, ' '
        };

        if (sc >= 0x3B && sc <= 0x44) return 0xF1 + (sc - 0x3B);
        
        if (sc < 58) {
            if (is_shifted && shifted_layout[sc]) {
                return shifted_layout[sc];
            }
            return normal[sc];
        }
        if (sc == 0x01) return 27;
        if (sc == 0x0E) return '\b';
        if (sc == 0x1C) return '\n';
        if (sc == 0x0F) return '\t';
        
        return 0;
    }
    static bool is_ctrl_pressed() { return left_ctrl; }
};
bool Keyboard::left_shift = false;
bool Keyboard::right_shift = false;
bool Keyboard::left_ctrl = false;
bool Keyboard::caps_lock = false;

class TextEditor {
private:
    VGATerminal& term;
    char buffer[2048];
    int cursor;
    bool active;
public:
    TextEditor(VGATerminal& t) : term(t), cursor(0), active(false) {
        for (int i = 0; i < 2048; i++) buffer[i] = 0;
    }
    void open() {
        active = true;
        cursor = 0;
        term.set_color(0x0F, 0x01);
        term.clear();
        draw_ui();
    }
    void close() {
        active = false;
    }
    bool is_active() { return active; }
    void handle_input(char c) {
        if (!active) return;
        
        if (c == (char)0xF1) {
            close();
            return;
        }
        
        if (c == '\b') {
            if (cursor > 0) {
                cursor--;
                buffer[cursor] = 0;
            }
        } else if (c == '\n') {
            if (cursor < 2047) {
                buffer[cursor] = '\n';
                cursor++;
                buffer[cursor] = 0;
            }
        } else if (c >= 32 && c <= 126 && cursor < 2047) {
            buffer[cursor] = c;
            cursor++;
            buffer[cursor] = 0;
        }
        draw_content();
    }
    void draw_ui() {
        term.set_color(0x0F, 0x01);
        term.draw_box(1, 1, 78, 21, 0x3F);
        term.write_at(5, 2, "Text Editor - Press F1 to exit", 0x3F);
        term.write_at(60, 2, "F1:Exit", 0x3F);
        draw_content();
    }
    void draw_content() {
        term.fill_rect(3, 4, 74, 16, 0x17, ' ');
        term.write_at(3, 4, "Text:", 0x0F);
        
        int line = 6;
        int col = 3;
        for (int i = 0; i < cursor && line < 20; i++) {
            if (buffer[i] == '\n' || col >= 76) {
                line++;
                col = 3;
                if (line >= 20) break;
            }
            if (buffer[i] != '\n') {
                term.write_at(col, line, &buffer[i], 0x0F);
                col++;
            }
        }
        
        term.write_at(3 + (cursor % 73), 6 + (cursor / 73), "_", 0x0F);
        
        char info[32];
        int_to_str(cursor, info);
        term.write_at(3, 21, "Length: ", 0x0F);
        term.write_at(11, 21, info, 0x0F);
        term.write_at(20, 21, "bytes", 0x0F);
    }
private:
    void int_to_str(int num, char* str) {
        if (num == 0) {
            str[0] = '0';
            str[1] = '\0';
            return;
        }
        int i = 0;
        int n = num;
        while (n > 0) {
            n /= 10;
            i++;
        }
        str[i] = '\0';
        n = num;
        while (n > 0) {
            str[--i] = '0' + (n % 10);
            n /= 10;
        }
    }
};

class Calculator {
private:
    VGATerminal& term;
    char display[32];
    int value;
    char operation;
    int operand;
    bool active;
    bool new_input;
public:
    Calculator(VGATerminal& t) : term(t), value(0), operation(0), operand(0), active(false), new_input(true) {
        for (int i = 0; i < 32; i++) display[i] = 0;
        display[0] = '0';
    }
    void open() {
        active = true;
        value = 0;
        operation = 0;
        operand = 0;
        new_input = true;
        display[0] = '0';
        display[1] = 0;
        term.set_color(0x0F, 0x01);
        term.clear();
        draw_ui();
    }
    void close() {
        active = false;
    }
    bool is_active() { return active; }
    void handle_input(char c) {
        if (!active) return;
        
        if (c == (char)0xF2) {
            close();
            return;
        }
        
        if (c == 'c' || c == 'C') {
            value = 0;
            operand = 0;
            operation = 0;
            new_input = true;
            display[0] = '0';
            display[1] = 0;
        } else if (c >= '0' && c <= '9') {
            if (new_input || (display[0] == '0' && display[1] == 0)) {
                display[0] = c;
                display[1] = 0;
                new_input = false;
            } else {
                int len = strlen(display);
                if (len < 30) {
                    display[len] = c;
                    display[len + 1] = 0;
                }
            }
            value = atoi(display);
        } else if (c == '+' || c == '-' || c == '*' || c == '/') {
            if (operation != 0) {
                calculate();
            }
            operation = c;
            operand = value;
            new_input = true;
        } else if (c == '=' || c == '\n') {
            calculate();
            operation = 0;
            new_input = true;
        }
        draw_display();
    }
    void calculate() {
        switch (operation) {
            case '+': value = operand + value; break;
            case '-': value = operand - value; break;
            case '*': value = operand * value; break;
            case '/': 
                if (value != 0) value = operand / value;
                break;
        }
        int_to_str(value, display);
    }
    void draw_ui() {
        term.set_color(0x0F, 0x01);
        term.draw_box(10, 5, 60, 15, 0x2F);
        term.write_at(15, 6, "Calculator - Press F2 to exit", 0x2F);
        term.write_at(55, 6, "F2:Exit", 0x2F);
        draw_display();
        draw_buttons();
    }
    void draw_display() {
        term.set_color(0x00, 0x07);
        term.fill_rect(12, 8, 56, 1, 0x70, ' ');
        term.write_at(12, 8, display, 0x70);
        
        if (operation != 0) {
            char op_str[2] = {operation, 0};
            term.write_at(68, 8, op_str, 0x70);
        }
    }
    void draw_buttons() {
        const char* buttons = "789/456*123-0C=+";
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                int x = 12 + col * 14;
                int y = 10 + row * 2;
                term.set_color(0x00, 0x07);
                term.draw_box(x, y, 12, 1, 0x70);
                char button[2] = {buttons[row * 4 + col], 0};
                term.write_at(x + 5, y, button, 0x70);
            }
        }
    }
private:
    int strlen(const char* str) {
        int len = 0;
        while (str[len]) len++;
        return len;
    }
    int atoi(const char* str) {
        int result = 0;
        for (int i = 0; str[i] >= '0' && str[i] <= '9'; i++) {
            result = result * 10 + (str[i] - '0');
        }
        return result;
    }
    void int_to_str(int num, char* str) {
        if (num == 0) {
            str[0] = '0';
            str[1] = '\0';
            return;
        }
        int i = 0;
        int n = num;
        while (n > 0) {
            n /= 10;
            i++;
        }
        str[i] = '\0';
        n = num;
        while (n > 0) {
            str[--i] = '0' + (n % 10);
            n /= 10;
        }
    }
};

class Desktop {
private:
    VGATerminal term;
    TextEditor editor;
    Calculator calculator;
    enum App { NONE, EDITOR, CALCULATOR} current_app;
    char input_buffer[128];
    int cursor;
    bool show_help;
public:
    Desktop() : editor(term), calculator(term), 
                current_app(NONE), cursor(0), show_help(false) {
        for (int i = 0; i < 128; i++) input_buffer[i] = 0;
    }
    void run() {
        term.set_color(0x0F, 0x01);
        term.clear();
        draw_desktop();
        while (true) {
            draw_taskbar();
            if (current_app == NONE) {
                handle_shell_input();
            } else {
                handle_app_input();
            }
            for (volatile int i = 0; i < 50000; i++);
        }
    }
    void handle_shell_input() {
        if (Keyboard::is_key_pressed()) {
            char c = Keyboard::get_char();
            if (c != 0) {
                if (c == '\n') {
                    execute_command();
                    cursor = 0;
                    for (int i = 0; i < 128; i++) input_buffer[i] = 0;
                } else if (c == '\b') {
                    if (cursor > 0) {
                        cursor--;
                        input_buffer[cursor] = 0;
                    }
                } else if (c == '\t') {
                    if (strcmp(input_buffer, "ed") == 0) strcpy(input_buffer, "edit");
                    else if (strcmp(input_buffer, "cal") == 0) strcpy(input_buffer, "calc");
                    cursor = strlen(input_buffer);
                } else if (c >= 32 && c <= 126 && cursor < 127) {
                    input_buffer[cursor] = c;
                    cursor++;
                    input_buffer[cursor] = 0;
                } else if (c == (char)0xF1 || c == (char)0xF2) {
                    switch ((uint8_t)c) {
                        case 0xF1: strcpy(input_buffer, "edit"); execute_command(); break;
                        case 0xF2: strcpy(input_buffer, "calc"); execute_command(); break;
                    }
                }
                draw_shell();
            }
        }
    }
    void handle_app_input() {
        if (Keyboard::is_key_pressed()) {
            char c = Keyboard::get_char();
            if (c != 0) {
                switch (current_app) {
                    case EDITOR:
                        editor.handle_input(c);
                        if (!editor.is_active()) {
                            current_app = NONE;
                            term.set_color(0x0F, 0x01);
                            term.clear();
                            draw_desktop();
                        }
                        break;
                    case CALCULATOR:
                        calculator.handle_input(c);
                        if (!calculator.is_active()) {
                            current_app = NONE;
                            term.set_color(0x0F, 0x01);
                            term.clear();
                            draw_desktop();
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
    void execute_command() {
        if (strcmp(input_buffer, "edit") == 0 || strcmp(input_buffer, "editor") == 0) {
            current_app = EDITOR;
            editor.open();
        } else if (strcmp(input_buffer, "calc") == 0 || strcmp(input_buffer, "calculator") == 0) {
            current_app = CALCULATOR;
            calculator.open();
        } else if (strcmp(input_buffer, "help") == 0 || strcmp(input_buffer, "?") == 0) {
            show_help = !show_help;
            draw_desktop();
        } else if (strcmp(input_buffer, "clear") == 0 || strcmp(input_buffer, "cls") == 0) {
            term.set_color(0x0F, 0x01);
            term.clear();
            draw_desktop();
        } else if (strcmp(input_buffer, "about") == 0) {
            draw_about();
        } else if (input_buffer[0] != 0) {
            term.write_at(2, 18, "Unknown command. Type 'help' for commands", 0x0F);
        }
    }
    void draw_desktop() {
        term.set_color(0x0F, 0x01);
        term.draw_box(0, 0, 80, 22, 0x3F);
        term.write_at(2, 1, "Event Horizon Kernel 3!", 0x3F);
        term.write_at(50, 1, "F1:Edit F2:Calc", 0x3F);
        
        term.write_at(2, 3, "Welcome to Event Horizon DSB!", 0x0F);
        term.write_at(2, 6, "Available applications:", 0x0F);
        term.write_at(2, 8, "  edit     - Text Editor", 0x0F);
        term.write_at(2, 9, "  calc     - Calculator", 0x0F);
        term.write_at(2, 11, "  help     - Show this help", 0x0F);
        term.write_at(2, 12, "  clear    - Clear screen", 0x0F);
        term.write_at(2, 13, "  about    - System information", 0x0F);
        
        if (show_help) {
            term.draw_box(15, 5, 50, 12, 0x2F);
            term.write_at(17, 6, "Event Horizon DSB - Help System", 0x2F);
            term.write_at(17, 8, "Quick Keys:", 0x0F);
            term.write_at(17, 9, "  F1 - Open Text Editor", 0x0F);
            term.write_at(17, 10, "  F2 - Open Calculator", 0x0F);
            term.write_at(17, 13, "In Apps: Use same function key to exit", 0x0F);
        }
        
        draw_shell();
    }
    void draw_about() {
        term.fill_rect(10, 5, 60, 15, 0x17, ' ');
        term.draw_box(10, 5, 60, 15, 0x5F);
        term.write_at(12, 6, "Event Horizon DSB - About", 0x5F);
        term.write_at(12, 8, "Advanced Microkernel System", 0x0F);
        term.write_at(12, 10, "Features:", 0x0F);
        term.write_at(12, 14, "- Keyboard Driver", 0x0F);
        term.write_at(12, 15, "- Shell with Tab-completion", 0x0F);
        
        while (!Keyboard::is_key_pressed()) {}
        while (Keyboard::is_key_pressed()) { Keyboard::get_char(); }
        
        draw_desktop();
    }
    void draw_shell() {
        term.write_at(2, 20, "ehdsb> ", 0x0F);
        term.write_at(8, 20, input_buffer, 0x0F);
        term.set_cursor(8 + strlen(input_buffer), 20);
        term.putchar('_');
    }
    void draw_taskbar() {
        term.fill_line(22, 0x70, ' ');
        term.write_at(2, 22, "EH DSB | F1:Editor F2:Calc", 0x70);
        
        const char* status = "Ready";
        if (current_app == EDITOR) {
            status = "Text Editor - Press F1 to exit";
        } else if (current_app == CALCULATOR) {
            status = "Calculator - Press F2 to exit";
        }
        term.write_at(50, 22, status, 0x70);
        
        char pos_str[16];
        int_to_str(cursor, pos_str);
        term.write_at(70, 22, pos_str, 0x70);
    }
private:
    int strlen(const char* str) {
        int len = 0;
        while (str[len]) len++;
        return len;
    }
    int strcmp(const char* a, const char* b) {
        while (*a && *b && *a == *b) { a++; b++; }
        return *a - *b;
    }
    void strcpy(char* dest, const char* src) {
        while (*src) {
            *dest++ = *src++;
        }
        *dest = 0;
    }
    void int_to_str(int num, char* str) {
        if (num == 0) {
            str[0] = '0';
            str[1] = '\0';
            return;
        }
        int i = 0;
        int n = num;
        while (n > 0) {
            n /= 10;
            i++;
        }
        str[i] = '\0';
        n = num;
        while (n > 0) {
            str[--i] = '0' + (n % 10);
            n /= 10;
        }
    }
};

extern "C" void kernel_main() {
    extern uint32_t _sbss, _ebss;
    uint32_t* p = &_sbss;
    while (p < &_ebss) *p++ = 0;

    uint16_t* video = (uint16_t*)VGA_BUFFER;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i] = (0x1F << 8) | ' ';
    }
    const char* msg = "EH DSB Loading...";
    for (int i = 0; msg[i]; i++) {
        video[i] = (0x1F << 8) | msg[i];
    }

    Desktop desktop;
    desktop.run();

    while (1) {
        asm volatile("hlt");
    }
}

extern "C" void __cxa_pure_virtual() {
    while(1) {}
}
extern "C" void __stack_chk_fail() {
    while(1) {}
}
