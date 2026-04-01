typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB8000
#define MAX_FILES 64
#define MAX_FILE_SIZE 8192
#define FS_METADATA_SIZE 4096
#define FS_START 0x20000
#define FS_DATA_START (FS_START + FS_METADATA_SIZE)
#define FS_TOTAL_SIZE 0x20000
#define MAX_INPUT_LEN 512
#define MAX_COMMAND_HISTORY 50
#define FS_MAGIC 0xE4F5D3B2
static uint16_t* vga_buffer = (uint16_t*)VGA_BUFFER;
static void outb(uint16_t port, uint8_t value) {
asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}
static uint8_t inb(uint16_t port) {
uint8_t result;
asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
return result;
}
static void io_wait() {
outb(0x80, 0);
}
static int strlen(const char* str) {
int len = 0;
while (str[len]) len++;
return len;
}
static const char* strstr(const char* haystack, const char* needle) {
if (!needle || !needle[0]) return haystack;
for (; haystack && *haystack; ++haystack) {
const char* h = haystack;
const char* n = needle;
while (*h && *n && *h == *n) { ++h; ++n; }
if (!*n) return haystack;
}
return 0;
}
static void strcpy(char* dest, const char* src) {
int i = 0;
while (src[i]) {
dest[i] = src[i];
i++;
}
dest[i] = 0;
}
static void strncpy(char* dest, const char* src, int n) {
int i;
for (i = 0; i < n - 1 && src[i]; i++) dest[i] = src[i];
dest[i] = 0;
}
static void strcat(char* dest, const char* src) {
int i = strlen(dest);
int j = 0;
while (src[j]) {
dest[i] = src[j];
i++; j++;
}
dest[i] = 0;
}
static int strcmp(const char* a, const char* b) {
while (*a && *b && *a == *b) { a++; b++; }
return *a - *b;
}
static int strncmp(const char* a, const char* b, int n) {
for (int i = 0; i < n; i++) {
if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
if (a[i] == 0) return 0;
}
return 0;
}
static void memset(void* ptr, uint8_t value, uint32_t n) {
uint8_t* p = (uint8_t*)ptr;
for (uint32_t i = 0; i < n; i++) p[i] = value;
}
static void memcpy(void* dest, const void* src, uint32_t n) {
uint8_t* d = (uint8_t*)dest;
const uint8_t* s = (const uint8_t*)src;
for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}
static void int_to_str(int num, char* str) {
if (num == 0) {
str[0] = '0';
str[1] = 0;
return;
}
int i = 0;
char temp[12];
int n = num;
if (n < 0) n = -n;
while (n > 0) {
temp[i] = '0' + (n % 10);
n /= 10;
i++;
}
int pos = 0;
if (num < 0) str[pos++] = '-';
for (int j = i - 1; j >= 0; j--) {
str[pos++] = temp[j];
}
str[pos] = 0;
}
struct FileEntry {
char name[13];
uint32_t size;
uint32_t data_offset;
bool used;
bool read_only;
};
struct FileSystemHeader {
uint32_t magic;
uint32_t version;
uint32_t next_free_offset;
uint32_t file_count;
};
class FileSystem {
private:
FileEntry files[MAX_FILES];
uint8_t* fs_buffer;
uint32_t next_free_offset;
void load_metadata() {
FileSystemHeader* header = (FileSystemHeader*)fs_buffer;
if (header->magic != FS_MAGIC) {
memset(files, 0, sizeof(files));
next_free_offset = 0;
return;
}
memcpy(files, fs_buffer + sizeof(FileSystemHeader), sizeof(files));
next_free_offset = header->next_free_offset;
}

void save_metadata() {
FileSystemHeader header;
header.magic = FS_MAGIC;
header.version = 2;
header.next_free_offset = next_free_offset;
header.file_count = get_file_count();
memcpy(fs_buffer, &header, sizeof(FileSystemHeader));
memcpy(fs_buffer + sizeof(FileSystemHeader), files, sizeof(files));
}

int find_free_file() {
for (int i = 0; i < MAX_FILES; i++) {
if (!files[i].used) return i;
}
return -1;
}

int find_file(const char* name) {
for (int i = 0; i < MAX_FILES; i++) {
if (files[i].used && strcmp(files[i].name, name) == 0) return i;
}
return -1;
}
public:
FileSystem() : fs_buffer((uint8_t*)FS_START), next_free_offset(0) {
load_metadata();
create_default_files();
}
void create_default_files() {
if (find_file("README.TXT") == -1) {
create_file("README.TXT",
"EH-DSB v0.01 - Commands\n"
"==========================\n"
"help/?       - Show help\n"
"ls/dir       - List files\n"
"cat <file>   - View file\n"
"edit <file>  - Edit file\n"
"rm <file>    - Delete file\n"
"mv <old> <new> - Rename\n"
"time         - Show time\n"
"clear/cls    - Clear screen\n"
"about        - info\n"
"history      - Command history\n"
"reboot       - Reboot system\n"
"echo <text>  - Print text\n"
"mem          - Memory info\n"
"info         - Information\n"
"tz <offset>  - Set timezone (-12 to +12)\n", true);
}
if (find_file("HELLO.BF") == -1) {
create_file("HELLO.BF",
"++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.");
}
if (find_file("ECHO.BF") == -1) {
create_file("ECHO.BF", ",[.,]");
}
}

bool create_file(const char* name, const char* content, bool read_only = false) {
int idx = find_free_file();
if (idx == -1) return false;

strncpy(files[idx].name, name, 12);
files[idx].size = content ? strlen(content) : 0;
files[idx].used = true;
files[idx].read_only = read_only;
files[idx].data_offset = next_free_offset;

if (next_free_offset + files[idx].size > FS_TOTAL_SIZE - FS_METADATA_SIZE) {
files[idx].used = false;
return false;
}

if (content) {
memcpy(fs_buffer + FS_METADATA_SIZE + files[idx].data_offset, content, files[idx].size);
}

next_free_offset += files[idx].size;
save_metadata();
return true;
}

bool save_file(const char* name, const char* content, uint32_t size) {
int idx = find_file(name);
if (idx == -1) {
idx = find_free_file();
if (idx == -1) return false;
strncpy(files[idx].name, name, 12);
files[idx].used = true;
files[idx].read_only = false;
files[idx].data_offset = next_free_offset;
next_free_offset += size;
} else {
if (files[idx].read_only) return false;
files[idx].data_offset = next_free_offset;
next_free_offset += size;
}

files[idx].size = size;

if (files[idx].data_offset + size > FS_TOTAL_SIZE - FS_METADATA_SIZE) return false;

memcpy(fs_buffer + FS_METADATA_SIZE + files[idx].data_offset, content, size);
save_metadata();
return true;
}

bool load_file(const char* name, char* buffer, uint32_t &size) {
int idx = find_file(name);
if (idx == -1) return false;

size = files[idx].size;
if (size > MAX_FILE_SIZE) size = MAX_FILE_SIZE;

memcpy(buffer, fs_buffer + FS_METADATA_SIZE + files[idx].data_offset, size);
if (size < MAX_FILE_SIZE) buffer[size] = 0;

return true;
}

bool delete_file(const char* name) {
int idx = find_file(name);
if (idx == -1) return false;
if (files[idx].read_only) return false;

files[idx].used = false;
save_metadata();
return true;
}

bool rename_file(const char* old_name, const char* new_name) {
int idx = find_file(old_name);
if (idx == -1) return false;
if (files[idx].read_only) return false;
if (find_file(new_name) != -1) return false;

memset(files[idx].name, 0, 13);
strncpy(files[idx].name, new_name, 12);
save_metadata();
return true;
}

bool toggle_readonly(const char* name) {
int idx = find_file(name);
if (idx == -1) return false;
if (strcmp(name, "README.TXT") == 0) return false;

files[idx].read_only = !files[idx].read_only;
save_metadata();
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
if (index < 0 || index >= MAX_FILES) return 0;
int count = 0;
for (int i = 0; i < MAX_FILES; i++) {
if (files[i].used) {
if (count == index) return &files[i];
count++;
}
}
return 0;
}

uint32_t get_fs_size() {
uint32_t total = 0;
for (int i = 0; i < MAX_FILES; i++) {
if (files[i].used) total += files[i].size;
}
return total;
}

uint32_t get_free_space() {
return FS_TOTAL_SIZE - FS_METADATA_SIZE - get_fs_size();
}
};
class RTC {
private:
static uint8_t bcd_to_bin(uint8_t bcd) {
return (bcd >> 4) * 10 + (bcd & 0x0F);
}
static uint8_t read_register(uint8_t reg) {
outb(0x70, reg);
return inb(0x71);
}
static uint32_t timer_ticks;
static int timezone_offset;
public:
static void init() {
timezone_offset = 3;
}
static void set_timezone(int offset) {
if (offset >= -12 && offset <= 12) {
timezone_offset = offset;
}
}
static int get_timezone() {
return timezone_offset;
}
static void get_time(uint8_t& hour, uint8_t& minute, uint8_t& second) {
uint8_t century_register = 0x32;
uint8_t century = 0;
uint8_t registerB;

do {
hour = read_register(0x04);
minute = read_register(0x02);
second = read_register(0x00);
century = read_register(century_register);
registerB = read_register(0x0B);
} while (hour != read_register(0x04) || minute != read_register(0x02) || second != read_register(0x00));

if (!(registerB & 0x04)) {
hour = bcd_to_bin(hour);
minute = bcd_to_bin(minute);
second = bcd_to_bin(second);
}

int temp_hour = (int)hour + timezone_offset;
while (temp_hour >= 24) temp_hour -= 24;
while (temp_hour < 0) temp_hour += 24;
hour = (uint8_t)temp_hour;

if (!(registerB & 0x02)) {
if (hour >= 12) {
if (hour != 12) hour -= 12;
} else {
if (hour == 12) hour = 0;
}
}

if (century) {
century = bcd_to_bin(century);
}
}
static void tick() {
timer_ticks++;
}
static bool should_update() {
static uint32_t last = 0;
if (timer_ticks - last > 18) {
last = timer_ticks;
return true;
}
return false;
}
};
uint32_t RTC::timer_ticks = 0;
int RTC::timezone_offset = 3;
class Mouse {
private:
static int mouse_x;
static int mouse_y;
static bool mouse_left;
static bool mouse_right;
static bool mouse_middle;
static bool mouse_enabled;
static uint8_t mouse_packet[3];
static int mouse_byte;
static int mouse_remainder_x;
static int mouse_remainder_y;
static void wait_write() {
for (int i = 0; i < 100000; i++) {
if ((inb(0x64) & 2) == 0) return;
io_wait();
}
}

static void wait_read() {
for (int i = 0; i < 100000; i++) {
if (inb(0x64) & 1) return;
io_wait();
}
}

static void write_mouse(uint8_t value) {
wait_write();
outb(0x64, 0xD4);
wait_write();
outb(0x60, value);
for (int i = 0; i < 1000; i++) io_wait();
}

static uint8_t read_mouse() {
wait_read();
uint8_t val = inb(0x60);
for (int i = 0; i < 100; i++) io_wait();
return val;
}

static bool wait_ack() {
for (int i = 0; i < 100000; i++) {
if (inb(0x64) & 1) {
uint8_t b = inb(0x60);
if (b == 0xFA) return true;
if (b == 0xFE) return false;
}
io_wait();
}
return false;
}

static void clear_buffer() {
for (int i = 0; i < 20; i++) {
if (inb(0x64) & 1) {
inb(0x60);
}
io_wait();
}
}
public:
static void init() {
mouse_x = 40;
mouse_y = 12;
mouse_left = false;
mouse_right = false;
mouse_middle = false;
mouse_enabled = false;
mouse_byte = 0;
mouse_remainder_x = 0;
mouse_remainder_y = 0;
mouse_packet[0] = 0;
mouse_packet[1] = 0;
mouse_packet[2] = 0;
for (int i = 0; i < 10000; i++) io_wait();
clear_buffer();
wait_write();
outb(0x64, 0xA8);
for (int i = 0; i < 1000; i++) io_wait();

wait_write();
outb(0x64, 0x20);
for (int i = 0; i < 1000; i++) io_wait();
wait_read();
uint8_t status = inb(0x60);
for (int i = 0; i < 100; i++) io_wait();

status |= 2;
status &= ~1;

wait_write();
outb(0x64, 0x60);
for (int i = 0; i < 1000; i++) io_wait();
wait_write();
outb(0x60, status);
for (int i = 0; i < 1000; i++) io_wait();

clear_buffer();

write_mouse(0xFF);
for (int i = 0; i < 50000; i++) io_wait();
clear_buffer();
if (inb(0x64) & 1) inb(0x60);
if (inb(0x64) & 1) inb(0x60);

write_mouse(0xF6);
if (!wait_ack()) {
clear_buffer();
return;
}
if (inb(0x64) & 1) inb(0x60);

write_mouse(0xF4);
if (!wait_ack()) {
clear_buffer();
return;
}

clear_buffer();
for (int i = 0; i < 10000; i++) io_wait();

mouse_enabled = true;
}

static void handle_packet() {
if (!mouse_enabled) return;

uint8_t buttons = mouse_packet[0];

if ((buttons & 0x08) == 0) {
mouse_byte = 0;
return;
}

int8_t dx = (int8_t)mouse_packet[1];
int8_t dy = (int8_t)mouse_packet[2];

if (buttons & 0x10) dx = (int8_t)(dx | 0xC0);
if (buttons & 0x20) dy = (int8_t)(dy | 0xC0);

mouse_remainder_x += dx;
mouse_remainder_y += dy;

int move_x = mouse_remainder_x / 4;
int move_y = mouse_remainder_y / 4;

mouse_remainder_x %= 4;
mouse_remainder_y %= 4;

mouse_x += move_x;
mouse_y -= move_y;

if (mouse_x < 0) mouse_x = 0;
if (mouse_x > 79) mouse_x = 79;
if (mouse_y < 0) mouse_y = 0;
if (mouse_y > 24) mouse_y = 24;

mouse_left = (buttons & 1) != 0;
mouse_right = (buttons & 2) != 0;
mouse_middle = (buttons & 4) != 0;
}

static void update() {
if (!mouse_enabled) return;

while (inb(0x64) & 0x20) {
uint8_t byte = read_mouse();

if (mouse_byte == 0) {
if ((byte & 0x08) == 0 || (byte & 0xC0) != 0) {
continue;
}
}

mouse_packet[mouse_byte] = byte;
mouse_byte++;

if (mouse_byte >= 3) {
mouse_byte = 0;
handle_packet();
}
}
}

static int get_x() { return mouse_x; }
static int get_y() { return mouse_y; }
static bool is_left_clicked() { return mouse_left; }
static bool is_right_clicked() { return mouse_right; }
static bool is_middle_clicked() { return mouse_middle; }
static bool is_enabled() { return mouse_enabled; }

static void reset_clicks() {
mouse_left = false;
mouse_right = false;
mouse_middle = false;
}
};
int Mouse::mouse_x = 40;
int Mouse::mouse_y = 12;
bool Mouse::mouse_left = false;
bool Mouse::mouse_right = false;
bool Mouse::mouse_middle = false;
bool Mouse::mouse_enabled = false;
uint8_t Mouse::mouse_packet[3] = {0, 0, 0};
int Mouse::mouse_byte = 0;
int Mouse::mouse_remainder_x = 0;
int Mouse::mouse_remainder_y = 0;
class Keyboard {
private:
static bool left_shift, right_shift, caps_lock;
public:
static bool is_key_pressed() {
uint8_t status = inb(0x64);
return (status & 0x01) && !(status & 0x20);
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
static const char shifted[58] = {
0, 0,
'!','@','#','$','%','^','&','*','(',')','_','+',
'\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
0, 'A','S','D','F','G','H','J','K','L',':','"','~',
0, '|','Z','X','C','V','B','N','M','<','>','?',
0, '*', 0, ' '
};

if (sc == 0x3B) return (char)0xF1;
if (sc == 0x3C) return (char)0xF2;
if (sc == 0x3D) return (char)0xF3;
if (sc == 0x3E) return (char)0xF4;
if (sc == 0x3F) return (char)0xF5;
if (sc == 0x40) return (char)0xF6;
if (sc == 0x41) return (char)0xF7;
if (sc == 0x42) return (char)0xF8;
if (sc == 0x43) return (char)0xF9;
if (sc == 0x44) return (char)0xFA;
if (sc == 0x48) return (char)0x10;
if (sc == 0x50) return (char)0x11;

if (sc < 58) {
if (is_shifted && shifted[sc]) return shifted[sc];
return normal[sc];
}
return 0;
}

static void flush() {
while (is_key_pressed()) get_scancode();
}
};
bool Keyboard::left_shift = false;
bool Keyboard::right_shift = false;
bool Keyboard::caps_lock = false;
class VGATerminal {
private:
uint8_t color;
int cursor_x, cursor_y;
int mouse_x, mouse_y;
bool mouse_visible;
uint16_t mouse_orig_val;
int last_mouse_x, last_mouse_y;
bool mouse_orig_valid;
void clear_mouse_cursor() {
if (!mouse_visible || !mouse_orig_valid) return;
if (last_mouse_x >= 0 && last_mouse_x < VGA_WIDTH &&
last_mouse_y >= 0 && last_mouse_y < VGA_HEIGHT) {
vga_buffer[last_mouse_y * VGA_WIDTH + last_mouse_x] = mouse_orig_val;
}
mouse_orig_valid = false;
}

void save_mouse_char() {
if (!mouse_visible || mouse_x < 0 || mouse_x >= VGA_WIDTH ||
mouse_y < 0 || mouse_y >= VGA_HEIGHT) return;

int idx = mouse_y * VGA_WIDTH + mouse_x;
mouse_orig_val = vga_buffer[idx];
mouse_orig_valid = true;

last_mouse_x = mouse_x;
last_mouse_y = mouse_y;
}

void draw_mouse_cursor() {
if (!mouse_visible) return;
if (mouse_x < 0 || mouse_x >= VGA_WIDTH ||
mouse_y < 0 || mouse_y >= VGA_HEIGHT) return;

int idx = mouse_y * VGA_WIDTH + mouse_x;
vga_buffer[idx] = (0x0F << 8) | 0xDB;
}

void update_mouse_position() {
if (!Mouse::is_enabled()) return;

int new_x = Mouse::get_x();
int new_y = Mouse::get_y();

if (new_x != mouse_x || new_y != mouse_y) {
clear_mouse_cursor();
mouse_x = new_x;
mouse_y = new_y;
save_mouse_char();
draw_mouse_cursor();
}
}

void scroll() {
clear_mouse_cursor();
for (int y = 0; y < VGA_HEIGHT - 1; y++) {
for (int x = 0; x < VGA_WIDTH; x++) {
vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
}
}
for (int x = 0; x < VGA_WIDTH; x++) {
vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (color << 8) | ' ';
}
if (cursor_y > 0) cursor_y--;
mouse_orig_valid = false;
save_mouse_char();
draw_mouse_cursor();
}
public:
VGATerminal() : color(0x07), cursor_x(0), cursor_y(0),
mouse_x(40), mouse_y(12), mouse_visible(true),
mouse_orig_val(0), last_mouse_x(-1), last_mouse_y(-1),
mouse_orig_valid(false) {}
void set_color(uint8_t fg, uint8_t bg) {
color = (bg << 4) | fg;
}

void set_mouse_visible(bool v) {
mouse_visible = v;
}

void clear() {
clear_mouse_cursor();
for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
vga_buffer[i] = (color << 8) | ' ';
}
cursor_x = cursor_y = 0;
mouse_orig_valid = false;
last_mouse_x = -1;
last_mouse_y = -1;
save_mouse_char();
draw_mouse_cursor();
}

void clear_area(int x, int y, int w, int h) {
clear_mouse_cursor();
for (int row = y; row < y + h && row < VGA_HEIGHT; row++) {
for (int col = x; col < x + w && col < VGA_WIDTH; col++) {
vga_buffer[row * VGA_WIDTH + col] = (color << 8) | ' ';
}
}
mouse_orig_valid = false;
save_mouse_char();
draw_mouse_cursor();
}

void putchar(char c) {
clear_mouse_cursor();
if (c == '\n') {
cursor_x = 0;
cursor_y++;
if (cursor_y >= VGA_HEIGHT) scroll();
} else if (c == '\r') {
cursor_x = 0;
} else if (c == '\t') {
cursor_x = (cursor_x + 8) & ~7;
} else if (c == '\b') {
if (cursor_x > 0) {
cursor_x--;
vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | ' ';
}
} else if (c >= 32 && c <= 126) {
if (cursor_x >= VGA_WIDTH) {
cursor_x = 0;
cursor_y++;
if (cursor_y >= VGA_HEIGHT) scroll();
}
if (cursor_y < VGA_HEIGHT) {
vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | c;
cursor_x++;
}
}
mouse_orig_valid = false;
save_mouse_char();
draw_mouse_cursor();
}

void write(const char* str) {
for (int i = 0; str[i]; i++) {
putchar(str[i]);
}
}

void write_at(int x, int y, const char* str, uint8_t text_color) {
if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
clear_mouse_cursor();
uint8_t old_color = color;
color = text_color;
int pos = 0;
while (str[pos] && x + pos < VGA_WIDTH) {
vga_buffer[y * VGA_WIDTH + x + pos] = (color << 8) | str[pos];
pos++;
}
color = old_color;
mouse_orig_valid = false;
save_mouse_char();
draw_mouse_cursor();
}

void draw_box(int x, int y, int w, int h, uint8_t box_color) {
if (w < 2 || h < 2) return;
char buf[2] = {0, 0};
clear_mouse_cursor();
buf[0] = 0xC9; write_at(x, y, buf, box_color);
buf[0] = 0xBB; write_at(x + w - 1, y, buf, box_color);
buf[0] = 0xC8; write_at(x, y + h - 1, buf, box_color);
buf[0] = 0xBC; write_at(x + w - 1, y + h - 1, buf, box_color);

for (int i = x + 1; i < x + w - 1; i++) {
buf[0] = 0xCD; write_at(i, y, buf, box_color);
write_at(i, y + h - 1, buf, box_color);
}
for (int i = y + 1; i < y + h - 1; i++) {
buf[0] = 0xBA; write_at(x, i, buf, box_color);
write_at(x + w - 1, i, buf, box_color);
}
save_mouse_char();
draw_mouse_cursor();
}

void fill_rect(int x, int y, int w, int h, uint8_t rect_color, char fill_char) {
clear_mouse_cursor();
uint8_t old_color = color;
color = rect_color;
for (int row = y; row < y + h && row < VGA_HEIGHT; row++) {
for (int col = x; col < x + w && col < VGA_WIDTH; col++) {
vga_buffer[row * VGA_WIDTH + col] = (color << 8) | fill_char;
}
}
color = old_color;
mouse_orig_valid = false;
save_mouse_char();
draw_mouse_cursor();
}

void set_cursor(int x, int y) {
if (x >= 0 && x < VGA_WIDTH) cursor_x = x;
if (y >= 0 && y < VGA_HEIGHT) cursor_y = y;
}

int get_cursor_x() { return cursor_x; }
int get_cursor_y() { return cursor_y; }

void save_state(int &x, int &y, uint8_t &c) {
x = cursor_x;
y = cursor_y;
c = color;
}

void restore_state(int x, int y, uint8_t c) {
clear_mouse_cursor();
cursor_x = x;
cursor_y = y;
color = c;
mouse_orig_valid = false;
save_mouse_char();
draw_mouse_cursor();
}

void update_mouse() {
update_mouse_position();
}

bool is_mouse_over(int x, int y, int w, int h) {
int mx = Mouse::get_x();
int my = Mouse::get_y();
return (mx >= x && mx < x + w && my >= y && my < y + h);
}

bool is_mouse_clicked(int x, int y, int w, int h) {
bool clicked = is_mouse_over(x, y, w, h) && Mouse::is_left_clicked();
if (clicked) Mouse::reset_clicks();
return clicked;
}
};
class SystemMonitor {
private:
VGATerminal& term;
FileSystem& fs;
bool active;
uint32_t last_update;
void draw_ui() {
term.set_color(0x0F, 0x01);
term.fill_rect(0, 0, 80, 25, 0x01, ' ');

term.draw_box(1, 1, 78, 3, 0x3F);
term.write_at(30, 2, "SYSTEM MONITOR  ", 0x3F);
term.write_at(60, 2, "F10:Exit  ", 0x3F);

term.draw_box(1, 5, 38, 8, 0x2F);
term.write_at(15, 6, "File System  ", 0x2F);

char buffer[32];
int_to_str(fs.get_file_count(), buffer);
term.write_at(3, 8, "Files:  ", 0x0F);
term.write_at(15, 8, buffer, 0x0F);

int_to_str(fs.get_fs_size(), buffer);
term.write_at(3, 9, "Used:  ", 0x0F);
term.write_at(15, 9, buffer, 0x0F);
term.write_at(25, 9, "bytes  ", 0x0F);

int_to_str(fs.get_free_space(), buffer);
term.write_at(3, 10, "Free:  ", 0x0F);
term.write_at(15, 10, buffer, 0x0F);
term.write_at(25, 10, "bytes  ", 0x0F);

term.draw_box(41, 5, 38, 8, 0x2F);
term.write_at(55, 6, "Memory  ", 0x2F);

term.write_at(43, 8, "Kernel:  ", 0x0F);
term.write_at(60, 8, "64 KB  ", 0x0F);
term.write_at(43, 9, "Stack:  ", 0x0F);
term.write_at(60, 9, "32 KB  ", 0x0F);
term.write_at(43, 10, "Heap:  ", 0x0F);
term.write_at(60, 10, "128 KB  ", 0x0F);
term.write_at(43, 11, "FS:  ", 0x0F);
term.write_at(60, 11, "128 KB  ", 0x0F);

term.draw_box(1, 14, 78, 8, 0x2F);
term.write_at(35, 15, "System Status  ", 0x2F);

uint8_t hour, minute, second;
RTC::get_time(hour, minute, second);
char time_str[9];
time_str[0] = '0' + (hour / 10);
time_str[1] = '0' + (hour % 10);
time_str[2] = ':';
time_str[3] = '0' + (minute / 10);
time_str[4] = '0' + (minute % 10);
time_str[5] = ':';
time_str[6] = '0' + (second / 10);
time_str[7] = '0' + (second % 10);
time_str[8] = 0;

term.write_at(3, 17, "Current Time:  ", 0x0F);
term.write_at(25, 17, time_str, 0x0A);
term.write_at(3, 18, "System:  ", 0x0F);
term.write_at(25, 18, "EH-DSB v0.01  ", 0x0A);

term.fill_rect(2, 23, 3, 1, 0x4F, ' ');
term.write_at(2, 23, "[X] ", 0x0F);
}
public:
SystemMonitor(VGATerminal& t, FileSystem& f) : term(t), fs(f), active(false), last_update(0) {}
void open() {
active = true;
term.set_color(0x0F, 0x01);
term.clear();
draw_ui();
}

void close() { active = false; }
bool is_active() { return active; }

void handle_input(char c) {
if (!active) return;
if (c == (char)0xFA) {
close();
return;
}
}

void update() {
if (!active) return;
term.update_mouse();
if (term.is_mouse_clicked(60, 2, 8, 1)) {
close();
return;
}
if (term.is_mouse_clicked(2, 23, 3, 1)) {
close();
return;
}
if (RTC::should_update()) {
draw_ui();
}
}
};
class TextEditor {
private:
VGATerminal& term;
FileSystem& fs;
char buffer[MAX_FILE_SIZE];
int cursor;
int cursor_line;
int cursor_col;
int scroll_y;
bool active;
char current_filename[13];
bool modified;
void update_cursor_pos() {
cursor_line = 0;
cursor_col = 0;
for (int i = 0; i < cursor && buffer[i]; i++) {
if (buffer[i] == '\n') {
cursor_line++;
cursor_col = 0;
} else {
cursor_col++;
}
}
if (cursor_line < scroll_y) scroll_y = cursor_line;
if (cursor_line >= scroll_y + 16) scroll_y = cursor_line - 15;
}

void draw_content() {
term.fill_rect(3, 4, 74, 16, 0x17, ' ');

int line = scroll_y;
int screen_line = 4;
int buf_pos = 0;
int cur_line = 0;

while (cur_line < scroll_y && buffer[buf_pos]) {
if (buffer[buf_pos] == '\n') cur_line++;
buf_pos++;
}

while (screen_line < 20 && buffer[buf_pos] && line < scroll_y + 16) {
char num[4];
int_to_str(line + 1, num);
if (line + 1 < 10) {
term.write_at(2, screen_line, "    ", 0x08);
term.write_at(3, screen_line, num, 0x08);
} else {
term.write_at(2, screen_line, num, 0x08);
}

int col = 5;
while (buffer[buf_pos] && buffer[buf_pos] != '\n' && screen_line < 20) {
if (col < 77) {
if (buffer[buf_pos] >= 32 && buffer[buf_pos] <= 126) {
char ch[2] = {buffer[buf_pos], 0};
term.write_at(col, screen_line, ch, 0x0F);
col++;
} else if (buffer[buf_pos] == '\t') {
term.write_at(col, screen_line, "        ", 0x0F);
col += 8;
}
}
buf_pos++;
}

if (buffer[buf_pos] == '\n') buf_pos++;
screen_line++;
line++;
}

int disp_line = cursor_line - scroll_y + 4;
int disp_col = cursor_col + 5;
if (disp_line >= 4 && disp_line < 20 && disp_col < 77) {
term.write_at(disp_col, disp_line, "_  ", 0x0F);
}

char info[32];
int_to_str(cursor_line + 1, info);
term.write_at(3, 21, "Line:  ", 0x0F);
term.write_at(9, 21, info, 0x0F);
term.write_at(15, 21, "Col:  ", 0x0F);
int_to_str(cursor_col + 1, info);
term.write_at(20, 21, info, 0x0F);
if (modified) term.write_at(60, 21, "Modified  ", 0x0E);
}
public:
TextEditor(VGATerminal& t, FileSystem& f) : term(t), fs(f), cursor(0), cursor_line(0), cursor_col(0), scroll_y(0), active(false), modified(false) {
current_filename[0] = 0;
buffer[0] = 0;
}
void open(const char* filename = 0) {
active = true;
cursor = 0;
cursor_line = 0;
cursor_col = 0;
scroll_y = 0;
buffer[0] = 0;
modified = false;

if (filename && filename[0]) {
strncpy(current_filename, filename, 12);
uint32_t size;
if (fs.load_file(filename, buffer, size)) {
cursor = size;
buffer[cursor] = 0;
update_cursor_pos();
}
} else {
current_filename[0] = 0;
}

term.set_color(0x0F, 0x01);
term.clear();
draw_ui();
draw_content();
}

void close() {
if (modified && current_filename[0]) save_file();
active = false;
}

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

if (c == (char)0xF8) {
if (scroll_y > 0) scroll_y--;
draw_content();
return;
}

if (c == (char)0xF9) {
scroll_y++;
draw_content();
return;
}

if (c == '\b') {
if (cursor > 0) {
cursor--;
for (int i = cursor; i < MAX_FILE_SIZE - 1 && buffer[i]; i++) {
buffer[i] = buffer[i + 1];
}
modified = true;
update_cursor_pos();
draw_content();
}
} else if (c == '\n') {
if (cursor < MAX_FILE_SIZE - 2) {
for (int i = strlen(buffer); i >= cursor; i--) {
buffer[i + 1] = buffer[i];
}
buffer[cursor] = '\n';
cursor++;
modified = true;
update_cursor_pos();
draw_content();
}
} else if (c >= 32 && c <= 126 && cursor < MAX_FILE_SIZE - 2) {
for (int i = strlen(buffer); i >= cursor; i--) {
buffer[i + 1] = buffer[i];
}
buffer[cursor] = c;
cursor++;
modified = true;
update_cursor_pos();
draw_content();
} else if (c == (char)0xF6) {
if (cursor_line > 0) {
int new_cursor = 0;
int line = 0;
for (int i = 0; i < cursor && buffer[i]; i++) {
if (buffer[i] == '\n') line++;
}
for (int i = 0; buffer[i] && line > 0; i++) {
if (buffer[i] == '\n') line--;
new_cursor = i + 1;
}
cursor = new_cursor;
update_cursor_pos();
draw_content();
}
} else if (c == (char)0xF7) {
int new_cursor = cursor;
while (buffer[new_cursor] && buffer[new_cursor] != '\n') new_cursor++;
if (buffer[new_cursor] == '\n') new_cursor++;
cursor = new_cursor;
update_cursor_pos();
draw_content();
}
}

void save_file() {
if (current_filename[0] == 0) {
term.fill_rect(20, 10, 40, 5, 0x17, ' ');
term.draw_box(20, 10, 40, 5, 0x2F);
term.write_at(22, 11, "Filename:  ", 0x2F);
term.write_at(22, 12, "  >   ", 0x0F);

char filename[13] = {0};
int pos = 0;

while (true) {
if (Keyboard::is_key_pressed()) {
char ch = Keyboard::get_char();
if (ch == '\n') {
filename[pos] = 0;
break;
} else if (ch == '\b') {
if (pos > 0) {
pos--;
term.write_at(24 + pos, 12, "     ", 0x0F);
}
} else if (ch >= 32 && ch <= 126 && pos < 12) {
filename[pos] = ch;
pos++;
char ch_str[2] = {ch, 0};
term.write_at(24 + pos - 1, 12, ch_str, 0x0F);
} else if (ch == (char)0xFA) {
draw_ui();
draw_content();
return;
}
}
}

if (filename[0]) {
strncpy(current_filename, filename, 12);
} else {
draw_ui();
draw_content();
return;
}
}

if (fs.save_file(current_filename, buffer, strlen(buffer))) {
modified = false;
term.fill_rect(20, 10, 40, 3, 0x17, ' ');
term.draw_box(20, 10, 40, 3, 0x2F);
term.write_at(22, 11, "Saved!  ", 0x0A);
for (int i = 0; i < 300000; i++);
}

draw_ui();
draw_content();
}

void draw_ui() {
term.set_color(0x0F, 0x01);
term.draw_box(1, 1, 78, 21, 0x3F);

char title[64];
if (current_filename[0]) {
strcpy(title, "Editor - ");
strcat(title, current_filename);
} else {
strcpy(title, "Editor - New File ");
}
if (modified) strcat(title, " * ");

term.write_at(5, 2, title, 0x3F);
term.write_at(45, 2, "F1:Exit F4:Save  ", 0x3F);
term.write_at(45, 3, "F8:Up F9:Dn  ", 0x3F);
term.fill_rect(2, 23, 3, 1, 0x4F, ' ');
term.write_at(2, 23, "[X] ", 0x0F);
}

void update() {
if (!active) return;
term.update_mouse();
if (term.is_mouse_clicked(2, 23, 3, 1)) {
close();
return;
}
}

const char* get_current_filename() {
return current_filename;
}

bool is_modified() {
return modified;
}
};
class Calculator {
private:
VGATerminal& term;
char display[16];
int value;
char operation;
int operand;
bool active;
bool new_input;
public:
Calculator(VGATerminal& t) : term(t), value(0), operation(0), operand(0), active(false), new_input(true) {
display[0] = '0';
display[1] = 0;
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
if (len < 14) {
display[len] = c;
display[len + 1] = 0;
}
}
value = 0;
for (int i = 0; display[i]; i++) {
value = value * 10 + (display[i] - '0');
}
} else if (c == '+' || c == '-' || c == '*' || c == '/') {
if (operation != 0) calculate();
operation = c;
operand = value;
new_input = true;
} else if (c == '=' || c == '\n') {
calculate();
operation = 0;
new_input = true;
}
draw_display();
draw_buttons();
}

void calculate() {
switch (operation) {
case '+': value = operand + value; break;
case '-': value = operand - value; break;
case '*': value = operand * value; break;
case '/': if (value != 0) value = operand / value; break;
}
int_to_str(value, display);
}

void draw_ui() {
term.draw_box(10, 5, 60, 15, 0x2F);
term.write_at(15, 6, "Calculator - F2:Exit  ", 0x2F);
draw_display();
draw_buttons();
term.fill_rect(11, 23, 3, 1, 0x4F, ' ');
term.write_at(11, 23, "[X] ", 0x0F);
}

void draw_display() {
term.fill_rect(12, 8, 56, 3, 0x70, ' ');
term.write_at(13, 9, display, 0x0F);
}

void draw_buttons() {
const char* buttons = "789/456*123-0C=+";
for (int row = 0; row < 4; row++) {
for (int col = 0; col < 4; col++) {
int x = 12 + col * 14;
int y = 12 + row * 2;
term.draw_box(x, y, 12, 1, 0x70);
char button[2] = {buttons[row * 4 + col], 0};
term.write_at(x + 5, y, button, 0x0F);
}
}
}

void update() {
if (!active) return;
term.update_mouse();
if (term.is_mouse_clicked(11, 23, 3, 1)) {
close();
return;
}
for (int row = 0; row < 4; row++) {
for (int col = 0; col < 4; col++) {
int x = 12 + col * 14;
int y = 12 + row * 2;
if (term.is_mouse_clicked(x, y, 12, 1)) {
const char* buttons = "789/456*123-0C=+";
char c = buttons[row * 4 + col];
handle_input(c);
return;
}
}
}
}
};
class FileManager {
private:
VGATerminal& term;
FileSystem& fs;
TextEditor* editor;
int selected;
int page;
bool active;
bool delete_confirm;
bool rename_mode;
bool filter_mode;
bool edit_mode;
char filter[32];
char new_name[13];
int filter_pos;
uint16_t* screen_backup;
void backup_screen() {
if (!screen_backup) {
screen_backup = (uint16_t*)0x50000;
}
memcpy(screen_backup, vga_buffer, VGA_WIDTH * VGA_HEIGHT * 2);
}

void restore_screen() {
if (screen_backup) {
memcpy(vga_buffer, screen_backup, VGA_WIDTH * VGA_HEIGHT * 2);
}
}

void draw_ui() {
term.set_color(0x0F, 0x01);
term.clear();
term.draw_box(1, 1, 78, 21, 0x6F);
term.write_at(5, 2, "File Manager - F3:Exit  ", 0x6F);
term.write_at(3, 4, "Name  ", 0x6F);
term.write_at(40, 4, "Size  ", 0x6F);
term.write_at(60, 4, "Type  ", 0x6F);
term.fill_rect(3, 6, 74, 14, 0x17, ' ');

int items_per_page = 14;
int start = page * items_per_page;
int count = 0;
int file_count = fs.get_file_count();

for (int i = start; i < file_count && count < items_per_page; i++) {
FileEntry* file = fs.get_file(i);
if (!file) continue;

if (filter[0] && !strstr(file->name, filter)) continue;

int y = 6 + count;
uint8_t color = (count == selected) ? 0x70 : 0x0F;

char line[40];
strcpy(line, file->name);
if (file->read_only) strcat(line, " [RO]");
term.write_at(5, y, line, color);

char size_str[16];
int_to_str(file->size, size_str);
term.write_at(40, y, size_str, color);

const char* ext = strstr(file->name, ".");
if (ext && (strcmp(ext, ".TXT") == 0 || strcmp(ext, ".txt") == 0)) {
term.write_at(60, y, "Text  ", color);
} else if (ext && (strcmp(ext, ".BF") == 0 || strcmp(ext, ".bf") == 0)) {
term.write_at(60, y, "BF  ", color);
} else {
term.write_at(60, y, "File  ", color);
}

if (count == selected) term.write_at(3, y, " >", color);
count++;
}

char info[32];
int_to_str(page + 1, info);
strcat(info, "/");
int total_pages = (file_count + 13) / 14;
if (total_pages == 0) total_pages = 1;
char pages[8];
int_to_str(total_pages, pages);
strcat(info, pages);
term.write_at(3, 20, "Page:  ", 0x0F);
term.write_at(9, 20, info, 0x0F);

if (delete_confirm) {
term.write_at(3, 21, "Delete? (Y/N)  ", 0x0C);
} else if (rename_mode) {
term.write_at(3, 21, "New name:  ", 0x0E);
term.write_at(13, 21, new_name, 0x0E);
term.write_at(13 + strlen(new_name), 21, "_  ", 0x0E);
} else if (filter_mode) {
term.write_at(3, 21, "Filter:  ", 0x0E);
term.write_at(11, 21, filter, 0x0E);
term.write_at(11 + strlen(filter), 21, "_  ", 0x0E);
} else {
term.write_at(3, 21, "j/k:Move Space:Page Enter:Open d:Del r:Rename t:RO /:Filter", 0x0F);
}
term.write_at(50, 20, "Open ", 0x0F);
term.write_at(59, 20, "Edit ", 0x0F);
term.write_at(68, 20, "Del ", 0x0F);
}

void open_selected() {
FileEntry* file = fs.get_file(selected + page * 14);
if (!file) return;

backup_screen();
term.clear();
term.draw_box(0, 0, 80, 23, 0x6F);
term.write_at(2, 1, "File:  ", 0x6F);
term.write_at(8, 1, file->name, 0x6F);
term.write_at(60, 1, "F10:Exit  ", 0x6F);

char content[MAX_FILE_SIZE];
uint32_t size;
if (fs.load_file(file->name, content, size)) {
int line = 3;
int col = 2;
for (uint32_t i = 0; i < size && line < 22; i++) {
if (content[i] == '\n') {
line++;
col = 2;
if (line >= 22) break;
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
Mouse::update();
term.update_mouse();
io_wait();
if (Keyboard::is_key_pressed()) {
char c = Keyboard::get_char();
if (c == (char)0xFA || c == '\n') break;
}
}

restore_screen();
draw_ui();
}

void edit_selected() {
FileEntry* file = fs.get_file(selected + page * 14);
if (!file) return;
if (file->read_only) {
term.fill_rect(20, 10, 40, 3, 0x17, ' ');
term.draw_box(20, 10, 40, 3, 0x2F);
term.write_at(22, 11, "File is read-only!  ", 0x0C);
for (int i = 0; i < 300000; i++);
draw_ui();
return;
}

if (editor) {
editor->open(file->name);
edit_mode = true;
}
}

void toggle_readonly() {
FileEntry* file = fs.get_file(selected + page * 14);
if (!file) return;

if (strcmp(file->name, "README.TXT") == 0) {
term.fill_rect(20, 10, 40, 3, 0x17, ' ');
term.draw_box(20, 10, 40, 3, 0x2F);
term.write_at(22, 11, "Cannot change README!  ", 0x0C);
for (int i = 0; i < 300000; i++);
draw_ui();
return;
}

fs.toggle_readonly(file->name);
draw_ui();
}
public:
FileManager(VGATerminal& t, FileSystem& f, TextEditor* e = 0) : term(t), fs(f), editor(e), selected(0), page(0), active(false),
delete_confirm(false), rename_mode(false), filter_mode(false), edit_mode(false), filter_pos(0) {
filter[0] = 0;
new_name[0] = 0;
screen_backup = 0;
}
void open() {
active = true;
edit_mode = false;
selected = 0;
page = 0;
delete_confirm = false;
rename_mode = false;
filter_mode = false;
filter[0] = 0;
term.set_color(0x0F, 0x01);
term.clear();
draw_ui();
}

void close() { active = false; }
bool is_active() { return active; }
bool is_edit_mode() { return edit_mode; }
void set_edit_mode(bool mode) { edit_mode = mode; }

void handle_input(char c) {
if (!active) return;

if (c == (char)0xF3) {
close();
return;
}

if (delete_confirm) {
if (c == 'y' || c == 'Y') {
FileEntry* file = fs.get_file(selected + page * 14);
if (file && !file->read_only) {
fs.delete_file(file->name);
if (selected > 0) selected--;
}
delete_confirm = false;
} else if (c == 'n' || c == 'N' || c == (char)0xFA) {
delete_confirm = false;
}
draw_ui();
return;
}

if (rename_mode) {
if (c == '\n') {
FileEntry* file = fs.get_file(selected + page * 14);
if (file && !file->read_only && new_name[0]) {
fs.rename_file(file->name, new_name);
}
rename_mode = false;
new_name[0] = 0;
} else if (c == '\b') {
int len = strlen(new_name);
if (len > 0) new_name[len - 1] = 0;
} else if (c >= 32 && c <= 126 && strlen(new_name) < 12) {
int len = strlen(new_name);
new_name[len] = c;
new_name[len + 1] = 0;
} else if (c == (char)0xFA) {
rename_mode = false;
new_name[0] = 0;
}
draw_ui();
return;
}

if (filter_mode) {
if (c == '\n') {
filter_mode = false;
} else if (c == '\b') {
if (filter_pos > 0) {
filter_pos--;
filter[filter_pos] = 0;
}
} else if (c >= 32 && c <= 126 && filter_pos < 31) {
filter[filter_pos] = c;
filter_pos++;
filter[filter_pos] = 0;
} else if (c == 'c' || c == 'C') {
filter[0] = 0;
filter_pos = 0;
} else if (c == (char)0xFA) {
filter_mode = false;
filter[0] = 0;
filter_pos = 0;
}
draw_ui();
return;
}

if (c == '\n') {
open_selected();
return;
}

if (c == 'e' || c == 'E') {
edit_selected();
return;
}

if (c == 't' || c == 'T') {
toggle_readonly();
return;
}

if (c == 'j' || c == 'J') {
int items_per_page = 14;
int file_count = fs.get_file_count();
if (selected < items_per_page - 1 && selected + page * items_per_page < file_count - 1) {
selected++;
}
} else if (c == 'k' || c == 'K') {
if (selected > 0) selected--;
} else if (c == ' ') {
page++;
if (page * 14 >= fs.get_file_count()) page = 0;
selected = 0;
} else if (c == 'd' || c == 'D') {
FileEntry* file = fs.get_file(selected + page * 14);
if (file && !file->read_only) delete_confirm = true;
} else if (c == 'r' || c == 'R') {
FileEntry* file = fs.get_file(selected + page * 14);
if (file && !file->read_only) {
rename_mode = true;
strncpy(new_name, file->name, 12);
}
} else if (c == '/') {
filter_mode = true;
filter_pos = 0;
filter[0] = 0;
} else if (c == (char)0x1B) {
filter[0] = 0;
filter_pos = 0;
filter_mode = false;
}

draw_ui();
}

void update() {
if (!active) return;
term.update_mouse();
if (term.is_mouse_clicked(50, 20, 7, 1)) {
open_selected();
return;
}
if (term.is_mouse_clicked(59, 20, 7, 1)) {
edit_selected();
return;
}
if (term.is_mouse_clicked(68, 20, 7, 1)) {
FileEntry* file = fs.get_file(selected + page * 14);
if (file && !file->read_only) delete_confirm = true;
draw_ui();
return;
}
int items_per_page = 14;
for (int i = 0; i < items_per_page; i++) {
int y = 6 + i;
if (term.is_mouse_clicked(5, y, 35, 1)) {
selected = i;
open_selected();
return;
}
}
}
};
class BrainfuckIDE {
private:
VGATerminal& term;
FileSystem& fs;
char code[2048];
int cursor;
bool active;
bool running;
bool input_mode;
char input_buffer[256];
int input_pos;
int saved_x, saved_y;
uint8_t saved_color;
int find_match(const char* prog, int pos, char open, char close, bool forward) {
int depth = 1;
while (true) {
if (forward) pos++; else pos--;
if (pos < 0 || prog[pos] == 0) return -1;
if (prog[pos] == open) depth++;
else if (prog[pos] == close) depth--;
if (depth == 0) return pos;
}
}

void run_program() {
running = true;
input_mode = false;

term.save_state(saved_x, saved_y, saved_color);
term.set_color(0x0F, 0x00);
term.clear();

term.write("Brainfuck Program Output\n");
term.write("========================\n");
term.write("Press F10 to stop\n\n");

uint8_t memory[30000];
for (int i = 0; i < 30000; i++) memory[i] = 0;
int ptr = 0;
int pc = 0;
int steps = 0;
const int MAX_STEPS = 100000;

while (code[pc] && steps < MAX_STEPS && running) {
char c = code[pc];
steps++;

switch (c) {
case '>': ptr = (ptr + 1) % 30000; break;
case '<': ptr = (ptr == 0) ? 29999 : ptr - 1; break;
case '+': memory[ptr]++; break;
case '-': memory[ptr]--; break;
case '.':
if (memory[ptr] >= 32 && memory[ptr] <= 126) {
term.putchar(memory[ptr]);
} else if (memory[ptr] == 10) {
term.putchar('\n');
}
break;
case ',':
if (!input_mode) {
input_mode = true;
input_pos = 0;
term.write("\n[Input] ");
return;
}
memory[ptr] = input_buffer[0];
input_mode = false;
break;
case '[':
if (memory[ptr] == 0) {
int match = find_match(code, pc, '[', ']', true);
if (match == -1) {
term.write("\nError: Unmatched [ ");
running = false;
return;
}
pc = match;
}
break;
case ']':
if (memory[ptr] != 0) {
int match = find_match(code, pc, ']', '[', false);
if (match == -1) {
term.write("\nError: Unmatched ] ");
running = false;
return;
}
pc = match;
}
break;
}
pc++;
}

if (steps >= MAX_STEPS) {
term.write("\n\nProgram stopped: too many steps ");
} else if (running) {
term.write("\n\nProgram finished ");
}

term.write("\n\nPress any key to return to editor ");
Keyboard::flush();
while (!Keyboard::is_key_pressed()) {}
Keyboard::flush();

running = false;
input_mode = false;
term.restore_state(saved_x, saved_y, saved_color);
draw_editor();
}

void draw_editor() {
term.set_color(0x0F, 0x01);
term.clear();

term.draw_box(0, 0, 80, 3, 0x4F);
term.write_at(2, 1, "Brainfuck IDE ", 0x4F);
term.write_at(30, 1, "F5:Run F7:Save F8:Load F9:Examples F10:Exit ", 0x4F);

term.draw_box(0, 3, 80, 18, 0x3F);
term.fill_rect(1, 4, 78, 16, 0x17, ' ');

int line = 4;
int col = 1;
for (int i = 0; code[i] && line < 20; i++) {
if (code[i] == '\n') {
line++;
col = 1;
continue;
}
if (col >= 79) {
line++;
col = 1;
continue;
}
if (code[i] >= 32 && code[i] <= 126) {
char ch[2] = {code[i], 0};
term.write_at(col, line, ch, 0x0F);
col++;
}
}

int cursor_line = 4;
int cursor_col = 1;
for (int i = 0; i < cursor && code[i]; i++) {
if (code[i] == '\n') {
cursor_line++;
cursor_col = 1;
} else {
cursor_col++;
if (cursor_col >= 79) {
cursor_line++;
cursor_col = 1;
}
}
}
if (cursor_line < 20 && cursor_col < 79) {
term.write_at(cursor_col, cursor_line, "_ ", 0x0E);
}

char info[32];
int_to_str(strlen(code), info);
term.write_at(2, 21, "Length: ", 0x0F);
term.write_at(10, 21, info, 0x0F);
term.write_at(2, 22, "F5:Run F7:Save F8:Load F9:Examples F10:Exit ", 0x70);
}

void load_example(int num) {
char buffer[2048];
uint32_t size;
switch (num) {
case 1:
if (fs.load_file("HELLO.BF", buffer, size)) {
strcpy(code, buffer);
}
break;
case 2:
if (fs.load_file("ECHO.BF", buffer, size)) {
strcpy(code, buffer);
}
break;
}
cursor = strlen(code);
draw_editor();
}
public:
BrainfuckIDE(VGATerminal& t, FileSystem& f) : term(t), fs(f), cursor(0), active(false), running(false), input_mode(false), input_pos(0) {
code[0] = 0;
}
void open() {
active = true;
running = false;
input_mode = false;
cursor = 0;
code[0] = 0;
draw_editor();
}

void close() { active = false; }
bool is_active() { return active; }
bool is_running() { return running; }

void handle_input(char c) {
if (!active) return;

if (running) {
if (input_mode) {
if (c == '\n') {
input_buffer[input_pos] = 0;
term.write("\n");
run_program();
} else if (c == '\b') {
if (input_pos > 0) {
input_pos--;
term.write("\b \b");
}
} else if (c >= 32 && c <= 126 && input_pos < 255) {
input_buffer[input_pos] = c;
input_pos++;
term.putchar(c);
} else if (c == (char)0xFA) {
running = false;
input_mode = false;
term.restore_state(saved_x, saved_y, saved_color);
draw_editor();
}
} else {
if (c == (char)0xFA) {
running = false;
term.restore_state(saved_x, saved_y, saved_color);
draw_editor();
}
}
return;
}

if (c == (char)0xFA) {
close();
return;
}

if (c == (char)0xF5) {
if (code[0] != 0) {
run_program();
}
return;
}

if (c == (char)0xF7) {
if (fs.save_file("PROGRAM.BF", code, strlen(code))) {
term.write_at(2, 21, "Saved! ", 0x0A);
} else {
term.write_at(2, 21, "Save failed! ", 0x0C);
}
return;
}

if (c == (char)0xF8) {
char buffer[2048];
uint32_t size;
if (fs.load_file("PROGRAM.BF", buffer, size)) {
strcpy(code, buffer);
cursor = size;
term.write_at(2, 21, "Loaded! ", 0x0A);
} else {
term.write_at(2, 21, "Load failed! ", 0x0C);
}
draw_editor();
return;
}

if (c == (char)0xF9) {
term.fill_rect(20, 10, 40, 6, 0x17, ' ');
term.draw_box(20, 10, 40, 6, 0x5F);
term.write_at(22, 11, "Examples: ", 0x5F);
term.write_at(22, 12, "1. Hello World ", 0x0F);
term.write_at(22, 13, "2. Echo ", 0x0F);

while (true) {
if (Keyboard::is_key_pressed()) {
char ch = Keyboard::get_char();
if (ch == '1' || ch == '2') {
load_example(ch - '0');
break;
} else if (ch == (char)0xFA) {
break;
}
}
}
draw_editor();
return;
}

if (c == '\b') {
if (cursor > 0) {
cursor--;
for (int i = cursor; code[i]; i++) {
code[i] = code[i+1];
}
}
} else if (c == '\n') {
if (cursor < 2046) {
for (int i = strlen(code); i >= cursor; i--) {
code[i+1] = code[i];
}
code[cursor] = '\n';
cursor++;
}
} else if (c >= 32 && c <= 126 && cursor < 2046) {
for (int i = strlen(code); i >= cursor; i--) {
code[i+1] = code[i];
}
code[cursor] = c;
cursor++;
}

draw_editor();
}
};
class TerminalShell {
private:
VGATerminal& term;
FileSystem& fs;
char input_buffer[MAX_INPUT_LEN];
int cursor;
char history[MAX_COMMAND_HISTORY][MAX_INPUT_LEN];
int history_count;
int history_pos;
bool command_mode;
bool history_browsing;
char temp_buffer[MAX_INPUT_LEN];
void add_to_history(const char* cmd) {
if (!cmd || cmd[0] == 0) return;
if (history_count > 0 && strcmp(history[history_count - 1], cmd) == 0) return;

if (history_count < MAX_COMMAND_HISTORY) {
strncpy(history[history_count], cmd, MAX_INPUT_LEN - 1);
history_count++;
} else {
for (int i = 0; i < MAX_COMMAND_HISTORY - 1; i++) {
strcpy(history[i], history[i + 1]);
}
strncpy(history[MAX_COMMAND_HISTORY - 1], cmd, MAX_INPUT_LEN - 1);
}
history_pos = history_count;
}

void list_files() {
term.write("\n");
int count = fs.get_file_count();

for (int i = 0; i < count; i++) {
FileEntry* file = fs.get_file(i);
if (!file) continue;

char line[80];
strcpy(line, "       ");
strcat(line, file->name);
int name_len = strlen(line);
for (int j = name_len; j < 20; j++) strcat(line, " ");

char size_str[16];
int_to_str(file->size, size_str);
strcat(line, size_str);
for (int j = strlen(size_str); j < 8; j++) strcat(line, " ");

strcat(line, " bytes ");
if (file->read_only) strcat(line, " [RO] ");
term.write(line);
term.write("\n");
}

char total[32];
int_to_str(count, total);
term.write("\nTotal: ");
term.write(total);
term.write(" files\n");
}

void cat_file(const char* filename) {
if (!filename || strlen(filename) == 0) {
term.write("Usage: cat <filename>\n");
return;
}

char content[MAX_FILE_SIZE];
uint32_t size;

if (fs.load_file(filename, content, size)) {
term.write("\n");
term.write(content);
if (size > 0 && content[size - 1] != '\n') term.write("\n");
} else {
term.write("File '");
term.write(filename);
term.write("' not found.\n");
}
}

void show_help() {
term.write("\nCommands:\n");
term.write("  help/?       - Show this help\n");
term.write("  ls/dir       - List files\n");
term.write("  cat <file>   - View file\n");
term.write("  rm <file>    - Delete file\n");
term.write("  mv <old> <new> - Rename\n");
term.write("  time         - Show time\n");
term.write("  clear/cls    - Clear screen\n");
term.write("  about        - System info\n");
term.write("  history      - Command history\n");
term.write("  reboot       - Reboot system\n");
term.write("  echo <text>  - Print text\n");
term.write("  mem          - Memory info\n");
term.write("  info         - System information\n");
term.write("  tz <offset>  - Set timezone (-12 to +12)\n\n");
}

void show_time() {
uint8_t hour, minute, second;
RTC::get_time(hour, minute, second);
char time_str[16];
time_str[0] = '0' + (hour / 10);
time_str[1] = '0' + (hour % 10);
time_str[2] = ':';
time_str[3] = '0' + (minute / 10);
time_str[4] = '0' + (minute % 10);
time_str[5] = ':';
time_str[6] = '0' + (second / 10);
time_str[7] = '0' + (second % 10);
time_str[8] = 0;
term.write("\n");
term.write(time_str);
term.write("\n");

char tz_str[8];
int_to_str(RTC::get_timezone(), tz_str);
term.write("Timezone: UTC");
if (RTC::get_timezone() >= 0) term.write("+");
term.write(tz_str);
term.write("\n");
}

void show_version() {
term.write("\nEH-DSB v0.01\n");
term.write("Public Domain (Unlicense)\n");
term.write("by quik/QUIK1001\n");
}

void show_memory_info() {
char fs_size[16];
int_to_str(fs.get_fs_size(), fs_size);
term.write("  FS Used: ");
term.write(fs_size);
term.write(" bytes\n");
char fs_free[16];
int_to_str(fs.get_free_space(), fs_free);
term.write("  FS Free: ");
term.write(fs_free);
term.write(" bytes\n");
}

void show_system_info() {
term.write("\nInformation:\n");
term.write("  EH-DSB v0.01\n");
term.write("  Public Domain\n");
term.write("  quik/QUIK1001\n");
char file_count[8];
int_to_str(fs.get_file_count(), file_count);
term.write("  Files: ");
term.write(file_count);
term.write("\n");
uint8_t hour, minute, second;
RTC::get_time(hour, minute, second);
char time_str[16];
time_str[0] = '0' + (hour / 10);
time_str[1] = '0' + (hour % 10);
time_str[2] = ':';
time_str[3] = '0' + (minute / 10);
time_str[4] = '0' + (minute % 10);
time_str[5] = ':';
time_str[6] = '0' + (second / 10);
time_str[7] = '0' + (second % 10);
time_str[8] = 0;
term.write("  Time: ");
term.write(time_str);
term.write("\n");
char tz_str[8];
int_to_str(RTC::get_timezone(), tz_str);
term.write("  Timezone: UTC");
if (RTC::get_timezone() >= 0) term.write("+");
term.write(tz_str);
term.write("\n");
}

void do_reboot() {
term.write("\nRebooting...\n");
for (int i = 0; i < 500000; i++);
outb(0x64, 0xFE);
while (1) {
asm volatile("hlt");
}
}

void set_timezone(const char* arg) {
while (*arg == ' ') arg++;
int offset = 0;
bool negative = false;
if (*arg == '-') {
negative = true;
arg++;
}
while (*arg >= '0' && *arg <= '9') {
offset = offset * 10 + (*arg - '0');
arg++;
}
if (negative) offset = -offset;
if (offset >= -12 && offset <= 12) {
RTC::set_timezone(offset);
char tz_str[8];
int_to_str(offset, tz_str);
term.write("\nTimezone set to UTC");
if (offset >= 0) term.write("+");
term.write(tz_str);
term.write("\n");
} else {
term.write("\nInvalid timezone. Use -12 to +12\n");
}
}

void execute_command() {
if (cursor == 0) return;

input_buffer[cursor] = 0;
add_to_history(input_buffer);

char* cmd = input_buffer;
while (*cmd == ' ')  cmd++;

if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
show_help();
} else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "dir") == 0) {
list_files();
} else if (strncmp(cmd, "cat ", 4) == 0) {
cat_file(cmd + 4);
} else if (strcmp(cmd, "time") == 0) {
show_time();
} else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
term.clear();
term.write("EH-DSB v0.01 Terminal\n");
term.write("==================\n");
} else if (strcmp(cmd, "about") == 0) {
term.write("\nEH-DSB v0.01\n");
term.write("Public Domain (Experiment)\n");
term.write("by quik/QUIK1001\n\n");
} else if (strcmp(cmd, "history") == 0) {
term.write("\n");
for (int i = 0; i < history_count; i++) {
char num[8];
int_to_str(i + 1, num);
term.write("   ");
term.write(num);
term.write(": ");
term.write(history[i]);
term.write("\n");
}
} else if (strncmp(cmd, "rm ", 3) == 0) {
const char* filename = cmd + 3;
while (*filename == ' ') filename++;
if (fs.delete_file(filename)) {
term.write("\nFile deleted.\n");
} else {
term.write("\nDelete failed.\n");
}
} else if (strncmp(cmd, "mv ", 3) == 0) {
const char* arg = cmd + 3;
while (*arg == ' ') arg++;
const char* space = strstr(arg, " ");
if (space) {
char old_name[13], new_name[13];
int i;
for (i = 0; i < 12 && arg + i < space; i++) old_name[i] = arg[i];
old_name[i] = 0;
const char* new_part = space + 1;
while (*new_part == ' ') new_part++;
strncpy(new_name, new_part, 12);
if (fs.rename_file(old_name, new_name)) {
term.write("\nFile renamed.\n");
} else {
term.write("\nRename failed.\n");
}
} else {
term.write("\nUsage: mv <old> <new>\n");
}
} else if (strcmp(cmd, "reboot") == 0) {
do_reboot();
} else if (strncmp(cmd, "echo ", 5) == 0) {
term.write("\n");
term.write(cmd + 5);
term.write("\n");
} else if (strcmp(cmd, "mem") == 0) {
show_memory_info();
} else if (strcmp(cmd, "info") == 0) {
show_system_info();
} else if (strncmp(cmd, "tz ", 3) == 0) {
set_timezone(cmd + 3);
} else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
command_mode = false;
return;
} else if (cmd[0] != 0) {
term.write("\nUnknown command. Type 'help'\n");
}

term.write("\nehdsb> ");
}

void clear_input_line() {
for (int i = 0; i < cursor; i++) {
term.write("\b \b");
}
}

void restore_input_line() {
term.write(input_buffer);
}
public:
TerminalShell(VGATerminal& t, FileSystem& f)
: term(t), fs(f), cursor(0), history_count(0), history_pos(0), command_mode(false),
history_browsing(false) {
input_buffer[0] = 0;
temp_buffer[0] = 0;
}
void open() {
command_mode = true;
cursor = 0;
input_buffer[0] = 0;
temp_buffer[0] = 0;
history_pos = 0;
history_browsing = false;
term.set_color(0x0F, 0x01);
term.clear();
term.write("EH-DSB v0.01 Terminal\n");
term.write("==================\n");
term.write("Type 'help' for commands. F4 to exit.\n\nehdsb> ");
term.set_mouse_visible(false);
}

bool is_active() { return command_mode; }

bool handle_input() {
if (!command_mode) return false;

if (Keyboard::is_key_pressed()) {
char c = Keyboard::get_char();
if (c == 0) return true;

if (c == (char)0xF4) {
command_mode = false;
return false;
}

if (c == (char)0x10) {
if (history_count > 0 && history_pos > 0) {
if (!history_browsing) {
strcpy(temp_buffer, input_buffer);
history_browsing = true;
}
history_pos--;
clear_input_line();
strcpy(input_buffer, history[history_pos]);
cursor = strlen(input_buffer);
restore_input_line();
}
return true;
}

if (c == (char)0x11) {
if (history_browsing && history_pos < history_count - 1) {
history_pos++;
clear_input_line();
if (history_pos == history_count) {
strcpy(input_buffer, temp_buffer);
} else {
strcpy(input_buffer, history[history_pos]);
}
cursor = strlen(input_buffer);
restore_input_line();
} else if (history_browsing && history_pos >= history_count - 1) {
history_browsing = false;
history_pos = history_count;
clear_input_line();
strcpy(input_buffer, temp_buffer);
cursor = strlen(input_buffer);
restore_input_line();
}
return true;
}

if (c == '\n') {
if (history_browsing) {
history_browsing = false;
history_pos = history_count;
}
term.write("\n");
execute_command();
cursor = 0;
input_buffer[0] = 0;
temp_buffer[0] = 0;
} else if (c == '\b') {
if (cursor > 0) {
cursor--;
input_buffer[cursor] = 0;
term.write("\b \b");
}
} else if (c >= 32 && c <= 126 && cursor < MAX_INPUT_LEN - 1) {
input_buffer[cursor] = c;
cursor++;
input_buffer[cursor] = 0;
term.putchar(c);
}
}
return true;
}

void update() {
if (!command_mode) return;
}
};
class ClockDisplay {
private:
VGATerminal& term;
uint8_t last_hour, last_minute;
char time_str[6];
public:
ClockDisplay(VGATerminal& t) : term(t), last_hour(0), last_minute(0) {
time_str[0] = '0'; time_str[1] = '0'; time_str[2] = ':';
time_str[3] = '0'; time_str[4] = '0'; time_str[5] = 0;
}
void update() {
if (!RTC::should_update()) return;

uint8_t hour, minute, second;
RTC::get_time(hour, minute, second);

if (hour == last_hour && minute == last_minute) return;

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
class Desktop {
private:
VGATerminal term;
FileSystem fs;
ClockDisplay clock;
TextEditor editor;
Calculator calculator;
FileManager fileman;
BrainfuckIDE brainfuck;
SystemMonitor monitor;
TerminalShell terminal;
enum App { NONE, EDITOR, CALC, FILEMAN, BF, TERMINAL, MONITOR } current_app;
void draw_desktop() {
term.set_color(0x0F, 0x01);
term.clear();
term.draw_box(0, 0, 80, 3, 0x3F);
term.write_at(2, 1, "EH-DSB v0.01 - Public Domain  ", 0x3F);

clock.draw();

term.fill_rect(0, 3, 80, 19, 0x17, ' ');

term.write_at(2, 5, "Welcome to EH-DSB v0.01!  ", 0x0F);

char count[8];
int_to_str(fs.get_file_count(), count);
term.write_at(2, 7, "Files:  ", 0x0F);
term.write_at(10, 7, count, 0x0F);

char free_space[8];
int_to_str(fs.get_free_space(), free_space);
term.write_at(2, 8, "Free space:  ", 0x0F);
term.write_at(15, 8, free_space, 0x0F);
term.write_at(23, 8, "bytes  ", 0x0F);

term.write_at(2, 10, "Quick Access:  ", 0x0E);
term.write_at(2, 11, "F1 - Text Editor  ", 0x0F);
term.write_at(2, 12, "F2 - Calculator  ", 0x0F);
term.write_at(2, 13, "F3 - File Manager  ", 0x0F);
term.write_at(2, 14, "F4 - Terminal  ", 0x0F);
term.write_at(2, 15, "F5 - Brainfuck IDE  ", 0x0F);
term.write_at(2, 16, "F6 - System Monitor  ", 0x0F);

term.write_at(2, 19, "Press F1-F6 for apps  ", 0x07);
term.write_at(2, 20, "by quik/QUIK1001 - Public Domain  ", 0x08);
term.set_mouse_visible(true);
}
public:
Desktop() : fs(), clock(term), editor(term, fs), calculator(term), fileman(term, fs, &editor),
brainfuck(term, fs), monitor(term, fs), terminal(term, fs), current_app(NONE) {
RTC::init();
}
void run() {
Mouse::init();
draw_desktop();

while (true) {
Mouse::update();
clock.update();
RTC::tick();

if (current_app == EDITOR) {
editor.update();
if (Keyboard::is_key_pressed()) {
char c = Keyboard::get_char();
if (c != 0) editor.handle_input(c);
}
if (!editor.is_active()) {
if (fileman.is_edit_mode()) {
current_app = FILEMAN;
fileman.set_edit_mode(false);
fileman.open();
} else {
current_app = NONE;
draw_desktop();
}
}
} else if (current_app == CALC) {
calculator.update();
if (Keyboard::is_key_pressed()) {
char c = Keyboard::get_char();
if (c != 0) calculator.handle_input(c);
}
if (!calculator.is_active()) {
current_app = NONE;
draw_desktop();
}
} else if (current_app == FILEMAN) {
fileman.update();
if (Keyboard::is_key_pressed()) {
char c = Keyboard::get_char();
if (c != 0) fileman.handle_input(c);
}
if (fileman.is_edit_mode()) {
current_app = EDITOR;
} else if (!fileman.is_active()) {
current_app = NONE;
draw_desktop();
}
} else if (current_app == BF) {
if (Keyboard::is_key_pressed()) {
char c = Keyboard::get_char();
if (c != 0) brainfuck.handle_input(c);
}
if (!brainfuck.is_active() && !brainfuck.is_running()) {
current_app = NONE;
draw_desktop();
}
} else if (current_app == MONITOR) {
monitor.update();
if (Keyboard::is_key_pressed()) {
char c = Keyboard::get_char();
if (c != 0) monitor.handle_input(c);
}
if (!monitor.is_active()) {
current_app = NONE;
draw_desktop();
}
} else if (current_app == TERMINAL) {
terminal.update();
if (!terminal.handle_input()) {
current_app = NONE;
draw_desktop();
}
} else {
term.update_mouse();
if (Keyboard::is_key_pressed()) {
char c = Keyboard::get_char();
if (c != 0) {
if (c == (char)0xF1) {
current_app = EDITOR;
editor.open();
} else if (c == (char)0xF2) {
current_app = CALC;
calculator.open();
} else if (c == (char)0xF3) {
current_app = FILEMAN;
fileman.open();
} else if (c == (char)0xF4) {
current_app = TERMINAL;
terminal.open();
} else if (c == (char)0xF5) {
current_app = BF;
brainfuck.open();
} else if (c == (char)0xF6) {
current_app = MONITOR;
monitor.open();
}
}
}
if (term.is_mouse_clicked(2, 11, 18, 1)) {
current_app = EDITOR;
editor.open();
} else if (term.is_mouse_clicked(2, 12, 18, 1)) {
current_app = CALC;
calculator.open();
} else if (term.is_mouse_clicked(2, 13, 20, 1)) {
current_app = FILEMAN;
fileman.open();
} else if (term.is_mouse_clicked(2, 14, 16, 1)) {
current_app = TERMINAL;
terminal.open();
} else if (term.is_mouse_clicked(2, 15, 20, 1)) {
current_app = BF;
brainfuck.open();
} else if (term.is_mouse_clicked(2, 16, 22, 1)) {
current_app = MONITOR;
monitor.open();
}
}

for (int i = 0; i < 10000; i++);
}
}
};
extern "C" void kernel_main() {
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
while (1) {}
}
extern "C" void __stack_chk_fail() {
while (1) {}
}
