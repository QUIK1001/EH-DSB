typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed short int16_t;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB8000

#define MAX_FILES 16
#define MAX_FILE_SIZE 2048
#define FS_START 0x20000

int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

int strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == 0) return 0;
    }
    return 0;
}

void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
}

void strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
}

const char* strchr(const char* str, char ch) {
    while (*str) {
        if (*str == ch) return str;
        str++;
    }
    return 0;
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

class RTC {
private:
    static uint8_t bcd_to_bin(uint8_t bcd) {
        return (bcd >> 4) * 10 + (bcd & 0x0F);
    }
    static uint8_t read_register(uint8_t reg) {
        asm volatile("outb %0, %1" :: "a"(reg), "Nd"(0x70));
        asm volatile("inb %1, %0" : "=a"(reg) : "Nd"(0x71));
        return reg;
    }
public:
    static void get_time(uint8_t& hour, uint8_t& minute) {
        while (read_register(0x0A) & 0x80);
        minute = bcd_to_bin(read_register(0x02));
        hour = bcd_to_bin(read_register(0x04) & 0x7F);
    }
    static bool update_time() {
        static uint32_t last_ticks = 0;
        static uint32_t ticks = 0;
        ticks++;
        if (ticks - last_ticks > 180) {
            last_ticks = ticks;
            return true;
        }
        return false;
    }
};

class VGATerminal {
private:
    uint16_t* buffer;
    uint8_t color;
    int cursor_x, cursor_y;
    
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
    
    void clear_area(int x, int y, int w, int h) {
        uint8_t old_color = color;
        set_color(0x0F, 0x01);
        for (int row = y; row < y + h; row++) {
            for (int col = x; col < x + w; col++) {
                buffer[row * VGA_WIDTH + col] = (color << 8) | ' ';
            }
        }
        color = old_color;
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
        if (sc == 0x01) return 0;
        if (sc == 0x0E) return '\b';
        if (sc == 0x1C) return '\n';
        if (sc == 0x0F) return '\t';
        if (sc == 0x44) return (char)0xFA; // F10 key
        
        return 0;
    }
    
    static bool is_ctrl_pressed() { return left_ctrl; }
};

bool Keyboard::left_shift = false;
bool Keyboard::right_shift = false;
bool Keyboard::left_ctrl = false;
bool Keyboard::caps_lock = false;

struct FileEntry {
    char name[13];
    uint32_t size;
    uint32_t data_offset;
    bool used;
};

class FileSystem {
private:
    FileEntry files[MAX_FILES];
    uint8_t* fs_buffer;
    
    int find_free_file() {
        for (int i = 0; i < MAX_FILES; i++) {
            if (!files[i].used) return i;
        }
        return -1;
    }
    
    int find_file(const char* name) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].used && strcmp(files[i].name, name) == 0) {
                return i;
            }
        }
        return -1;
    }
    
public:
    FileSystem() : fs_buffer((uint8_t*)FS_START) {
        for (int i = 0; i < MAX_FILES; i++) {
            files[i].used = false;
            files[i].name[0] = 0;
            files[i].size = 0;
            files[i].data_offset = 0;
        }
        create_default_files();
    }
    
    void create_default_files() {
        create_file("README.TXT", "EH-DSB*beta_1\n\nWelcome to Event Horizon DSB!\nThis is a real operating system.\n\nCommands:\n- edit [file] - Text Editor\n- files - File Manager\n- calc - Calculator\n- brainfuck - Brainfuck IDE\n- time - Show time\n- clear - Clear screen\n- about - System info\n\nPress F1-F5 for quick access.");
        
        create_file("HELP.TXT", "EH-DSB*beta_1 Help\n\nF1 - Text Editor\nF2 - Calculator\nF3 - File Manager\nF5 - Brainfuck IDE\n\nIn Text Editor:\n- F4 - Save file\n- F1 - Exit\n\nIn Brainfuck IDE:\n- F5 - Run\n- F6 - Compile\n- F7 - Save\n- F8 - Load\n- F9 - Examples\n- F10 - Exit");
        
        create_file("SYSTEM.CFG", "[System]\nVersion=beta_1\nShell=ehdsb\nMemory=640KB\nFiles=16 max\nClock=RTC");
        
        create_file("HELLO.BF", "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.");
        create_file("CAT.BF", ",.[,.]");
        create_file("FIB.BF", ">++++++++++>+>+[\n    [+++++[>++++++++<-]>.<++++++[>--------<-]+<<<]>.>>[\n        [-]<[>+<-]>>[<<+>>-]<<<<<.>\n    ]\n]");
    }
    
    bool create_file(const char* name, const char* content) {
        int idx = find_free_file();
        if (idx == -1) return false;
        
        strcpy(files[idx].name, name);
        files[idx].size = strlen(content);
        
        uint32_t offset = 0;
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].used && i != idx) {
                offset += files[i].size;
            }
        }
        
        if (offset + files[idx].size > 0x8000) return false;
        
        files[idx].data_offset = offset;
        files[idx].used = true;
        
        uint8_t* data_ptr = fs_buffer + offset;
        for (uint32_t i = 0; i < files[idx].size; i++) {
            data_ptr[i] = content[i];
        }
        
        return true;
    }
    
    bool save_file(const char* name, const char* content, uint32_t size) {
        int idx = find_file(name);
        if (idx == -1) {
            idx = find_free_file();
            if (idx == -1) return false;
            
            strcpy(files[idx].name, name);
            files[idx].used = true;
        }
        
        files[idx].size = size;
        
        uint32_t offset = 0;
        for (int i = 0; i < MAX_FILES; i++) {
            if (i != idx && files[i].used) {
                offset += files[i].size;
            }
        }
        
        if (offset + size > 0x8000) return false;
        
        files[idx].data_offset = offset;
        
        uint8_t* data_ptr = fs_buffer + offset;
        for (uint32_t i = 0; i < size; i++) {
            data_ptr[i] = content[i];
        }
        
        return true;
    }
    
    bool load_file(const char* name, char* buffer, uint32_t& size) {
        int idx = find_file(name);
        if (idx == -1) return false;
        
        size = files[idx].size;
        if (size > MAX_FILE_SIZE) size = MAX_FILE_SIZE;
        
        uint8_t* data_ptr = fs_buffer + files[idx].data_offset;
        for (uint32_t i = 0; i < size; i++) {
            buffer[i] = data_ptr[i];
        }
        if (size < MAX_FILE_SIZE) buffer[size] = 0;
        
        return true;
    }
    
    bool delete_file(const char* name) {
        int idx = find_file(name);
        if (idx == -1) return false;
        
        files[idx].used = false;
        return true;
    }
    
    int get_file_count() {
        int count = 0;
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].used) count++;
        }
        return count;
    }
    
    FileEntry* get_file(int index) {
        int count = 0;
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].used) {
                if (count == index) return &files[i];
                count++;
            }
        }
        return nullptr;
    }
    
    int find_file_index(const char* name) {
        int count = 0;
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].used) {
                if (strcmp(files[i].name, name) == 0) return count;
                count++;
            }
        }
        return -1;
    }
};

class ClockDisplay {
private:
    VGATerminal& term;
    uint8_t last_hour, last_minute;
    char time_str[6];
    
public:
    ClockDisplay(VGATerminal& t) : term(t), last_hour(0), last_minute(0) {
        time_str[0] = '0';
        time_str[1] = '0';
        time_str[2] = ':';
        time_str[3] = '0';
        time_str[4] = '0';
        time_str[5] = 0;
    }
    
    void update() {
        if (!RTC::update_time()) return;
        
        uint8_t hour, minute;
        RTC::get_time(hour, minute);
        
        if (hour == last_hour && minute == last_minute) {
            return;
        }
        
        last_hour = hour;
        last_minute = minute;
        
        time_str[0] = '0' + (hour / 10);
        time_str[1] = '0' + (hour % 10);
        time_str[3] = '0' + (minute / 10);
        time_str[4] = '0' + (minute % 10);
        
        term.write_at(68, 0, time_str, 0x5E);
    }
    
    void draw() {
        term.write_at(68, 0, time_str, 0x5E);
    }
};

class BrainfuckInterpreter {
private:
    uint8_t memory[30000];
    uint16_t mem_ptr;
    uint16_t program_counter;
    const char* program;
    bool running;
    VGATerminal& term;
    
    int find_matching_bracket(int start, char open, char close, bool forward) {
        int depth = 1;
        int pos = start;
        while (true) {
            if (forward) {
                pos++;
            } else {
                pos--;
            }
            if (pos < 0 || program[pos] == 0) {
                return -1;
            }
            if (program[pos] == open) {
                depth++;
            } else if (program[pos] == close) {
                depth--;
            }
            if (depth == 0) {
                return pos;
            }
        }
    }
    
    char get_input() {
        term.write_at(30, 20, "Input: ", 0x0F);
        term.write_at(37, 20, "_", 0x0F);
        
        while (true) {
            if (Keyboard::is_key_pressed()) {
                char c = Keyboard::get_char();
                if (c != 0 && c >= 32 && c <= 126) {
                    term.write_at(37, 20, &c, 0x0F);
                    term.write_at(38, 20, " ", 0x0F);
                    return c;
                }
            }
        }
    }
    
public:
    BrainfuckInterpreter(VGATerminal& t) : term(t) {
        reset();
    }
    
    void reset() {
        for (int i = 0; i < 30000; i++) {
            memory[i] = 0;
        }
        mem_ptr = 0;
        program_counter = 0;
        program = nullptr;
        running = false;
    }
    
    void dump_memory(uint16_t start = 0, uint16_t count = 32) {
        char buffer[80];
        int buf_pos = 0;
        
        char addr_buf[6];
        int_to_str(start, addr_buf);
        strcpy(buffer, "Memory [");
        strcat(buffer, addr_buf);
        strcat(buffer, "]: ");
        buf_pos = strlen(buffer);
        
        for (uint16_t i = 0; i < count && i < 30000 - start; i++) {
            char hex_buf[4];
            uint8_t val = memory[start + i];
            
            char nib1 = (val >> 4) & 0x0F;
            char nib2 = val & 0x0F;
            hex_buf[0] = nib1 < 10 ? '0' + nib1 : 'A' + (nib1 - 10);
            hex_buf[1] = nib2 < 10 ? '0' + nib2 : 'A' + (nib2 - 10);
            hex_buf[2] = ' ';
            hex_buf[3] = 0;
            
            if (i == mem_ptr - start) {
                buffer[buf_pos++] = '[';
                strcpy(buffer + buf_pos, hex_buf);
                buf_pos += 2;
                buffer[buf_pos++] = ']';
                buffer[buf_pos++] = ' ';
            } else {
                strcpy(buffer + buf_pos, hex_buf);
                buf_pos += 3;
            }
        }
        buffer[buf_pos] = 0;
        
        term.write_at(2, 18, buffer, 0x0F);
    }
    
    void run(const char* code) {
        reset();
        program = code;
        running = true;
        
        term.fill_rect(2, 15, 76, 4, 0x01, ' ');
        term.write_at(2, 15, "Running Brainfuck program...", 0x0F);
        
        char output_buffer[256];
        int output_pos = 0;
        output_buffer[0] = 0;
        int steps = 0;
        
        while (running && program[program_counter] != 0 && steps < 10000) {
            char instruction = program[program_counter];
            steps++;
            
            switch (instruction) {
                case '>':
                    mem_ptr++;
                    if (mem_ptr >= 30000) mem_ptr = 0;
                    break;
                    
                case '<':
                    if (mem_ptr == 0) mem_ptr = 29999;
                    else mem_ptr--;
                    break;
                    
                case '+':
                    memory[mem_ptr]++;
                    break;
                    
                case '-':
                    memory[mem_ptr]--;
                    break;
                    
                case '.':
                    if (output_pos < 255) {
                        output_buffer[output_pos++] = memory[mem_ptr];
                        output_buffer[output_pos] = 0;
                    }
                    term.write_at(2, 16, "Output: ", 0x0F);
                    term.write_at(10, 16, output_buffer, 0x0A);
                    break;
                    
                case ',':
                    memory[mem_ptr] = get_input();
                    break;
                    
                case '[':
                    if (memory[mem_ptr] == 0) {
                        int match = find_matching_bracket(program_counter, '[', ']', true);
                        if (match != -1) {
                            program_counter = match;
                        } else {
                            running = false;
                            term.write_at(2, 17, "Error: Unmatched '['", 0x0C);
                        }
                    }
                    break;
                    
                case ']':
                    if (memory[mem_ptr] != 0) {
                        int match = find_matching_bracket(program_counter, ']', '[', false);
                        if (match != -1) {
                            program_counter = match;
                        } else {
                            running = false;
                            term.write_at(2, 17, "Error: Unmatched ']'", 0x0C);
                        }
                    }
                    break;
                    
                default:
                    break;
            }
            
            program_counter++;
            
            char status[32];
            char pc_buf[6], mp_buf[6];
            int_to_str(program_counter, pc_buf);
            int_to_str(mem_ptr, mp_buf);
            
            strcpy(status, "PC: ");
            strcat(status, pc_buf);
            strcat(status, " MP: ");
            strcat(status, mp_buf);
            strcat(status, " I: ");
            status[strlen(status)] = instruction;
            status[strlen(status)] = 0;
            
            term.write_at(2, 14, status, 0x0E);
            
            for (volatile int i = 0; i < 10000; i++);
            
            if (Keyboard::is_key_pressed()) {
                char c = Keyboard::get_char();
                if (c == (char)0xFA) { // F10 key
                    running = false;
                    term.write_at(2, 17, "Execution stopped by user (F10)", 0x0C);
                }
            }
        }
        
        if (steps >= 10000) {
            term.write_at(2, 17, "Execution limit reached (10k steps)", 0x0C);
        } else if (running) {
            term.write_at(2, 17, "Program finished successfully", 0x0A);
        }
        
        dump_memory(0, 16);
    }
    
    bool is_running() const { return running; }
    void stop() { running = false; }
};

class BrainfuckCompiler {
private:
    VGATerminal& term;
    
    void optimize_bf_code(char* bf_code) {
        int read = 0, write = 0;
        while (bf_code[read]) {
            char c = bf_code[read];
            if (c == '>' || c == '<' || c == '+' || c == '-' || 
                c == '.' || c == ',' || c == '[' || c == ']') {
                bf_code[write++] = bf_code[read];
            }
            read++;
        }
        bf_code[write] = 0;
    }
    
public:
    BrainfuckCompiler(VGATerminal& t) : term(t) {}
    
    void show_intermediate(const char* bf_code) {
        term.clear_area(2, 10, 76, 12);
        term.write_at(2, 10, "Brainfuck Intermediate Representation:", 0x0E);
        
        int line = 11;
        int indent = 0;
        
        for (int i = 0; bf_code[i] && line < 20; i++) {
            char instr = bf_code[i];
            char desc[32];
            
            switch (instr) {
                case '>': strcpy(desc, "ptr++"); break;
                case '<': strcpy(desc, "ptr--"); break;
                case '+': strcpy(desc, "(*ptr)++"); break;
                case '-': strcpy(desc, "(*ptr)--"); break;
                case '.': strcpy(desc, "putchar(*ptr)"); break;
                case ',': strcpy(desc, "*ptr = getchar()"); break;
                case '[': 
                    strcpy(desc, "while (*ptr) {");
                    indent += 2;
                    break;
                case ']':
                    indent -= 2;
                    if (indent < 0) indent = 0;
                    strcpy(desc, "}");
                    break;
                default: continue;
            }
            
            char line_buf[64];
            int pos = 0;
            for (int j = 0; j < indent; j++) {
                line_buf[pos++] = ' ';
            }
            
            char num_buf[6];
            int_to_str(i, num_buf);
            line_buf[pos++] = ' ';
            strcpy(line_buf + pos, num_buf);
            pos += strlen(num_buf);
            line_buf[pos++] = ':';
            line_buf[pos++] = ' ';
            line_buf[pos++] = instr;
            line_buf[pos++] = ' ';
            line_buf[pos++] = '=';
            line_buf[pos++] = ' ';
            strcpy(line_buf + pos, desc);
            
            term.write_at(2, line++, line_buf, 0x0F);
        }
    }
    
    bool compile_to_c(const char* bf_code, char* output, uint32_t max_size) {
        char optimized[1024];
        strcpy(optimized, bf_code);
        optimize_bf_code(optimized);
        
        int out_pos = 0;
        
        const char* header = 
            "/* Generated by EH-DSB Brainfuck */\n"
            "#include <stdio.h>\n\n"
            "int main() {\n"
            "    unsigned char tape[30000] = {0};\n"
            "    unsigned char* ptr = tape;\n\n";
        
        strcpy(output + out_pos, header);
        out_pos += strlen(header);
        
        int loop_depth = 0;
        uint32_t max = max_size;
        
        for (int i = 0; optimized[i] && (uint32_t)out_pos < max - 100; i++) {
            char instr = optimized[i];
            
            for (int j = 0; j < loop_depth + 1; j++) {
                if ((uint32_t)out_pos < max - 2) {
                    output[out_pos++] = ' ';
                    output[out_pos++] = ' ';
                }
            }
            
            switch (instr) {
                case '>':
                    output[out_pos++] = '+';
                    output[out_pos++] = '+';
                    output[out_pos++] = 'p';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'r';
                    output[out_pos++] = ';';
                    output[out_pos++] = '\n';
                    break;
                    
                case '<':
                    output[out_pos++] = '-';
                    output[out_pos++] = '-';
                    output[out_pos++] = 'p';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'r';
                    output[out_pos++] = ';';
                    output[out_pos++] = '\n';
                    break;
                    
                case '+':
                    output[out_pos++] = '+';
                    output[out_pos++] = '+';
                    output[out_pos++] = '(';
                    output[out_pos++] = '*';
                    output[out_pos++] = 'p';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'r';
                    output[out_pos++] = ')';
                    output[out_pos++] = ';';
                    output[out_pos++] = '\n';
                    break;
                    
                case '-':
                    output[out_pos++] = '-';
                    output[out_pos++] = '-';
                    output[out_pos++] = '(';
                    output[out_pos++] = '*';
                    output[out_pos++] = 'p';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'r';
                    output[out_pos++] = ')';
                    output[out_pos++] = ';';
                    output[out_pos++] = '\n';
                    break;
                    
                case '.':
                    output[out_pos++] = 'p';
                    output[out_pos++] = 'u';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'c';
                    output[out_pos++] = 'h';
                    output[out_pos++] = 'a';
                    output[out_pos++] = 'r';
                    output[out_pos++] = '(';
                    output[out_pos++] = '*';
                    output[out_pos++] = 'p';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'r';
                    output[out_pos++] = ')';
                    output[out_pos++] = ';';
                    output[out_pos++] = '\n';
                    break;
                    
                case ',':
                    output[out_pos++] = '*';
                    output[out_pos++] = 'p';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'r';
                    output[out_pos++] = ' ';
                    output[out_pos++] = '=';
                    output[out_pos++] = ' ';
                    output[out_pos++] = 'g';
                    output[out_pos++] = 'e';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'c';
                    output[out_pos++] = 'h';
                    output[out_pos++] = 'a';
                    output[out_pos++] = 'r';
                    output[out_pos++] = '(';
                    output[out_pos++] = ')';
                    output[out_pos++] = ';';
                    output[out_pos++] = '\n';
                    break;
                    
                case '[':
                    output[out_pos++] = 'w';
                    output[out_pos++] = 'h';
                    output[out_pos++] = 'i';
                    output[out_pos++] = 'l';
                    output[out_pos++] = 'e';
                    output[out_pos++] = ' ';
                    output[out_pos++] = '(';
                    output[out_pos++] = '*';
                    output[out_pos++] = 'p';
                    output[out_pos++] = 't';
                    output[out_pos++] = 'r';
                    output[out_pos++] = ')';
                    output[out_pos++] = ' ';
                    output[out_pos++] = '{';
                    output[out_pos++] = '\n';
                    loop_depth++;
                    break;
                    
                case ']':
                    loop_depth--;
                    if (loop_depth < 0) {
                        return false;
                    }
                    out_pos -= (loop_depth + 1) * 2 + 2;
                    output[out_pos++] = '}';
                    output[out_pos++] = '\n';
                    break;
            }
            
            output[out_pos] = 0;
        }
        
        if (loop_depth != 0) {
            return false;
        }
        
        strcpy(output + out_pos, "\n    return 0;\n}\n");
        
        return true;
    }
    
    void show_statistics(const char* bf_code) {
        int counts[256] = {0};
        for (int i = 0; bf_code[i]; i++) {
            counts[(uint8_t)bf_code[i]]++;
        }
        
        term.clear_area(2, 10, 76, 8);
        term.write_at(2, 10, "Brainfuck Statistics:", 0x0E);
        
        int line = 11;
        const char* ops = "><+-.,[]";
        const char* names[] = {"Move Right", "Move Left", "Increment", "Decrement", 
                              "Output", "Input", "Loop Start", "Loop End"};
        
        for (int i = 0; ops[i]; i++) {
            char stat[32];
            char count_buf[6];
            int_to_str(counts[(uint8_t)ops[i]], count_buf);
            
            strcpy(stat, names[i]);
            strcat(stat, " (");
            stat[strlen(stat)] = ops[i];
            strcat(stat, "): ");
            strcat(stat, count_buf);
            
            term.write_at(2, line++, stat, 0x0F);
        }
        
        char total_buf[32];
        int total = 0;
        for (int i = 0; ops[i]; i++) {
            total += counts[(uint8_t)ops[i]];
        }
        char total_str[6];
        int_to_str(total, total_str);
        strcpy(total_buf, "Total BF instructions: ");
        strcat(total_buf, total_str);
        term.write_at(2, line, total_buf, 0x0E);
    }
};

class TextEditor {
private:
    VGATerminal& term;
    FileSystem& fs;
    char buffer[MAX_FILE_SIZE];
    int cursor;
    bool active;
    char current_filename[13];
    
public:
    TextEditor(VGATerminal& t, FileSystem& f) : term(t), fs(f), cursor(0), active(false) {
        current_filename[0] = 0;
        for (int i = 0; i < MAX_FILE_SIZE; i++) buffer[i] = 0;
    }
    
    void open(const char* filename = nullptr) {
        active = true;
        cursor = 0;
        buffer[0] = 0;
        
        if (filename && filename[0]) {
            strcpy(current_filename, filename);
            
            uint32_t size;
            if (fs.load_file(filename, buffer, size)) {
                cursor = size;
                if (cursor >= MAX_FILE_SIZE - 1) cursor = MAX_FILE_SIZE - 2;
                buffer[cursor] = 0;
            }
        } else {
            current_filename[0] = 0;
        }
        
        term.set_color(0x0F, 0x01);
        term.clear();
        draw_ui();
        draw_content();
    }
    
    void close() { active = false; }
    bool is_active() { return active; }
    
    void handle_input(char c) {
        if (!active) return;
        
        if (c == (char)0xF1) {
            close();
            return;
        }
        
        if (c == (char)0xF4) {
            save_file();
            return;
        }
        
        if (c == '\b') {
            if (cursor > 0) {
                cursor--;
                buffer[cursor] = 0;
                draw_content();
            }
        } else if (c == '\n') {
            if (cursor < MAX_FILE_SIZE - 2) {
                buffer[cursor] = '\n';
                cursor++;
                buffer[cursor] = 0;
                draw_content();
            }
        } else if (c >= 32 && c <= 126 && cursor < MAX_FILE_SIZE - 2) {
            buffer[cursor] = c;
            cursor++;
            buffer[cursor] = 0;
            draw_content();
        }
    }
    
    void save_file() {
        term.fill_rect(20, 10, 40, 5, 0x17, ' ');
        term.draw_box(20, 10, 40, 5, 0x2F);
        
        if (current_filename[0] == 0) {
            term.write_at(22, 11, "Enter filename:", 0x2F);
            term.write_at(22, 12, "> ", 0x0F);
            
            char filename[13] = {0};
            int pos = 0;
            
            while (true) {
                if (Keyboard::is_key_pressed()) {
                    char c = Keyboard::get_char();
                    if (c != 0) {
                        if (c == '\n') {
                            filename[pos] = 0;
                            break;
                        } else if (c == '\b') {
                            if (pos > 0) {
                                pos--;
                                term.write_at(24 + pos, 12, " ", 0x0F);
                            }
                        } else if (c >= 32 && c <= 126 && pos < 12) {
                            filename[pos] = c;
                            pos++;
                            char ch_str[2] = {c, 0};
                            term.write_at(24 + pos - 1, 12, ch_str, 0x0F);
                        }
                    }
                }
            }
            
            if (filename[0]) {
                strcpy(current_filename, filename);
            } else {
                term.fill_rect(20, 10, 40, 5, 0x17, ' ');
                draw_ui();
                draw_content();
                return;
            }
        }
        
        if (fs.save_file(current_filename, buffer, cursor)) {
            term.write_at(22, 13, "Saved successfully!", 0x0A);
        } else {
            term.write_at(22, 13, "Save failed!", 0x0C);
        }
        
        for (volatile int i = 0; i < 1000000; i++);
        
        draw_ui();
        draw_content();
    }
    
    void draw_ui() {
        term.set_color(0x0F, 0x01);
        term.draw_box(1, 1, 78, 21, 0x3F);
        
        char title[64];
        if (current_filename[0]) {
            strcpy(title, "Text Editor - ");
            strcat(title, current_filename);
        } else {
            strcpy(title, "Text Editor - New File");
        }
        
        term.write_at(5, 2, title, 0x3F);
        term.write_at(55, 2, "F1:Exit F4:Save", 0x3F);
    }
    
    void draw_content() {
        term.fill_rect(3, 4, 74, 16, 0x17, ' ');
        
        int line = 4;
        int col = 3;
        for (int i = 0; i < cursor && line < 20; i++) {
            if (buffer[i] == '\n') {
                line++;
                col = 3;
                if (line >= 20) break;
                continue;
            }
            
            if (col >= 77) {
                line++;
                col = 3;
                if (line >= 20) break;
            }
            
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                char ch_str[2] = {buffer[i], 0};
                term.write_at(col, line, ch_str, 0x0F);
                col++;
            } else if (buffer[i] == '\t') {
                int spaces = 8 - ((col - 3) % 8);
                for (int s = 0; s < spaces && col < 77; s++) {
                    term.write_at(col, line, " ", 0x0F);
                    col++;
                }
            }
        }
        
        int cursor_line = 4;
        int cursor_col = 3;
        
        for (int i = 0; i < cursor; i++) {
            if (buffer[i] == '\n') {
                cursor_line++;
                cursor_col = 3;
            } else if (buffer[i] == '\t') {
                cursor_col += 8 - ((cursor_col - 3) % 8);
            } else {
                cursor_col++;
            }
            
            if (cursor_col >= 77) {
                cursor_line++;
                cursor_col = 3;
            }
        }
        
        term.write_at(cursor_col, cursor_line, "_", 0x0F);
        
        char info[32];
        int_to_str(cursor, info);
        term.write_at(3, 21, "Length: ", 0x0F);
        term.write_at(11, 21, info, 0x0F);
        
        int info_len = strlen(info);
        term.write_at(11 + info_len, 21, " bytes", 0x0F);
    }
};

class BrainfuckIDE {
private:
    VGATerminal& term;
    FileSystem& fs;
    BrainfuckInterpreter interpreter;
    BrainfuckCompiler compiler;
    
    char bf_code[1024];
    int cursor;
    bool active;
    
    void draw_code_editor() {
        term.fill_rect(30, 4, 48, 14, 0x17, ' ');
        
        int lines = 0;
        int col = 0;
        
        for (int i = 0; bf_code[i] && lines < 14; i++) {
            if (bf_code[i] == '\n' || col >= 46) {
                lines++;
                col = 0;
                if (lines >= 14) break;
                if (bf_code[i] == '\n') continue;
            }
            
            if (bf_code[i] >= 32 && bf_code[i] <= 126) {
                char ch_str[2] = {bf_code[i], 0};
                term.write_at(31 + col, 4 + lines, ch_str, 0x0F);
                col++;
            }
        }
        
        int cursor_line = 0;
        int cursor_col = 0;
        for (int i = 0; i < cursor; i++) {
            if (bf_code[i] == '\n') {
                cursor_line++;
                cursor_col = 0;
            } else {
                cursor_col++;
                if (cursor_col >= 46) {
                    cursor_line++;
                    cursor_col = 0;
                }
            }
        }
        
        if (cursor_line < 14) {
            term.write_at(31 + cursor_col, 4 + cursor_line, "_", 0x0E);
        }
        
        int bf_len = strlen(bf_code);
        int bf_ops = 0;
        const char* bf_chars = "><+-.,[]";
        for (int i = 0; i < bf_len; i++) {
            for (int j = 0; bf_chars[j]; j++) {
                if (bf_code[i] == bf_chars[j]) {
                    bf_ops++;
                    break;
                }
            }
        }
        
        char stats[32];
        char len_str[6], ops_str[6];
        int_to_str(bf_len, len_str);
        int_to_str(bf_ops, ops_str);
        strcpy(stats, "Len: ");
        strcat(stats, len_str);
        strcat(stats, " Ops: ");
        strcat(stats, ops_str);
        term.write_at(31, 18, stats, 0x0E);
    }
    
    void show_examples_menu() {
        term.fill_rect(10, 10, 60, 10, 0x17, ' ');
        term.draw_box(10, 10, 60, 10, 0x5F);
        term.write_at(12, 11, "Brainfuck Examples:", 0x5F);
        term.write_at(12, 12, "1. Hello World", 0x0F);
        term.write_at(12, 13, "2. Echo/Cat program", 0x0F);
        term.write_at(12, 14, "3. Simple loop", 0x0F);
        term.write_at(12, 15, "4. Fibonacci sequence", 0x0F);
        term.write_at(12, 16, "5. Clear screen", 0x0F);
        
        while (true) {
            if (Keyboard::is_key_pressed()) {
                char c = Keyboard::get_char();
                if (c == '1') {
                    strcpy(bf_code, "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.");
                    cursor = strlen(bf_code);
                    break;
                } else if (c == '2') {
                    strcpy(bf_code, ",.[,.]");
                    cursor = strlen(bf_code);
                    break;
                } else if (c == '3') {
                    strcpy(bf_code, "+++[>+++<-]>.");
                    cursor = strlen(bf_code);
                    break;
                } else if (c == '4') {
                    strcpy(bf_code, ">++++++++++>+>+[\n    [+++++[>++++++++<-]>.<++++++[>--------<-]+<<<]>.>>[\n        [-]<[>+<-]>>[<<+>>-]<<<<<.>\n    ]\n]");
                    cursor = strlen(bf_code);
                    break;
                } else if (c == '5') {
                    strcpy(bf_code, "++++++++++[>++++++++++<-]>+.");
                    cursor = strlen(bf_code);
                    break;
                } else if (c == (char)0xFA) { // F10 key
                    break;
                }
            }
        }
        
        draw_ui();
        draw_code_editor();
    }
    
    void show_compile_result() {
        char compiled_c[2048];
        if (compiler.compile_to_c(bf_code, compiled_c, 2048)) {
            term.fill_rect(10, 5, 60, 15, 0x17, ' ');
            term.draw_box(10, 5, 60, 15, 0x3F);
            term.write_at(12, 6, "Compiled C Code (Brainfuck -> C):", 0x3F);
            
            int line = 7;
            char* line_start = compiled_c;
            for (int i = 0; compiled_c[i] && line < 18; i++) {
                if (compiled_c[i] == '\n') {
                    compiled_c[i] = 0;
                    if (strlen(line_start) > 0) {
                        term.write_at(12, line++, line_start, 0x0F);
                    }
                    line_start = compiled_c + i + 1;
                }
            }
            
            term.write_at(12, 19, "Press any key to continue...", 0x0E);
            while (!Keyboard::is_key_pressed()) {}
            while (Keyboard::is_key_pressed()) { Keyboard::get_char(); }
        } else {
            term.write_at(31, 19, "Compilation failed!", 0x0C);
            for (volatile int i = 0; i < 2000000; i++);
        }
        
        draw_ui();
        draw_code_editor();
    }
    
public:
    BrainfuckIDE(VGATerminal& t, FileSystem& f) 
        : term(t), fs(f), interpreter(t), compiler(t), 
          cursor(0), active(false) {
        bf_code[0] = 0;
    }
    
    void open() {
        active = true;
        cursor = 0;
        bf_code[0] = 0;
        term.set_color(0x0F, 0x01);
        term.clear();
        draw_ui();
    }
    
    void close() { active = false; }
    bool is_active() { return active; }
    
    void draw_ui() {
        term.draw_box(0, 0, 80, 23, 0x4F);
        term.write_at(2, 1, "Brainfuck IDE - F5:Run F6:Compile F7:Save F8:Load F9:Examples F10:Exit", 0x4F);
        
        term.write_at(2, 3, "Examples:", 0x0E);
        term.write_at(2, 4, "1. Hello World", 0x0F);
        term.write_at(2, 5, "2. Echo input", 0x0F);
        term.write_at(2, 6, "3. Simple loop", 0x0F);
        term.write_at(2, 7, "4. Fibonacci", 0x0F);
        term.write_at(2, 8, "5. Clear screen", 0x0F);
        
        term.fill_line(22, 0x70, ' ');
        term.write_at(2, 22, "F10:Exit F5:Run F6:Compile F7:Save F8:Load F9:Examples", 0x70);
        
        draw_code_editor();
    }
    
    void handle_input(char c) {
        if (!active) return;
        
        if (c == (char)0xFA) { // F10 key instead of 0x1B (ESC)
            close();
            return;
        }
        
        if (c == (char)0xF5) {
            interpreter.run(bf_code);
            draw_ui();
            draw_code_editor();
            return;
        }
        
        if (c == (char)0xF6) {
            show_compile_result();
            return;
        }
        
        if (c == (char)0xF7) {
            term.fill_rect(20, 10, 40, 5, 0x17, ' ');
            term.draw_box(20, 10, 40, 5, 0x2F);
            term.write_at(22, 11, "Save as:", 0x2F);
            term.write_at(22, 12, "> PROGRAM.BF", 0x0F);
            
            if (fs.save_file("PROGRAM.BF", bf_code, strlen(bf_code))) {
                term.write_at(22, 13, "Saved successfully!", 0x0A);
            } else {
                term.write_at(22, 13, "Save failed!", 0x0C);
            }
            
            for (volatile int i = 0; i < 1000000; i++);
            draw_ui();
            draw_code_editor();
            return;
        }
        
        if (c == (char)0xF8) {
            term.fill_rect(20, 10, 40, 5, 0x17, ' ');
            term.draw_box(20, 10, 40, 5, 0x2F);
            term.write_at(22, 11, "Load file:", 0x2F);
            term.write_at(22, 12, "> PROGRAM.BF", 0x0F);
            
            char loaded[1024];
            uint32_t size;
            if (fs.load_file("PROGRAM.BF", loaded, size)) {
                strcpy(bf_code, loaded);
                cursor = size;
                if (cursor >= 1023) cursor = 1023;
                bf_code[cursor] = 0;
                term.write_at(22, 13, "Loaded successfully!", 0x0A);
            } else {
                term.write_at(22, 13, "File not found", 0x0C);
            }
            
            for (volatile int i = 0; i < 1000000; i++);
            draw_ui();
            draw_code_editor();
            return;
        }
        
        if (c == (char)0xF9) {
            show_examples_menu();
            return;
        }
        
        if (c == '\b') {
            if (cursor > 0) {
                cursor--;
                for (int i = cursor; i < 1023; i++) {
                    bf_code[i] = bf_code[i + 1];
                }
                bf_code[1023] = 0;
            }
        } else if (c == '\n') {
            if (cursor < 1022) {
                bf_code[cursor++] = '\n';
                bf_code[cursor] = 0;
            }
        } else if (c >= 32 && c <= 126 && cursor < 1022) {
            bf_code[cursor++] = c;
            bf_code[cursor] = 0;
        }
        
        draw_code_editor();
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
    
    int atoi(const char* str) {
        int result = 0;
        for (int i = 0; str[i] >= '0' && str[i] <= '9'; i++) {
            result = result * 10 + (str[i] - '0');
        }
        return result;
    }
    
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
    
    void close() { active = false; }
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
        term.write_at(15, 6, "Calculator - F2 to exit", 0x2F);
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
};

class FileManager {
private:
    VGATerminal& term;
    FileSystem& fs;
    int selected;
    bool active;
    
public:
    FileManager(VGATerminal& t, FileSystem& f) : term(t), fs(f), selected(0), active(false) {}
    
    void open() {
        active = true;
        selected = 0;
        term.set_color(0x0F, 0x01);
        term.clear();
        draw_ui();
    }
    
    void close() { active = false; }
    bool is_active() { return active; }
    
    void handle_input(char c) {
        if (!active) return;
        
        if (c == (char)0xF3) {
            close();
            return;
        }
        
        if (c == '\n' || c == '\r') {
            open_selected();
            return;
        }
        
        if (c == 'j') {
            if (selected < fs.get_file_count() - 1) selected++;
        } else if (c == 'k') {
            if (selected > 0) selected--;
        }
        
        draw_ui();
    }
    
    void open_selected() {
        FileEntry* file = fs.get_file(selected);
        if (!file) return;
        
        term.clear();
        term.draw_box(0, 0, 80, 23, 0x6F);
        term.write_at(2, 1, "File: ", 0x6F);
        term.write_at(8, 1, file->name, 0x6F);
        term.write_at(60, 1, "F10 to exit", 0x6F);
        
        char content[MAX_FILE_SIZE];
        uint32_t size;
        if (fs.load_file(file->name, content, size)) {
            int line = 3;
            int col = 2;
            for (uint32_t i = 0; i < size && line < 22; i++) {
                if (content[i] == '\n') {
                    line++;
                    col = 2;
                    continue;
                }
                
                if (col >= 78) {
                    line++;
                    col = 2;
                    if (line >= 22) break;
                }
                
                if (content[i] >= 32 && content[i] <= 126) {
                    char ch_str[2] = {content[i], 0};
                    term.write_at(col, line, ch_str, 0x0F);
                    col++;
                }
            }
        }
        
        while (true) {
            if (Keyboard::is_key_pressed()) {
                char c = Keyboard::get_char();
                if (c == (char)0xFA) { // F10 key
                    break;
                }
            }
        }
        
        draw_ui();
    }
    
    void draw_ui() {
        term.set_color(0x0F, 0x01);
        term.draw_box(1, 1, 78, 21, 0x6F);
        term.write_at(5, 2, "File Manager - F3 to exit", 0x6F);
        term.write_at(55, 2, "j/k:Navigate Enter:Open", 0x6F);
        
        term.write_at(3, 4, "Name", 0x6F);
        term.write_at(40, 4, "Size", 0x6F);
        term.fill_rect(3, 5, 74, 1, 0x60, 0xC4);
        
        term.fill_rect(3, 6, 74, 14, 0x17, ' ');
        
        int file_count = fs.get_file_count();
        for (int i = 0; i < file_count && i < 14; i++) {
            FileEntry* file = fs.get_file(i);
            if (!file) continue;
            
            int y = 6 + i;
            uint8_t color = (i == selected) ? 0x70 : 0x0F;
            
            char size_str[16];
            int_to_str(file->size, size_str);
            
            term.write_at(3, y, file->name, color);
            term.write_at(40, y, size_str, color);
            
            int size_len = strlen(size_str);
            term.write_at(40 + size_len + 1, y, "bytes", color);
            
            if (i == selected) {
                term.write_at(2, y, ">", color);
            }
        }
        
        char info[32];
        int_to_str(selected + 1, info);
        term.write_at(3, 20, "Selected: ", 0x0F);
        term.write_at(13, 20, info, 0x0F);
        term.write_at(14 + strlen(info), 20, " of ", 0x0F);
        
        int_to_str(file_count, info);
        term.write_at(18 + strlen(info), 20, info, 0x0F);
        
        term.write_at(3, 21, "Press Enter to open, F10 to exit", 0x0F);
    }
};

class Desktop {
private:
    VGATerminal term;
    FileSystem fs;
    ClockDisplay clock;
    TextEditor editor;
    Calculator calculator;
    FileManager fileman;
    BrainfuckIDE brainfuck;
    
    enum App { NONE, EDITOR, CALCULATOR, FILEMAN, BRAINFUCK } current_app;
    char input_buffer[128];
    int cursor;
    
public:
    Desktop() : fs(), clock(term), editor(term, fs), calculator(term), 
                fileman(term, fs), brainfuck(term, fs), 
                current_app(NONE), cursor(0) {
        for (int i = 0; i < 128; i++) input_buffer[i] = 0;
    }
    
    void run() {
        term.set_color(0x0F, 0x01);
        term.clear();
        draw_desktop();
        
        while (true) {
            clock.update();
            draw_taskbar();
            
            if (current_app == NONE) {
                handle_shell_input();
            } else {
                handle_app_input();
            }
            
            for (volatile int i = 0; i < 20000; i++);
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
                    else if (strcmp(input_buffer, "fi") == 0) strcpy(input_buffer, "files");
                    else if (strcmp(input_buffer, "bf") == 0) strcpy(input_buffer, "brainfuck");
                    cursor = strlen(input_buffer);
                } else if (c >= 32 && c <= 126 && cursor < 127) {
                    input_buffer[cursor] = c;
                    cursor++;
                    input_buffer[cursor] = 0;
                } else if (c == (char)0xF1 || c == (char)0xF2 || c == (char)0xF3 || c == (char)0xF5) {
                    switch ((uint8_t)c) {
                        case 0xF1: strcpy(input_buffer, "edit"); execute_command(); break;
                        case 0xF2: strcpy(input_buffer, "calc"); execute_command(); break;
                        case 0xF3: strcpy(input_buffer, "files"); execute_command(); break;
                        case 0xF5: strcpy(input_buffer, "brainfuck"); execute_command(); break;
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
                            clock.draw();
                        }
                        break;
                        
                    case CALCULATOR:
                        calculator.handle_input(c);
                        if (!calculator.is_active()) {
                            current_app = NONE;
                            term.set_color(0x0F, 0x01);
                            term.clear();
                            draw_desktop();
                            clock.draw();
                        }
                        break;
                        
                    case FILEMAN:
                        fileman.handle_input(c);
                        if (!fileman.is_active()) {
                            current_app = NONE;
                            term.set_color(0x0F, 0x01);
                            term.clear();
                            draw_desktop();
                            clock.draw();
                        }
                        break;
                        
                    case BRAINFUCK:
                        brainfuck.handle_input(c);
                        if (!brainfuck.is_active()) {
                            current_app = NONE;
                            term.set_color(0x0F, 0x01);
                            term.clear();
                            draw_desktop();
                            clock.draw();
                        }
                        break;
                        
                    case NONE:
                        break;
                }
            }
        }
    }
    
    void execute_command() {
        term.clear_area(2, 18, 78, 1);
        
        if (strncmp(input_buffer, "edit ", 5) == 0) {
            const char* filename = input_buffer + 5;
            current_app = EDITOR;
            editor.open(filename);
        } else if (strcmp(input_buffer, "edit") == 0) {
            current_app = EDITOR;
            editor.open();
        } else if (strcmp(input_buffer, "calc") == 0) {
            current_app = CALCULATOR;
            calculator.open();
        } else if (strcmp(input_buffer, "files") == 0) {
            current_app = FILEMAN;
            fileman.open();
        } else if (strcmp(input_buffer, "brainfuck") == 0 || strcmp(input_buffer, "bf") == 0) {
            current_app = BRAINFUCK;
            brainfuck.open();
        } else if (strncmp(input_buffer, "bfrun ", 6) == 0) {
            const char* code = input_buffer + 6;
            BrainfuckInterpreter bf_interp(term);
            bf_interp.run(code);
            draw_desktop();
            clock.draw();
        } else if (strcmp(input_buffer, "clear") == 0) {
            term.set_color(0x0F, 0x01);
            term.clear();
            draw_desktop();
            clock.draw();
        } else if (strcmp(input_buffer, "about") == 0) {
            draw_about();
        } else if (strcmp(input_buffer, "time") == 0) {
            show_time_info();
        } else if (input_buffer[0] != 0) {
            term.write_at(2, 18, "Unknown command. Type 'help' in README", 0x0F);
        }
    }
    
    void draw_desktop() {
        term.set_color(0x0F, 0x01);
        term.clear();
        term.draw_box(0, 0, 80, 22, 0x3F);
        term.write_at(2, 1, "EH-DSB*beta_1 with Brainfuck IDE (F10 to exit Brainfuck)", 0x3F);
        term.write_at(50, 1, "F1:Edit F2:Calc F3:Files F5:Brainfuck", 0x3F);
        
        clock.draw();
        
        term.write_at(2, 3, "Welcome to Event Horizon DSB!", 0x0F);
        
        char file_count_str[32];
        int_to_str(fs.get_file_count(), file_count_str);
        term.write_at(2, 5, "Files: ", 0x0F);
        term.write_at(9, 5, file_count_str, 0x0F);
        
        term.write_at(2, 7, "Commands:", 0x0F);
        term.write_at(2, 9, "  edit [file] - Text Editor", 0x0F);
        term.write_at(2, 10, "  calc        - Calculator", 0x0F);
        term.write_at(2, 11, "  files       - File Manager", 0x0F);
        term.write_at(2, 12, "  brainfuck   - Brainfuck IDE", 0x0F);
        term.write_at(2, 13, "  bfrun <code>- Run Brainfuck", 0x0F);
        term.write_at(2, 14, "  time        - Show time", 0x0F);
        term.write_at(2, 15, "  clear       - Clear screen", 0x0F);
        term.write_at(2, 16, "  about       - System info", 0x0F);
        
        draw_shell();
    }
    
    void show_time_info() {
        uint8_t hour, minute;
        RTC::get_time(hour, minute);
        
        term.fill_rect(20, 8, 40, 8, 0x17, ' ');
        term.draw_box(20, 8, 40, 8, 0x5E);
        term.write_at(22, 9, "System Time", 0x5E);
        
        char time_info[32];
        time_info[0] = '0' + (hour / 10);
        time_info[1] = '0' + (hour % 10);
        time_info[2] = ':';
        time_info[3] = '0' + (minute / 10);
        time_info[4] = '0' + (minute % 10);
        time_info[5] = 0;
        
        term.write_at(22, 11, "Current: ", 0x0F);
        term.write_at(31, 11, time_info, 0x0F);
        term.write_at(22, 13, "Press any key", 0x0F);
        
        while (!Keyboard::is_key_pressed()) {}
        while (Keyboard::is_key_pressed()) { Keyboard::get_char(); }
        
        draw_desktop();
        clock.draw();
    }
    
    void draw_about() {
        term.fill_rect(10, 5, 60, 15, 0x17, ' ');
        term.draw_box(10, 5, 60, 15, 0x5F);
        term.write_at(12, 6, "EH-DSB*beta_1", 0x5F);
        term.write_at(12, 8, "Version: beta_1", 0x0F);
        term.write_at(12, 14, "Features:", 0x0F);
        term.write_at(12, 15, "- Text Editor", 0x0F);
        term.write_at(12, 16, "- Calculator", 0x0F);
        term.write_at(12, 17, "- File Manager", 0x0F);
        term.write_at(12, 18, "- Brainfuck IDE (F10 to exit)", 0x0F);
        
        while (!Keyboard::is_key_pressed()) {}
        while (Keyboard::is_key_pressed()) { Keyboard::get_char(); }
        
        draw_desktop();
        clock.draw();
    }
    
    void draw_shell() {
        term.write_at(2, 20, "ehdsb> ", 0x0F);
        term.write_at(8, 20, input_buffer, 0x0F);
        term.set_cursor(8 + strlen(input_buffer), 20);
        term.putchar('_');
    }
    
    void draw_taskbar() {
        term.fill_line(22, 0x70, ' ');
        
        if (current_app == NONE) {
            term.write_at(2, 22, "EH-DSB*beta_1 | F1:Editor F2:Calculator F3:Files F5:Brainfuck IDE", 0x70);
        } else if (current_app == EDITOR) {
            term.write_at(2, 22, "Text Editor | F1:Exit F4:Save", 0x70);
        } else if (current_app == CALCULATOR) {
            term.write_at(2, 22, "Calculator | F2:Exit", 0x70);
        } else if (current_app == FILEMAN) {
            term.write_at(2, 22, "File Manager | F3:Exit j/k:Navigate Enter:Open F10:Exit viewer", 0x70);
        } else if (current_app == BRAINFUCK) {
            term.write_at(2, 22, "Brainfuck IDE | F10:Exit F5:Run F6:Compile F7:Save F8:Load F9:Examples", 0x70);
        }
    }
};
extern "C" void kernel_main() {
    uint16_t* video = (uint16_t*)VGA_BUFFER;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i] = (0x1F << 8) | ' ';
    }
    const char* msg = "EH-DSB*beta_1 with Brainfuck - Loading...";
    for (int i = 0; msg[i]; i++) {
        video[i] = (0x1F << 8) | msg[i];
    }

    Desktop desktop;
    desktop.run();
}
extern "C" void _start() __attribute__((section(".text.start")));
extern "C" void _start() {
    extern uint32_t __bss_start, __bss_end;
    uint32_t* p = &__bss_start;
    while (p < &__bss_end) {
        *p++ = 0;
    }
    kernel_main();

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
