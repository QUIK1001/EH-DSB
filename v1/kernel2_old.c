void clear_screen(unsigned char attr);
void print_centered(int line, const char *msg, unsigned char attr);
void print_message(int line, const char *msg);
void print_status(int line, const char *msg, unsigned char attr);
void draw_horizontal_line(int line, unsigned char ch, unsigned char attr);
void draw_box(int start_line, int start_col, int width, int height, unsigned char border_ch, unsigned char attr);
void print_logo(int start_line);

void kernel_main() {
    clear_screen(0x1F);
    
    print_logo(1);
    
    print_centered(9, "Event Horizon DSB", 0x1F);
    print_centered(11, "microkernel 2", 0x1E);
    
    draw_horizontal_line(13, 0xC4, 0x17);
    
    print_status(17, "[info] microkernel initialized", 0x2C);
    print_centered(23, "t.me/Event_Horizon-Shell (my python proj)", 0x5F);
    draw_box(19, 20, 60, 3, 0xB3, 0x20);
    print_centered(20, "by quik", 0x5E);
    print_centered(21, "testing", 0x2E);
    while (1) {
    }
}

void print_logo(int start_line) {
    unsigned char *video_memory = (unsigned char*)0xB8000;
    
    static const char *logo_lines[] = {
        "    ███████╗██╗ ⠀⠀⣀⣰⣶⣿⣿⣿⣿⣿⣿⣀⣀⣀⣀⣀⣀⣰⣰⣶⣿█╗██████╗███████⠀⠀⣀⣰⣶⣿⣿⣿⣿⣿⣿⣀⣀⣀⣀⣀⣀⣰⣿⣿⣿⣿⣿⣷⣦⣤⣀████╗⠉⠉⠉⠈⠉⠛⠛⠛⠛⣿⣿⡽⣏⠉⠉⠉⠉⠉⣽⣿⠛⠉⠉⠉⠉⠉⠉⠉⠁╗ ██╗███████╗",
	"    ███ █⠉⠉⠉⠈⠉⠛⠛⠛⠛⣿⣿⡽⣏⠉⠉⠉⣀⣀⣰⣰⣶⣿⣿⣿⣿⣿⣿⣷⣦⣤⣀████╗██╗⠉⠉⠁███████╗⠀⠉⠉⣽⣿⠛⠉⠉⠉⠉⠉⠀█╗██████╗███████⠀⠀⣀⣰⣶⣿⣿⣿⣿⣿⣿⣀⣀⣀⣀⣀⣀⣰⣀⣰⣶⣿⣿⣿⣿⣿⣿⣀⣀⣀⣀ ███╗",
	"    ██████⣿⣿⣿⣿⣿⣿⣷⣦⣤⣀██╗ ███████╗██⠉⠉⠉⠈⠉⠛╗██⠀⠀⣀⣰⣶⣿⣿⣿█╗██████╗███████⠀⠀⣀⣰⣶⣿⣿⣿⣿⣿⣿⣀⣀⣀⣀⣀⣀⣰⣿╗██████╗█⣿⣿⣀⣀⣀⣀⣀⣀⣰⣰⣶⠛⠛⠉⠉⠉⠉⠉⠉⠉⠁╗ ██╗███████╗",
	"    ███ ██╗██████╗███████⠀⠀⣀⣰⣶⣿⣿⣿⣿⣿⣿⣀⣀█╗██████╗███████⠀⠀⣀⣰⣶⣿⣿⣿█╗██████╗███████⠀⠀⣀⣰⣶⣿⣿⣿⣿⣿⣿⣀⣀⣀⣀⣀⣀⣰⣿⣿⣿⣀⣀⣀⣀⣀⣀⣰██╗ ███╗",

    };
    
    static const unsigned char colors[] = {0x1E, 0x1C, 0x1D, 0x1E, 0x1A, 0x1B, 0x17};
    int total_lines = 7;
    
    for (int i = 0; i < total_lines; i++) {
        int pos = (start_line + i) * 160 + 10;
        const char *line = logo_lines[i];
        int j = 0;
        
        while (line[j] != 0) {
            video_memory[pos] = line[j];
            video_memory[pos + 1] = colors[i];
            pos += 2;
            j++;
        }
    }
}

void clear_screen(unsigned char attr) {
    unsigned char *video_memory = (unsigned char*)0xB8000;
    int i = 0;
    while (i < 80 * 25 * 2) {
        video_memory[i] = ' ';
        video_memory[i + 1] = attr;
        i += 2;
    }
}

void print_centered(int line, const char *msg, unsigned char attr) {
    unsigned char *video_memory = (unsigned char*)0xB8000;
    int len = 0;
    while (msg[len]) len++;
    
    int pos = ((80 - len) / 2) * 2 + line * 160;
    
    int i = 0;
    while (msg[i] != 0) {
        video_memory[pos] = msg[i];
        video_memory[pos + 1] = attr;
        pos += 2;
        i++;
    }
}

void print_message(int line, const char *msg) {
    unsigned char *video_memory = (unsigned char*)0xB8000;
    int pos = line * 160;
    
    static const unsigned char colors[] = {0x1E, 0x1C, 0x1D, 0x1E, 0x1A, 0x1B};
    int color_index = 0;
    
    int i = 0;
    while (msg[i] != 0) {
        video_memory[pos] = msg[i];
        video_memory[pos + 1] = colors[color_index % 6];
        pos += 2;
        i++;
        color_index++;
    }
}

void print_status(int line, const char *msg, unsigned char attr) {
    unsigned char *video_memory = (unsigned char*)0xB8000;
    int pos = 10 * 2 + line * 160;
    
    int i = 0;
    while (msg[i] != 0) {
        video_memory[pos] = msg[i];
        video_memory[pos + 1] = attr;
        pos += 2;
        i++;
    }
}

void draw_horizontal_line(int line, unsigned char ch, unsigned char attr) {
    unsigned char *video_memory = (unsigned char*)0xB8000;
    int pos = line * 160;
    
    int i = 0;
    while (i < 80) {
        video_memory[pos] = ch;
        video_memory[pos + 1] = attr;
        pos += 2;
        i++;
    }
}

void draw_box(int start_line, int start_col, int width, int height, unsigned char border_ch, unsigned char attr) {
    unsigned char *video_memory = (unsigned char*)0xB8000;
    
    int y = 0;
    while (y < height) {
        int x = 0;
        while (x < width) {
            int pos = (start_line + y) * 160 + (start_col + x) * 2;
            
            unsigned char draw_ch = ' ';
            if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
                draw_ch = border_ch;
            }
            
            video_memory[pos] = draw_ch;
            video_memory[pos + 1] = attr;
            x++;
        }
        y++;
    }
}
