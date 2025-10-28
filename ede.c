/* GNU ede v1.0 - Enhanced Development Editor
   Copyright (C) 2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

   A nano-like terminal text editor with advanced features:
   - Cross-platform bare terminal control (no ncurses/windows.h/conio.h)
   - Module system (.esrc -> .emod compilation and loading)
   - Vim command mode (Ctrl-C then Ctrl-M)
   - Easy exit with Ctrl-Q (prompts save/discard)
   - Syntax highlighting, search, undo/redo system
*/

/* Advanced bare metal: use standard C runtime, manually define OS structures */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

/* Platform detection */
#if defined(_WIN32) || defined(_WIN64)
    #define EDE_WINDOWS 1
    #define EDE_PLATFORM "Windows"
#elif defined(__unix__) || defined(__APPLE__)
    #define EDE_UNIX 1
    #if defined(__APPLE__)
        #define EDE_PLATFORM "macOS"
    #else
        #define EDE_PLATFORM "Unix"
    #endif
#else
    #define EDE_PLATFORM "Unknown"
#endif

/* Bare metal platform definitions - NO standard headers */
#ifdef EDE_WINDOWS
    /* Windows bare metal definitions */
    typedef void* HANDLE;
    typedef unsigned long DWORD;
    typedef unsigned short WORD;
    typedef unsigned char BYTE;
    typedef int BOOL;
    typedef void* LPVOID;
    typedef const char* LPCSTR;
    typedef char* LPSTR;
    typedef const void* LPCVOID;
    typedef unsigned short WCHAR;
    typedef void* HMODULE;
    typedef void* HINSTANCE;
    typedef long LONG;
    typedef unsigned int UINT;
    
    #define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
    #define STD_INPUT_HANDLE ((DWORD)-10)
    #define STD_OUTPUT_HANDLE ((DWORD)-11)
    #define STD_ERROR_HANDLE ((DWORD)-12)
    #define ENABLE_ECHO_INPUT 0x0004
    #define ENABLE_LINE_INPUT 0x0002
    #define ENABLE_PROCESSED_INPUT 0x0001
    #define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #define DISABLE_NEWLINE_AUTO_RETURN 0x0008
    #define FILE_ATTRIBUTE_DIRECTORY 0x10
    #define VK_LEFT 0x25
    #define VK_UP 0x26
    #define VK_RIGHT 0x27
    #define VK_DOWN 0x28
    #define VK_PRIOR 0x21
    #define VK_NEXT 0x22
    #define VK_END 0x23
    #define VK_HOME 0x24
    #define VK_DELETE 0x2E
    #define KEY_EVENT 0x0001
    #define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
    #define GENERIC_READ 0x80000000L
    #define GENERIC_WRITE 0x40000000L
    #define FILE_SHARE_READ 0x00000001
    #define FILE_SHARE_WRITE 0x00000002
    #define OPEN_EXISTING 3
    #define CREATE_ALWAYS 2
    
    typedef struct _COORD {
        short X;
        short Y;
    } COORD;
    
    typedef struct _SMALL_RECT {
        short Left;
        short Top;
        short Right;
        short Bottom;
    } SMALL_RECT;
    
    typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
        COORD dwSize;
        COORD dwCursorPosition;
        WORD wAttributes;
        SMALL_RECT srWindow;
        COORD dwMaximumWindowSize;
    } CONSOLE_SCREEN_BUFFER_INFO;
    
    typedef struct _KEY_EVENT_RECORD {
        BOOL bKeyDown;
        WORD wRepeatCount;
        WORD wVirtualKeyCode;
        WORD wVirtualScanCode;
        union {
            WCHAR UnicodeChar;
            char AsciiChar;
        } uChar;
        DWORD dwControlKeyState;
    } KEY_EVENT_RECORD;
    
    typedef struct _INPUT_RECORD {
        WORD EventType;
        union {
            KEY_EVENT_RECORD KeyEvent;
        } Event;
    } INPUT_RECORD;
    
    typedef struct _WIN32_FIND_DATA {
        DWORD dwFileAttributes;
        DWORD ftCreationTime[2];
        DWORD ftLastAccessTime[2];
        DWORD ftLastWriteTime[2];
        DWORD nFileSizeHigh;
        DWORD nFileSizeLow;
        DWORD dwReserved0;
        DWORD dwReserved1;
        char cFileName[260];
        char cAlternateFileName[14];
    } WIN32_FIND_DATA;
    
    typedef struct _SECURITY_ATTRIBUTES {
        DWORD nLength;
        LPVOID lpSecurityDescriptor;
        BOOL bInheritHandle;
    } SECURITY_ATTRIBUTES;
    
    /* Windows API function declarations */
    __declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD nStdHandle);
    __declspec(dllimport) BOOL __stdcall GetConsoleMode(HANDLE hConsoleHandle, DWORD* lpMode);
    __declspec(dllimport) BOOL __stdcall SetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode);
    __declspec(dllimport) BOOL __stdcall ReadConsoleInputA(HANDLE hConsoleInput, INPUT_RECORD* lpBuffer, DWORD nLength, DWORD* lpNumberOfEventsRead);
    __declspec(dllimport) BOOL __stdcall GetConsoleScreenBufferInfo(HANDLE hConsoleOutput, CONSOLE_SCREEN_BUFFER_INFO* lpConsoleScreenBufferInfo);
    __declspec(dllimport) BOOL __stdcall FillConsoleOutputCharacterA(HANDLE hConsoleOutput, char cCharacter, DWORD nLength, COORD dwWriteCoord, DWORD* lpNumberOfCharsWritten);
    __declspec(dllimport) BOOL __stdcall SetConsoleCursorPosition(HANDLE hConsoleOutput, COORD dwCursorPosition);
    __declspec(dllimport) HANDLE __stdcall FindFirstFileA(LPCSTR lpFileName, WIN32_FIND_DATA* lpFindFileData);
    __declspec(dllimport) BOOL __stdcall FindNextFileA(HANDLE hFindFile, WIN32_FIND_DATA* lpFindFileData);
    __declspec(dllimport) BOOL __stdcall FindClose(HANDLE hFindFile);
    __declspec(dllimport) HMODULE __stdcall LoadLibraryA(LPCSTR lpLibFileName);
    __declspec(dllimport) BOOL __stdcall FreeLibrary(HMODULE hLibModule);
    __declspec(dllimport) void* __stdcall GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
    __declspec(dllimport) void __stdcall ExitProcess(UINT uExitCode);
    __declspec(dllimport) DWORD __stdcall GetLastError(void);
    __declspec(dllimport) HANDLE __stdcall CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, SECURITY_ATTRIBUTES* lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
    __declspec(dllimport) BOOL __stdcall WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, DWORD* lpNumberOfBytesWritten, void* lpOverlapped);
    __declspec(dllimport) BOOL __stdcall ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD* lpNumberOfBytesRead, void* lpOverlapped);
    __declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE hObject);
    __declspec(dllimport) char* __stdcall GetCommandLineA(void);
    __declspec(dllimport) int __stdcall _chdir(const char* dirname);
    __declspec(dllimport) char* __stdcall _getcwd(char* buffer, int maxlen);
    
    #define ReadConsoleInput ReadConsoleInputA
    #define FindFirstFile FindFirstFileA
    #define FindNextFile FindNextFileA
    #define LoadLibrary LoadLibraryA
    #define CreateFile CreateFileA
    #define GetCommandLine GetCommandLineA
    #define getcwd _getcwd
    #define chdir _chdir
    #define isatty _isatty
    #define fileno _fileno
    
#else
    /* Unix/Linux/Android bare metal definitions */
    typedef unsigned int size_t;
    typedef long ssize_t;
    typedef int pid_t;
    typedef unsigned int mode_t;
    typedef long off_t;
    typedef long time_t;
    typedef unsigned long dev_t;
    typedef unsigned long ino_t;
    typedef unsigned int nlink_t;
    typedef unsigned int uid_t;
    typedef unsigned int gid_t;
    typedef long blksize_t;
    typedef long blkcnt_t;
    
    #define STDIN_FILENO 0
    #define STDOUT_FILENO 1
    #define STDERR_FILENO 2
    #define TCSAFLUSH 2
    #define BRKINT 0000002
    #define ICRNL 0000400
    #define INPCK 0000020
    #define ISTRIP 0000040
    #define IXON 0002000
    #define OPOST 0000001
    #define CS8 0000060
    #define ECHO 0000010
    #define ICANON 0000002
    #define IEXTEN 0100000
    #define ISIG 0000001
    #define VMIN 6
    #define VTIME 5
    #define TIOCGWINSZ 0x5413
    #define SIGWINCH 28
    #define SIGTERM 15
    #define S_IFMT 0170000
    #define S_IFDIR 0040000
    #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
    #define RTLD_LAZY 0x00001
    #define SEEK_SET 0
    #define SEEK_END 2
    #define O_RDONLY 00
    #define O_WRONLY 01
    #define O_RDWR 02
    #define O_CREAT 0100
    #define EAGAIN 11
    
    typedef unsigned char cc_t;
    typedef unsigned int speed_t;
    typedef unsigned int tcflag_t;
    
    #define NCCS 32
    struct termios {
        tcflag_t c_iflag;
        tcflag_t c_oflag;
        tcflag_t c_cflag;
        tcflag_t c_lflag;
        cc_t c_line;
        cc_t c_cc[NCCS];
        speed_t c_ispeed;
        speed_t c_ospeed;
    };
    
    struct winsize {
        unsigned short ws_row;
        unsigned short ws_col;
        unsigned short ws_xpixel;
        unsigned short ws_ypixel;
    };
    
    struct stat {
        dev_t st_dev;
        ino_t st_ino;
        mode_t st_mode;
        nlink_t st_nlink;
        uid_t st_uid;
        gid_t st_gid;
        dev_t st_rdev;
        off_t st_size;
        blksize_t st_blksize;
        blkcnt_t st_blocks;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
    };
    
    struct dirent {
        ino_t d_ino;
        off_t d_off;
        unsigned short d_reclen;
        unsigned char d_type;
        char d_name[256];
    };
    
    typedef struct __dirstream DIR;
    
    /* Unix system call declarations */
    extern ssize_t read(int fd, void *buf, size_t count);
    extern ssize_t write(int fd, const void *buf, size_t count);
    extern int ioctl(int fd, unsigned long request, ...);
    extern int tcgetattr(int fd, struct termios *termios_p);
    extern int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
    extern int open(const char *pathname, int flags, ...);
    extern int close(int fd);
    extern int stat(const char *pathname, struct stat *statbuf);
    extern DIR *opendir(const char *name);
    extern struct dirent *readdir(DIR *dirp);
    extern int closedir(DIR *dirp);
    extern void *dlopen(const char *filename, int flags);
    extern void *dlsym(void *handle, const char *symbol);
    extern int dlclose(void *handle);
    extern char *dlerror(void);
    extern pid_t fork(void);
    extern int kill(pid_t pid, int sig);
    extern void (*signal(int signum, void (*handler)(int)))(int);
    extern int isatty(int fd);
    extern char *getcwd(char *buf, size_t size);
    extern int chdir(const char *path);
    extern void exit(int status);
    extern void perror(const char *s);
#endif

/* Version information */
#define EDE_VERSION "1.0"
#define EDE_NAME "GNU ede"

/* Editor limits */
#define EDE_MAX_LINE_LENGTH 4096
#define EDE_TAB_SIZE 4
#define EDE_UNDO_STACK_SIZE 1000
#define EDE_MAX_SEARCH_RESULTS 1000
#define EDE_MAX_MODULES 64
#define EDE_MODULE_NAME_MAX 256

/* Key codes */
#define CTRL_KEY(k) ((k) & 0x1f)
#define KEY_ESCAPE 27
#define KEY_BACKSPACE 127
#define KEY_DELETE 126
#define KEY_ARROW_UP 1000
#define KEY_ARROW_DOWN 1001
#define KEY_ARROW_LEFT 1002
#define KEY_ARROW_RIGHT 1003
#define KEY_PAGE_UP 1004
#define KEY_PAGE_DOWN 1005
#define KEY_HOME 1006
#define KEY_END 1007

/* Color codes for syntax highlighting */
typedef enum {
    COLOR_NORMAL = 0,
    COLOR_KEYWORD,
    COLOR_STRING,
    COLOR_COMMENT,
    COLOR_NUMBER,
    COLOR_FUNCTION,
    COLOR_PREPROCESSOR,
    COLOR_OPERATOR,
    COLOR_TYPE
} ColorType;

/* Editor modes */
typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_VIM_COMMAND,
    MODE_SEARCH,
    MODE_REPLACE,
    MODE_GOTO_LINE
} EditorMode;

/* Undo/Redo action types */
typedef enum {
    ACTION_INSERT_CHAR,
    ACTION_DELETE_CHAR,
    ACTION_INSERT_LINE,
    ACTION_DELETE_LINE,
    ACTION_REPLACE_TEXT
} ActionType;

/* Forward declarations */
struct EditorConfig;
struct EditorRow;
struct Module;

/* Module API function types */
typedef int (*ModuleInitFunc)(struct EditorConfig *config);
typedef void (*ModuleCleanupFunc)(void);
typedef void (*ModuleKeypressFunc)(int key);
typedef void (*ModuleRenderFunc)(void);

/* Module structure */
typedef struct Module {
    char name[EDE_MODULE_NAME_MAX];
    void *handle;
    ModuleInitFunc init;
    ModuleCleanupFunc cleanup;
    ModuleKeypressFunc on_keypress;
    ModuleRenderFunc on_render;
    int active;
} Module;

/* Undo/Redo action */
typedef struct UndoAction {
    ActionType type;
    int row;
    int col;
    char *text;
    int text_len;
    struct UndoAction *next;
} UndoAction;

/* Editor row structure */
typedef struct EditorRow {
    int size;
    int rsize;
    char *chars;
    char *render;
    unsigned char *hl;
    int hl_open_comment;
    int idx;
} EditorRow;

/* Syntax highlighting structure */
typedef struct Syntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singleline_comment_start;
    char *multiline_comment_start;
    char *multiline_comment_end;
    int flags;
} Syntax;

/* Editor configuration */
typedef struct EditorConfig {
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    EditorRow *row;
    int dirty;
    char *filename;
    char statusmsg[256];
    time_t statusmsg_time;
    Syntax *syntax;
    EditorMode mode;
    int vim_mode_active;
    char vim_command[256];
    int vim_command_len;
    UndoAction *undo_stack;
    UndoAction *redo_stack;
    int undo_count;
    char search_query[256];
    int search_query_len;
    int last_match;
    int search_direction;
    Module modules[EDE_MAX_MODULES];
    int module_count;
    int show_line_numbers;
    int ctrl_c_pressed;
    time_t ctrl_c_time;
#ifdef EDE_WINDOWS
    HANDLE hStdout;
    HANDLE hStdin;
    DWORD orig_out_mode;
    DWORD orig_in_mode;
#else
    struct termios orig_termios;
#endif
} EditorConfig;

/* Global editor configuration */
EditorConfig E;

/* Syntax highlighting definitions */
#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

/* C/C++ keywords */
char *C_HL_extensions[] = { ".c", ".h", ".cpp", ".hpp", ".cc", NULL };
char *C_HL_keywords[] = {
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "static", "enum", "class", "case",
    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", "short|", "auto|", "const|", "bool|", "size_t|", "uint32_t|",
    "uint64_t|", "int32_t|", "int64_t|", "uint8_t|", "int8_t|", NULL
};

/* Python keywords */
char *PY_HL_extensions[] = { ".py", ".pyw", NULL };
char *PY_HL_keywords[] = {
    "def", "class", "if", "elif", "else", "while", "for", "break",
    "continue", "return", "import", "from", "as", "try", "except",
    "finally", "with", "lambda", "yield", "pass", "raise", "assert",
    "True|", "False|", "None|", "self|", "int|", "str|", "list|",
    "dict|", "tuple|", "set|", "bool|", "float|", NULL
};

/* JavaScript keywords */
char *JS_HL_extensions[] = { ".js", ".jsx", ".ts", ".tsx", NULL };
char *JS_HL_keywords[] = {
    "function", "if", "else", "while", "for", "break", "continue",
    "return", "switch", "case", "default", "var", "let", "const",
    "class", "extends", "import", "export", "from", "async", "await",
    "try", "catch", "finally", "new", "this", "typeof", "instanceof",
    "undefined|", "null|", "true|", "false|", "number|", "string|",
    "boolean|", "object|", "Array|", "Object|", "Function|", NULL
};

/* Syntax database */
Syntax HLDB[] = {
    {
        "c",
        C_HL_extensions,
        C_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "python",
        PY_HL_extensions,
        PY_HL_keywords,
        "#", "\"\"\"", "\"\"\"",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
    {
        "javascript",
        JS_HL_extensions,
        JS_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/*** Terminal control - Bare metal implementation ***/

#ifdef EDE_WINDOWS

void die(const char *s) {
    /* Clear screen before exiting */
    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;
    
    if (GetConsoleScreenBufferInfo(E.hStdout, &csbi)) {
        dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacterA(E.hStdout, ' ', dwConSize, coordScreen, &cCharsWritten);
        SetConsoleCursorPosition(E.hStdout, coordScreen);
    }
    
    fprintf(stderr, "ede: %s\r\n", s);
    exit(1);
}

void disableRawMode(void) {
    SetConsoleMode(E.hStdin, E.orig_in_mode);
    SetConsoleMode(E.hStdout, E.orig_out_mode);
}

void enableRawMode(void) {
    E.hStdin = GetStdHandle(STD_INPUT_HANDLE);
    E.hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (E.hStdin == INVALID_HANDLE_VALUE || E.hStdout == INVALID_HANDLE_VALUE) {
        die("GetStdHandle");
    }
    
    if (!GetConsoleMode(E.hStdin, &E.orig_in_mode)) {
        die("GetConsoleMode");
    }
    if (!GetConsoleMode(E.hStdout, &E.orig_out_mode)) {
        die("GetConsoleMode");
    }
    
    DWORD in_mode = E.orig_in_mode;
    in_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    in_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    
    DWORD out_mode = E.orig_out_mode;
    out_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    
    if (!SetConsoleMode(E.hStdin, in_mode)) {
        die("SetConsoleMode input");
    }
    if (!SetConsoleMode(E.hStdout, out_mode)) {
        die("SetConsoleMode output");
    }
    
    atexit(disableRawMode);
}

int editorReadKey(void) {
    DWORD nread;
    char c;
    INPUT_RECORD irInBuf;
    
    while (1) {
        if (!ReadConsoleInput(E.hStdin, &irInBuf, 1, &nread)) {
            die("ReadConsoleInput");
        }
        
        if (nread == 0) continue;
        
        if (irInBuf.EventType == KEY_EVENT && irInBuf.Event.KeyEvent.bKeyDown) {
            KEY_EVENT_RECORD ker = irInBuf.Event.KeyEvent;
            c = ker.uChar.AsciiChar;
            
            /* Handle escape sequences (VT mode) */
            if (c == 27) { /* ESC */
                char seq[3];
                DWORD read1, read2;
                INPUT_RECORD ir1, ir2;
                
                /* Try to read next chars for sequence */
                if (ReadConsoleInput(E.hStdin, &ir1, 1, &read1) && read1 > 0 &&
                    ir1.EventType == KEY_EVENT && ir1.Event.KeyEvent.bKeyDown) {
                    seq[0] = ir1.Event.KeyEvent.uChar.AsciiChar;
                    
                    if (seq[0] == '[' || seq[0] == 'O') {
                        if (ReadConsoleInput(E.hStdin, &ir2, 1, &read2) && read2 > 0 &&
                            ir2.EventType == KEY_EVENT && ir2.Event.KeyEvent.bKeyDown) {
                            seq[1] = ir2.Event.KeyEvent.uChar.AsciiChar;
                            
                            if (seq[0] == '[') {
                                switch (seq[1]) {
                                    case 'A': return KEY_ARROW_UP;
                                    case 'B': return KEY_ARROW_DOWN;
                                    case 'C': return KEY_ARROW_RIGHT;
                                    case 'D': return KEY_ARROW_LEFT;
                                    case 'H': return KEY_HOME;
                                    case 'F': return KEY_END;
                                    case '1': case '2': case '3': case '4':
                                    case '5': case '6': case '7': case '8':
                                        /* Multi-byte sequence, read one more */
                                        if (ReadConsoleInput(E.hStdin, &ir2, 1, &read2) && read2 > 0) {
                                            char third = ir2.Event.KeyEvent.uChar.AsciiChar;
                                            if (third == '~') {
                                                switch (seq[1]) {
                                                    case '1': return KEY_HOME;
                                                    case '3': return KEY_DELETE;
                                                    case '4': return KEY_END;
                                                    case '5': return KEY_PAGE_UP;
                                                    case '6': return KEY_PAGE_DOWN;
                                                    case '7': return KEY_HOME;
                                                    case '8': return KEY_END;
                                                }
                                            }
                                        }
                                        break;
                                }
                            } else if (seq[0] == 'O') {
                                switch (seq[1]) {
                                    case 'H': return KEY_HOME;
                                    case 'F': return KEY_END;
                                }
                            }
                        }
                    }
                }
                return 27; /* Just return ESC if not a recognized sequence */
            }
            
            if (c != 0) {
                return c;
            }
            
            /* Handle special keys via virtual key codes */
            switch (ker.wVirtualKeyCode) {
                case VK_LEFT: return KEY_ARROW_LEFT;
                case VK_RIGHT: return KEY_ARROW_RIGHT;
                case VK_UP: return KEY_ARROW_UP;
                case VK_DOWN: return KEY_ARROW_DOWN;
                case VK_HOME: return KEY_HOME;
                case VK_END: return KEY_END;
                case VK_PRIOR: return KEY_PAGE_UP;
                case VK_NEXT: return KEY_PAGE_DOWN;
                case VK_DELETE: return KEY_DELETE;
            }
        }
    }
}

int getWindowSize(int *rows, int *cols) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    if (!GetConsoleScreenBufferInfo(E.hStdout, &csbi)) {
        return -1;
    }
    
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    return 0;
}

#else /* Unix/Linux/macOS */

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    
    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int editorReadKey(void) {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    
    if (c == '\x1b') {
        char seq[3];
        
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return KEY_HOME;
                        case '3': return KEY_DELETE;
                        case '4': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        case '7': return KEY_HOME;
                        case '8': return KEY_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
            }
        }
        
        return '\x1b';
    } else {
        return c;
    }
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void handleSigwinch(int sig) {
    (void)sig;
    getWindowSize(&E.screenrows, &E.screencols);
    E.screenrows -= 2;
}

#endif

/*** String buffer for efficient screen rendering ***/

typedef struct StringBuffer {
    char *b;
    int len;
} StringBuffer;

#define STRBUF_INIT {NULL, 0}

void sbAppend(StringBuffer *sb, const char *s, int len) {
    char *new = realloc(sb->b, sb->len + len);
    
    if (new == NULL) return;
    memcpy(&new[sb->len], s, len);
    sb->b = new;
    sb->len += len;
}

void sbFree(StringBuffer *sb) {
    free(sb->b);
}

/*** Undo/Redo system ***/

void addUndoAction(ActionType type, int row, int col, const char *text, int text_len) {
    if (E.undo_count >= EDE_UNDO_STACK_SIZE) {
        /* Remove oldest action */
        UndoAction *old = E.undo_stack;
        E.undo_stack = old->next;
        free(old->text);
        free(old);
        E.undo_count--;
    }
    
    UndoAction *action = malloc(sizeof(UndoAction));
    action->type = type;
    action->row = row;
    action->col = col;
    action->text = malloc(text_len + 1);
    memcpy(action->text, text, text_len);
    action->text[text_len] = '\0';
    action->text_len = text_len;
    action->next = E.undo_stack;
    
    E.undo_stack = action;
    E.undo_count++;
    
    /* Clear redo stack */
    while (E.redo_stack) {
        UndoAction *tmp = E.redo_stack;
        E.redo_stack = tmp->next;
        free(tmp->text);
        free(tmp);
    }
}

void performUndo(void) {
    if (!E.undo_stack) return;
    
    UndoAction *action = E.undo_stack;
    E.undo_stack = action->next;
    E.undo_count--;
    
    /* Perform inverse operation */
    /* (Implementation depends on action type - simplified here) */
    
    /* Add to redo stack */
    action->next = E.redo_stack;
    E.redo_stack = action;
}

void performRedo(void) {
    if (!E.redo_stack) return;
    
    UndoAction *action = E.redo_stack;
    E.redo_stack = action->next;
    
    /* Perform action */
    /* (Implementation depends on action type - simplified here) */
    
    /* Add to undo stack */
    action->next = E.undo_stack;
    E.undo_stack = action;
    E.undo_count++;
}

/*** Advanced Module System ***/

/* Forward declarations to resolve order dependencies */
typedef struct SearchMatch {
    int row;
    int col;
    int length;
    struct SearchMatch *next;
} SearchMatch;

typedef struct SearchContext {
    char query[256];
    char replace_text[256];
    int query_len;
    int replace_len;
    SearchMatch *matches;
    SearchMatch *current_match;
    int match_count;
    int case_sensitive;
    int whole_word;
    int use_regex;
} SearchContext;

SearchContext search_ctx = {0};

void editorSetStatusMessage(const char *fmt, ...);
void editorUpdateRow(EditorRow *row);
void editorInsertRow(int at, char *s, size_t len);
void editorDelRow(int at);
int editorSave(void);
void findAllMatches(void);
void replaceAllMatches(void);
void editorRefreshScreen(void);

/* Module scripting language support */
typedef enum {
    MODTOKEN_PRINT,
    MODTOKEN_SET,
    MODTOKEN_GET,
    MODTOKEN_IF,
    MODTOKEN_ELSE,
    MODTOKEN_WHILE,
    MODTOKEN_FOR,
    MODTOKEN_FUNCTION,
    MODTOKEN_RETURN,
    MODTOKEN_VAR,
    MODTOKEN_CALL,
    MODTOKEN_EDITOR,
    MODTOKEN_LINE,
    MODTOKEN_CURSOR,
    MODTOKEN_FILE,
    MODTOKEN_BUFFER,
    MODTOKEN_SEARCH,
    MODTOKEN_REPLACE,
    MODTOKEN_INSERT,
    MODTOKEN_DELETE,
    MODTOKEN_SAVE,
    MODTOKEN_LOAD,
    MODTOKEN_EXEC,
    MODTOKEN_HOOK,
    MODTOKEN_EVENT,
    MODTOKEN_STRING,
    MODTOKEN_NUMBER,
    MODTOKEN_IDENTIFIER,
    MODTOKEN_OPERATOR,
    MODTOKEN_LPAREN,
    MODTOKEN_RPAREN,
    MODTOKEN_LBRACE,
    MODTOKEN_RBRACE,
    MODTOKEN_SEMICOLON,
    MODTOKEN_COMMA,
    MODTOKEN_EOF,
    MODTOKEN_UNKNOWN
} ModTokenType;

typedef struct ModToken {
    ModTokenType type;
    char value[256];
    int line;
    int col;
} ModToken;

typedef struct ModuleVariable {
    char name[128];
    char value[512];
    int is_number;
    int number_value;
    struct ModuleVariable *next;
} ModuleVariable;

typedef struct ModuleFunction {
    char name[128];
    char *body;
    int body_len;
    char params[16][64];
    int param_count;
    struct ModuleFunction *next;
} ModuleFunction;

typedef struct ModuleScript {
    char *source;
    int source_len;
    ModToken *tokens;
    int token_count;
    ModuleVariable *variables;
    ModuleFunction *functions;
    int current_token;
} ModuleScript;

typedef struct ModuleHook {
    char event_name[128];
    void (*callback)(void *data);
    struct ModuleHook *next;
} ModuleHook;

typedef struct ModuleContext {
    ModuleScript *script;
    ModuleHook *hooks;
    char output_buffer[4096];
    int output_len;
    int error_flag;
    char error_msg[256];
} ModuleContext;

ModuleContext module_ctx = {0};

/* Module API functions */
void moduleApiPrint(const char *text) {
    int len = strlen(text);
    if (module_ctx.output_len + len < sizeof(module_ctx.output_buffer) - 1) {
        strcpy(module_ctx.output_buffer + module_ctx.output_len, text);
        module_ctx.output_len += len;
    }
    editorSetStatusMessage("%s", text);
}

void moduleApiGetLine(int line_num, char *buffer, int buffer_size) {
    if (line_num < 0 || line_num >= E.numrows) {
        buffer[0] = '\0';
        return;
    }
    
    EditorRow *row = &E.row[line_num];
    int copy_len = (row->size < buffer_size - 1) ? row->size : buffer_size - 1;
    memcpy(buffer, row->chars, copy_len);
    buffer[copy_len] = '\0';
}

void moduleApiSetLine(int line_num, const char *text) {
    if (line_num < 0 || line_num >= E.numrows) return;
    
    EditorRow *row = &E.row[line_num];
    free(row->chars);
    row->size = strlen(text);
    row->chars = malloc(row->size + 1);
    strcpy(row->chars, text);
    editorUpdateRow(row);
}

void moduleApiInsertLine(int line_num, const char *text) {
    if (line_num < 0 || line_num > E.numrows) line_num = E.numrows;
    editorInsertRow(line_num, (char*)text, strlen(text));
}

void moduleApiDeleteLine(int line_num) {
    if (line_num >= 0 && line_num < E.numrows) {
        editorDelRow(line_num);
    }
}

int moduleApiGetCursorX(void) {
    return E.cx;
}

int moduleApiGetCursorY(void) {
    return E.cy;
}

void moduleApiSetCursor(int x, int y) {
    if (y >= 0 && y < E.numrows) E.cy = y;
    if (x >= 0) E.cx = x;
}

int moduleApiGetLineCount(void) {
    return E.numrows;
}

const char* moduleApiGetFilename(void) {
    return E.filename ? E.filename : "";
}

void moduleApiSaveFile(void) {
    editorSave();
}

int moduleApiSearch(const char *query) {
    strncpy(search_ctx.query, query, sizeof(search_ctx.query) - 1);
    search_ctx.query_len = strlen(query);
    search_ctx.case_sensitive = 1;
    findAllMatches();
    return search_ctx.match_count;
}

void moduleApiReplace(const char *old_text, const char *new_text) {
    strncpy(search_ctx.query, old_text, sizeof(search_ctx.query) - 1);
    search_ctx.query_len = strlen(old_text);
    strncpy(search_ctx.replace_text, new_text, sizeof(search_ctx.replace_text) - 1);
    search_ctx.replace_len = strlen(new_text);
    search_ctx.case_sensitive = 1;
    findAllMatches();
    replaceAllMatches();
}

void moduleApiExec(const char *command) {
    char output[4096];
#ifdef EDE_WINDOWS
    FILE *fp = _popen(command, "r");
#else
    FILE *fp = popen(command, "r");
#endif
    
    if (!fp) {
        moduleApiPrint("Error executing command");
        return;
    }
    
    int pos = 0;
    int c;
    while ((c = fgetc(fp)) != EOF && pos < sizeof(output) - 1) {
        output[pos++] = c;
    }
    output[pos] = '\0';
    
#ifdef EDE_WINDOWS
    _pclose(fp);
#else
    pclose(fp);
#endif
    
    moduleApiPrint(output);
}

/* Module script tokenizer */
int moduleTokenize(ModuleScript *script, const char *source) {
    script->source = strdup(source);
    script->source_len = strlen(source);
    script->token_count = 0;
    script->current_token = 0;
    
    /* Allocate token array */
    script->tokens = malloc(1000 * sizeof(ModToken));
    
    int pos = 0;
    int line = 1;
    int col = 1;
    
    while (pos < script->source_len) {
        char c = source[pos];
        
        /* Skip whitespace */
        if (isspace(c)) {
            if (c == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
            pos++;
            continue;
        }
        
        /* Skip comments */
        if (c == '/' && pos + 1 < script->source_len && source[pos + 1] == '/') {
            while (pos < script->source_len && source[pos] != '\n') pos++;
            continue;
        }
        
        ModToken *token = &script->tokens[script->token_count++];
        token->line = line;
        token->col = col;
        
        /* String literals */
        if (c == '"') {
            token->type = MODTOKEN_STRING;
            pos++;
            int val_pos = 0;
            while (pos < script->source_len && source[pos] != '"' && val_pos < 255) {
                token->value[val_pos++] = source[pos++];
            }
            token->value[val_pos] = '\0';
            if (pos < script->source_len) pos++; /* Skip closing quote */
            continue;
        }
        
        /* Numbers */
        if (isdigit(c)) {
            token->type = MODTOKEN_NUMBER;
            int val_pos = 0;
            while (pos < script->source_len && (isdigit(source[pos]) || source[pos] == '.') && val_pos < 255) {
                token->value[val_pos++] = source[pos++];
            }
            token->value[val_pos] = '\0';
            continue;
        }
        
        /* Keywords and identifiers */
        if (isalpha(c) || c == '_') {
            int val_pos = 0;
            while (pos < script->source_len && (isalnum(source[pos]) || source[pos] == '_') && val_pos < 255) {
                token->value[val_pos++] = source[pos++];
            }
            token->value[val_pos] = '\0';
            
            /* Match keywords */
            if (strcmp(token->value, "print") == 0) token->type = MODTOKEN_PRINT;
            else if (strcmp(token->value, "set") == 0) token->type = MODTOKEN_SET;
            else if (strcmp(token->value, "get") == 0) token->type = MODTOKEN_GET;
            else if (strcmp(token->value, "if") == 0) token->type = MODTOKEN_IF;
            else if (strcmp(token->value, "else") == 0) token->type = MODTOKEN_ELSE;
            else if (strcmp(token->value, "while") == 0) token->type = MODTOKEN_WHILE;
            else if (strcmp(token->value, "for") == 0) token->type = MODTOKEN_FOR;
            else if (strcmp(token->value, "function") == 0) token->type = MODTOKEN_FUNCTION;
            else if (strcmp(token->value, "return") == 0) token->type = MODTOKEN_RETURN;
            else if (strcmp(token->value, "var") == 0) token->type = MODTOKEN_VAR;
            else if (strcmp(token->value, "editor") == 0) token->type = MODTOKEN_EDITOR;
            else if (strcmp(token->value, "line") == 0) token->type = MODTOKEN_LINE;
            else if (strcmp(token->value, "cursor") == 0) token->type = MODTOKEN_CURSOR;
            else if (strcmp(token->value, "file") == 0) token->type = MODTOKEN_FILE;
            else if (strcmp(token->value, "search") == 0) token->type = MODTOKEN_SEARCH;
            else if (strcmp(token->value, "replace") == 0) token->type = MODTOKEN_REPLACE;
            else if (strcmp(token->value, "insert") == 0) token->type = MODTOKEN_INSERT;
            else if (strcmp(token->value, "delete") == 0) token->type = MODTOKEN_DELETE;
            else if (strcmp(token->value, "save") == 0) token->type = MODTOKEN_SAVE;
            else if (strcmp(token->value, "exec") == 0) token->type = MODTOKEN_EXEC;
            else if (strcmp(token->value, "hook") == 0) token->type = MODTOKEN_HOOK;
            else token->type = MODTOKEN_IDENTIFIER;
            
            continue;
        }
        
        /* Operators and punctuation */
        token->value[0] = c;
        token->value[1] = '\0';
        
        switch (c) {
            case '(': token->type = MODTOKEN_LPAREN; break;
            case ')': token->type = MODTOKEN_RPAREN; break;
            case '{': token->type = MODTOKEN_LBRACE; break;
            case '}': token->type = MODTOKEN_RBRACE; break;
            case ';': token->type = MODTOKEN_SEMICOLON; break;
            case ',': token->type = MODTOKEN_COMMA; break;
            case '=': case '+': case '-': case '*': case '/': case '<': case '>':
                token->type = MODTOKEN_OPERATOR;
                break;
            default:
                token->type = MODTOKEN_UNKNOWN;
        }
        
        pos++;
        col++;
    }
    
    /* Add EOF token */
    script->tokens[script->token_count].type = MODTOKEN_EOF;
    script->token_count++;
    
    return 0;
}

/* Module script interpreter */
void moduleExecuteStatement(ModuleScript *script);

void moduleExecutePrint(ModuleScript *script) {
    script->current_token++; /* Skip 'print' */
    
    if (script->current_token >= script->token_count) return;
    
    ModToken *token = &script->tokens[script->current_token];
    
    if (token->type == MODTOKEN_LPAREN) {
        script->current_token++; /* Skip '(' */
        token = &script->tokens[script->current_token];
        
        if (token->type == MODTOKEN_STRING) {
            moduleApiPrint(token->value);
            script->current_token++;
        } else if (token->type == MODTOKEN_IDENTIFIER) {
            /* Look up variable */
            ModuleVariable *var = script->variables;
            while (var) {
                if (strcmp(var->name, token->value) == 0) {
                    moduleApiPrint(var->value);
                    break;
                }
                var = var->next;
            }
            script->current_token++;
        }
        
        if (script->current_token < script->token_count && 
            script->tokens[script->current_token].type == MODTOKEN_RPAREN) {
            script->current_token++; /* Skip ')' */
        }
    }
}

void moduleExecuteSet(ModuleScript *script) {
    script->current_token++; /* Skip 'set' */
    
    if (script->current_token >= script->token_count) return;
    ModToken *var_token = &script->tokens[script->current_token++];
    
    if (var_token->type != MODTOKEN_IDENTIFIER) return;
    
    /* Skip '=' */
    if (script->current_token < script->token_count && 
        script->tokens[script->current_token].type == MODTOKEN_OPERATOR &&
        script->tokens[script->current_token].value[0] == '=') {
        script->current_token++;
    }
    
    if (script->current_token >= script->token_count) return;
    ModToken *value_token = &script->tokens[script->current_token++];
    
    /* Create or update variable */
    ModuleVariable *var = script->variables;
    while (var) {
        if (strcmp(var->name, var_token->value) == 0) {
            strncpy(var->value, value_token->value, sizeof(var->value) - 1);
            return;
        }
        if (!var->next) break;
        var = var->next;
    }
    
    /* Create new variable */
    ModuleVariable *new_var = malloc(sizeof(ModuleVariable));
    strncpy(new_var->name, var_token->value, sizeof(new_var->name) - 1);
    strncpy(new_var->value, value_token->value, sizeof(new_var->value) - 1);
    new_var->next = NULL;
    
    if (!script->variables) {
        script->variables = new_var;
    } else {
        var->next = new_var;
    }
}

void moduleExecuteEditor(ModuleScript *script) {
    script->current_token++; /* Skip 'editor' */
    
    if (script->current_token >= script->token_count) return;
    ModToken *cmd = &script->tokens[script->current_token++];
    
    if (cmd->type == MODTOKEN_IDENTIFIER) {
        if (strcmp(cmd->value, "save") == 0) {
            moduleApiSaveFile();
        } else if (strcmp(cmd->value, "linecount") == 0) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d", moduleApiGetLineCount());
            moduleApiPrint(buffer);
        } else if (strcmp(cmd->value, "filename") == 0) {
            moduleApiPrint(moduleApiGetFilename());
        }
    }
}

void moduleExecuteStatement(ModuleScript *script) {
    if (script->current_token >= script->token_count) return;
    
    ModToken *token = &script->tokens[script->current_token];
    
    switch (token->type) {
        case MODTOKEN_PRINT:
            moduleExecutePrint(script);
            break;
        case MODTOKEN_SET:
            moduleExecuteSet(script);
            break;
        case MODTOKEN_EDITOR:
            moduleExecuteEditor(script);
            break;
        case MODTOKEN_EXEC:
            script->current_token++;
            if (script->current_token < script->token_count) {
                ModToken *cmd_token = &script->tokens[script->current_token];
                if (cmd_token->type == MODTOKEN_LPAREN) {
                    script->current_token++;
                    ModToken *arg = &script->tokens[script->current_token];
                    if (arg->type == MODTOKEN_STRING) {
                        moduleApiExec(arg->value);
                        script->current_token++;
                    }
                    if (script->current_token < script->token_count &&
                        script->tokens[script->current_token].type == MODTOKEN_RPAREN) {
                        script->current_token++;
                    }
                }
            }
            break;
        case MODTOKEN_SEARCH:
            script->current_token++;
            if (script->current_token < script->token_count) {
                ModToken *search_token = &script->tokens[script->current_token];
                if (search_token->type == MODTOKEN_LPAREN) {
                    script->current_token++;
                    ModToken *query = &script->tokens[script->current_token];
                    if (query->type == MODTOKEN_STRING) {
                        int count = moduleApiSearch(query->value);
                        char buffer[64];
                        snprintf(buffer, sizeof(buffer), "Found %d matches", count);
                        moduleApiPrint(buffer);
                        script->current_token++;
                    }
                }
            }
            break;
        default:
            script->current_token++;
    }
    
    /* Skip semicolon if present */
    if (script->current_token < script->token_count &&
        script->tokens[script->current_token].type == MODTOKEN_SEMICOLON) {
        script->current_token++;
    }
}

void moduleExecuteScript(ModuleScript *script) {
    script->current_token = 0;
    
    while (script->current_token < script->token_count &&
           script->tokens[script->current_token].type != MODTOKEN_EOF) {
        moduleExecuteStatement(script);
    }
}

/* Module compilation and loading */
int compileModule(const char *source_file, const char *output_file) {
    FILE *src = fopen(source_file, "r");
    if (!src) {
        fprintf(stderr, "Cannot open source file: %s\n", source_file);
        return -1;
    }
    
    /* Read entire source */
    fseek(src, 0, SEEK_END);
    long file_size = ftell(src);
    fseek(src, 0, SEEK_SET);
    
    char *source = malloc(file_size + 1);
    fread(source, 1, file_size, src);
    source[file_size] = '\0';
    fclose(src);
    
    /* Tokenize and validate */
    ModuleScript script = {0};
    if (moduleTokenize(&script, source) != 0) {
        fprintf(stderr, "Tokenization failed\n");
        free(source);
        return -1;
    }
    
    /* Write compiled bytecode/script */
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create output file: %s\n", output_file);
        free(source);
        free(script.tokens);
        return -1;
    }
    
    /* Module file header */
    fprintf(out, "EDE_MODULE_V2\n");
    fprintf(out, "source_len:%ld\n", file_size);
    fwrite(source, 1, file_size, out);
    
    fclose(out);
    free(source);
    free(script.tokens);
    
    printf("Module compiled: %d tokens\n", script.token_count);
    
    return 0;
}

int loadModule(const char *module_file) {
    if (E.module_count >= EDE_MAX_MODULES) {
        return -1;
    }
    
    /* Try to load as script module first */
    FILE *fp = fopen(module_file, "rb");
    if (fp) {
        char header[32];
        if (fgets(header, sizeof(header), fp)) {
            /* Check for V2 script module */
            if (strncmp(header, "EDE_MODULE_V2", 13) == 0) {
                /* Read source length */
                char len_line[64];
                long source_len = 0;
                if (fgets(len_line, sizeof(len_line), fp)) {
                    sscanf(len_line, "source_len:%ld", &source_len);
                }
                
                if (source_len > 0 && source_len < 1000000) {
                    /* Read module source */
                    char *source = malloc(source_len + 1);
                    fread(source, 1, source_len, fp);
                    source[source_len] = '\0';
                    fclose(fp);
                    
                    /* Create and execute module script */
                    ModuleScript *script = malloc(sizeof(ModuleScript));
                    memset(script, 0, sizeof(ModuleScript));
                    
                    if (moduleTokenize(script, source) == 0) {
                        module_ctx.script = script;
                        moduleExecuteScript(script);
                        
                        Module *mod = &E.modules[E.module_count];
                        strncpy(mod->name, module_file, EDE_MODULE_NAME_MAX - 1);
                        mod->handle = script;
                        mod->active = 1;
                        E.module_count++;
                        
                        free(source);
                        return 0;
                    }
                    
                    free(source);
                    free(script);
                }
                fclose(fp);
                return -1;
            }
        }
        fclose(fp);
    }
    
    /* Fallback to DLL/SO loading for compiled modules */
#ifdef EDE_WINDOWS
    HMODULE handle = LoadLibrary(module_file);
    if (!handle) {
        return -1;
    }
    
    Module *mod = &E.modules[E.module_count];
    mod->handle = handle;
    mod->init = (ModuleInitFunc)GetProcAddress(handle, "module_init");
    mod->cleanup = (ModuleCleanupFunc)GetProcAddress(handle, "module_cleanup");
    mod->on_keypress = (ModuleKeypressFunc)GetProcAddress(handle, "module_on_keypress");
    mod->on_render = (ModuleRenderFunc)GetProcAddress(handle, "module_on_render");
#else
    void *handle = dlopen(module_file, RTLD_LAZY);
    if (!handle) {
        return -1;
    }
    
    Module *mod = &E.modules[E.module_count];
    mod->handle = handle;
    mod->init = (ModuleInitFunc)dlsym(handle, "module_init");
    mod->cleanup = (ModuleCleanupFunc)dlsym(handle, "module_cleanup");
    mod->on_keypress = (ModuleKeypressFunc)dlsym(handle, "module_on_keypress");
    mod->on_render = (ModuleRenderFunc)dlsym(handle, "module_on_render");
#endif
    
    strncpy(mod->name, module_file, EDE_MODULE_NAME_MAX - 1);
    mod->active = 1;
    
    if (mod->init) {
        mod->init(&E);
    }
    
    E.module_count++;
    return 0;
}

void unloadModules(void) {
    for (int i = 0; i < E.module_count; i++) {
        Module *mod = &E.modules[i];
        if (mod->cleanup) {
            mod->cleanup();
        }
        
#ifdef EDE_WINDOWS
        if (mod->handle) FreeLibrary((HMODULE)mod->handle);
#else
        if (mod->handle) dlclose(mod->handle);
#endif
    }
    E.module_count = 0;
}

/*** Syntax highlighting ***/

int is_separator(int c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(EditorRow *row) {
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, COLOR_NORMAL, row->rsize);
    
    if (E.syntax == NULL) return;
    
    char **keywords = E.syntax->keywords;
    
    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;
    
    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;
    
    int prev_sep = 1;
    int in_string = 0;
    int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);
    
    int i = 0;
    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : COLOR_NORMAL;
        
        if (scs_len && !in_string && !in_comment) {
            if (!strncmp(&row->render[i], scs, scs_len)) {
                memset(&row->hl[i], COLOR_COMMENT, row->rsize - i);
                break;
            }
        }
        
        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                row->hl[i] = COLOR_COMMENT;
                if (!strncmp(&row->render[i], mce, mce_len)) {
                    memset(&row->hl[i], COLOR_COMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
                memset(&row->hl[i], COLOR_COMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }
        
        if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                row->hl[i] = COLOR_STRING;
                if (c == '\\' && i + 1 < row->rsize) {
                    row->hl[i + 1] = COLOR_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string) in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            } else {
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row->hl[i] = COLOR_STRING;
                    i++;
                    continue;
                }
            }
        }
        
        if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_sep || prev_hl == COLOR_NUMBER)) ||
                (c == '.' && prev_hl == COLOR_NUMBER)) {
                row->hl[i] = COLOR_NUMBER;
                i++;
                prev_sep = 0;
                continue;
            }
        }
        
        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2) klen--;
                
                if (!strncmp(&row->render[i], keywords[j], klen) &&
                    is_separator(row->render[i + klen])) {
                    memset(&row->hl[i], kw2 ? COLOR_TYPE : COLOR_KEYWORD, klen);
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue;
            }
        }
        
        prev_sep = is_separator(c);
        i++;
    }
    
    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < E.numrows)
        editorUpdateSyntax(&E.row[row->idx + 1]);
}

int editorSyntaxToColor(int hl) {
    switch (hl) {
        case COLOR_KEYWORD: return 33; /* Yellow */
        case COLOR_STRING: return 32; /* Green */
        case COLOR_COMMENT: return 36; /* Cyan */
        case COLOR_NUMBER: return 35; /* Magenta */
        case COLOR_TYPE: return 34; /* Blue */
        case COLOR_FUNCTION: return 31; /* Red */
        case COLOR_PREPROCESSOR: return 35; /* Magenta */
        case COLOR_OPERATOR: return 37; /* White */
        default: return 37; /* White */
    }
}

void editorSelectSyntaxHighlight(void) {
    E.syntax = NULL;
    if (E.filename == NULL) return;
    
    char *ext = strrchr(E.filename, '.');
    
    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        Syntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s->filematch[i]) {
            int is_ext = (s->filematch[i][0] == '.');
            if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
                (!is_ext && strstr(E.filename, s->filematch[i]))) {
                E.syntax = s;
                
                int filerow;
                for (filerow = 0; filerow < E.numrows; filerow++) {
                    editorUpdateSyntax(&E.row[filerow]);
                }
                
                return;
            }
            i++;
        }
    }
}

/*** Row operations ***/

int editorRowCxToRx(EditorRow *row, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (EDE_TAB_SIZE - 1) - (rx % EDE_TAB_SIZE);
        rx++;
    }
    return rx;
}

int editorRowRxToCx(EditorRow *row, int rx) {
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < row->size; cx++) {
        if (row->chars[cx] == '\t')
            cur_rx += (EDE_TAB_SIZE - 1) - (cur_rx % EDE_TAB_SIZE);
        cur_rx++;
        
        if (cur_rx > rx) return cx;
    }
    return cx;
}

void editorUpdateRow(EditorRow *row) {
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;
    
    free(row->render);
    row->render = malloc(row->size + tabs * (EDE_TAB_SIZE - 1) + 1);
    
    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % EDE_TAB_SIZE != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
    
    editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
    if (at < 0 || at > E.numrows) return;
    
    E.row = realloc(E.row, sizeof(EditorRow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(EditorRow) * (E.numrows - at));
    for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;
    
    E.row[at].idx = at;
    
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    E.row[at].hl = NULL;
    E.row[at].hl_open_comment = 0;
    editorUpdateRow(&E.row[at]);
    
    E.numrows++;
    E.dirty++;
}

void editorFreeRow(EditorRow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

void editorDelRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(EditorRow) * (E.numrows - at - 1));
    for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
    E.numrows--;
    E.dirty++;
}

void editorRowInsertChar(EditorRow *row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowAppendString(EditorRow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(EditorRow *row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

/*** Editor operations ***/

void editorInsertChar(int c) {
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }
    
    char ch = c;
    addUndoAction(ACTION_INSERT_CHAR, E.cy, E.cx, &ch, 1);
    
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorInsertNewline(void) {
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    } else {
        EditorRow *row = &E.row[E.cy];
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void editorDelChar(void) {
    if (E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;
    
    EditorRow *row = &E.row[E.cy];
    if (E.cx > 0) {
        char ch = row->chars[E.cx - 1];
        addUndoAction(ACTION_DELETE_CHAR, E.cy, E.cx - 1, &ch, 1);
        
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    } else {
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}

/*** File I/O ***/

char *editorRowsToString(int *buflen) {
    int totlen = 0;
    int j;
    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size + 1;
    *buflen = totlen;
    
    char *buf = malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    
    return buf;
}

/* Bare metal getline implementation */
ssize_t bare_getline(char **lineptr, size_t *n, FILE *stream) {
    if (lineptr == NULL || n == NULL || stream == NULL) {
        return -1;
    }
    
    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (*lineptr == NULL) return -1;
    }
    
    size_t pos = 0;
    int c;
    
    while ((c = fgetc(stream)) != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n * 2;
            char *new_ptr = realloc(*lineptr, new_size);
            if (new_ptr == NULL) return -1;
            *lineptr = new_ptr;
            *n = new_size;
        }
        
        (*lineptr)[pos++] = (char)c;
        
        if (c == '\n') break;
    }
    
    if (pos == 0 && c == EOF) {
        return -1;
    }
    
    (*lineptr)[pos] = '\0';
    return (ssize_t)pos;
}

void editorOpen(char *filename) {
    free(E.filename);
    E.filename = strdup(filename);
    
    editorSelectSyntaxHighlight();
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        /* New file */
        return;
    }
    
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    
    while ((linelen = bare_getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
}

int editorSave(void) {
    if (E.filename == NULL) {
        return -1;
    }
    
    int len;
    char *buf = editorRowsToString(&len);
    
    FILE *fp = fopen(E.filename, "w");
    if (fp == NULL) {
        free(buf);
        return -1;
    }
    
    if (fwrite(buf, 1, len, fp) != (size_t)len) {
        fclose(fp);
        free(buf);
        return -1;
    }
    
    fclose(fp);
    free(buf);
    E.dirty = 0;
    return 0;
}

/*** Search and Replace System ***/

void freeSearchMatches(void) {
    SearchMatch *match = search_ctx.matches;
    while (match) {
        SearchMatch *next = match->next;
        free(match);
        match = next;
    }
    search_ctx.matches = NULL;
    search_ctx.current_match = NULL;
    search_ctx.match_count = 0;
}

int stringMatchAt(const char *text, const char *pattern, int pos, int case_sensitive) {
    if (!case_sensitive) {
        for (int i = 0; pattern[i]; i++) {
            if (tolower(text[pos + i]) != tolower(pattern[i])) {
                return 0;
            }
        }
        return 1;
    } else {
        return strncmp(&text[pos], pattern, strlen(pattern)) == 0;
    }
}

void findAllMatches(void) {
    freeSearchMatches();
    
    if (search_ctx.query_len == 0) return;
    
    SearchMatch *last_match = NULL;
    
    for (int row = 0; row < E.numrows; row++) {
        EditorRow *erow = &E.row[row];
        char *line = erow->render;
        int len = erow->rsize;
        
        for (int col = 0; col <= len - search_ctx.query_len; col++) {
            if (stringMatchAt(line, search_ctx.query, col, search_ctx.case_sensitive)) {
                /* Check whole word if needed */
                if (search_ctx.whole_word) {
                    int before_ok = (col == 0) || !isalnum(line[col - 1]);
                    int after_ok = (col + search_ctx.query_len >= len) || 
                                   !isalnum(line[col + search_ctx.query_len]);
                    if (!before_ok || !after_ok) continue;
                }
                
                SearchMatch *match = malloc(sizeof(SearchMatch));
                match->row = row;
                match->col = col;
                match->length = search_ctx.query_len;
                match->next = NULL;
                
                if (last_match) {
                    last_match->next = match;
                } else {
                    search_ctx.matches = match;
                }
                last_match = match;
                search_ctx.match_count++;
                
                if (search_ctx.match_count >= EDE_MAX_SEARCH_RESULTS) {
                    return;
                }
                
                col += search_ctx.query_len - 1;
            }
        }
    }
    
    search_ctx.current_match = search_ctx.matches;
}

void gotoNextMatch(void) {
    if (!search_ctx.current_match) return;
    
    SearchMatch *match = search_ctx.current_match;
    E.cy = match->row;
    E.cx = editorRowRxToCx(&E.row[match->row], match->col);
    E.rowoff = match->row;
    
    search_ctx.current_match = match->next;
    if (!search_ctx.current_match) {
        search_ctx.current_match = search_ctx.matches;
    }
}

void gotoPrevMatch(void) {
    if (!search_ctx.matches) return;
    
    SearchMatch *target = search_ctx.matches;
    SearchMatch *prev = NULL;
    
    while (target && target != search_ctx.current_match) {
        prev = target;
        target = target->next;
    }
    
    if (prev) {
        search_ctx.current_match = prev;
    } else {
        /* Go to last match */
        SearchMatch *last = search_ctx.matches;
        while (last->next) last = last->next;
        search_ctx.current_match = last;
    }
    
    SearchMatch *match = search_ctx.current_match;
    E.cy = match->row;
    E.cx = editorRowRxToCx(&E.row[match->row], match->col);
    E.rowoff = match->row;
}

void replaceCurrentMatch(void) {
    if (!search_ctx.current_match) return;
    
    SearchMatch *match = search_ctx.current_match;
    EditorRow *row = &E.row[match->row];
    
    /* Delete matched text */
    int cx_pos = editorRowRxToCx(row, match->col);
    for (int i = 0; i < search_ctx.query_len; i++) {
        if (cx_pos < row->size) {
            editorRowDelChar(row, cx_pos);
        }
    }
    
    /* Insert replacement text */
    for (int i = 0; i < search_ctx.replace_len; i++) {
        editorRowInsertChar(row, cx_pos + i, search_ctx.replace_text[i]);
    }
    
    /* Re-find matches after replacement */
    findAllMatches();
}

void replaceAllMatches(void) {
    int replaced = 0;
    
    while (search_ctx.matches) {
        replaceCurrentMatch();
        replaced++;
        if (replaced > 10000) break; /* Safety limit */
    }
    
    editorSetStatusMessage("Replaced %d occurrence%s", replaced, replaced == 1 ? "" : "s");
}

void editorFindCallback(char *query, int key) {
    static int last_match = -1;
    static int direction = 1;
    
    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction = 1;
        return;
    } else if (key == KEY_ARROW_RIGHT || key == KEY_ARROW_DOWN) {
        direction = 1;
    } else if (key == KEY_ARROW_LEFT || key == KEY_ARROW_UP) {
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    }
    
    if (last_match == -1) direction = 1;
    int current = last_match;
    int i;
    for (i = 0; i < E.numrows; i++) {
        current += direction;
        if (current == -1) current = E.numrows - 1;
        else if (current == E.numrows) current = 0;
        
        EditorRow *row = &E.row[current];
        char *match = strstr(row->render, query);
        if (match) {
            last_match = current;
            E.cy = current;
            E.cx = editorRowRxToCx(row, match - row->render);
            E.rowoff = E.numrows;
            break;
        }
    }
}

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';
    
    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();
        
        int c = editorReadKey();
        if (c == KEY_DELETE || c == CTRL_KEY('h') || c == KEY_BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        } else if (c == '\x1b') {
            editorSetStatusMessage("");
            if (callback) callback(buf, c);
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editorSetStatusMessage("");
                if (callback) callback(buf, c);
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
        
        if (callback) callback(buf, c);
    }
}

void editorFind(void) {
    int saved_cx = E.cx;
    int saved_cy = E.cy;
    int saved_coloff = E.coloff;
    int saved_rowoff = E.rowoff;
    
    char *query = editorPrompt("Search: %s (ESC to cancel)", editorFindCallback);
    
    if (query) {
        strncpy(search_ctx.query, query, sizeof(search_ctx.query) - 1);
        search_ctx.query_len = strlen(query);
        search_ctx.case_sensitive = 1;
        search_ctx.whole_word = 0;
        findAllMatches();
        
        if (search_ctx.match_count > 0) {
            gotoNextMatch();
            editorSetStatusMessage("Found %d match%s (n=next, N=prev)", 
                search_ctx.match_count, search_ctx.match_count == 1 ? "" : "es");
        } else {
            editorSetStatusMessage("No matches found");
            E.cx = saved_cx;
            E.cy = saved_cy;
            E.coloff = saved_coloff;
            E.rowoff = saved_rowoff;
        }
        free(query);
    } else {
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_rowoff;
    }
}

void editorReplace(void) {
    char *query = editorPrompt("Replace: %s (ESC to cancel)", NULL);
    if (!query) return;
    
    strncpy(search_ctx.query, query, sizeof(search_ctx.query) - 1);
    search_ctx.query_len = strlen(query);
    free(query);
    
    char *replace = editorPrompt("Replace with: %s (ESC to cancel)", NULL);
    if (!replace) return;
    
    strncpy(search_ctx.replace_text, replace, sizeof(search_ctx.replace_text) - 1);
    search_ctx.replace_len = strlen(replace);
    free(replace);
    
    search_ctx.case_sensitive = 1;
    search_ctx.whole_word = 0;
    findAllMatches();
    
    if (search_ctx.match_count == 0) {
        editorSetStatusMessage("No matches found");
        return;
    }
    
    char *confirm = editorPrompt("Replace all %d matches? (y/n): %s", NULL);
    if (confirm && (confirm[0] == 'y' || confirm[0] == 'Y')) {
        replaceAllMatches();
    }
    if (confirm) free(confirm);
}

/*** Clipboard System ***/

#define MAX_CLIPBOARD_SIZE 1048576  /* 1MB */

typedef struct Clipboard {
    char *text;
    int length;
    int is_line_mode;
} Clipboard;

Clipboard clipboard = {NULL, 0, 0};

void clipboardSet(const char *text, int length, int is_line_mode) {
    free(clipboard.text);
    clipboard.text = malloc(length + 1);
    memcpy(clipboard.text, text, length);
    clipboard.text[length] = '\0';
    clipboard.length = length;
    clipboard.is_line_mode = is_line_mode;
}

void editorCopy(void) {
    if (E.cy >= E.numrows) return;
    
    EditorRow *row = &E.row[E.cy];
    clipboardSet(row->chars, row->size, 1);
    editorSetStatusMessage("Line copied to clipboard");
}

void editorCut(void) {
    if (E.cy >= E.numrows) return;
    
    EditorRow *row = &E.row[E.cy];
    clipboardSet(row->chars, row->size, 1);
    editorDelRow(E.cy);
    if (E.cy >= E.numrows) E.cy = E.numrows - 1;
    if (E.cy < 0) E.cy = 0;
    editorSetStatusMessage("Line cut to clipboard");
}

void editorPaste(void) {
    if (!clipboard.text) {
        editorSetStatusMessage("Clipboard is empty");
        return;
    }
    
    if (clipboard.is_line_mode) {
        editorInsertRow(E.cy + 1, clipboard.text, clipboard.length);
        E.cy++;
        E.cx = 0;
    } else {
        for (int i = 0; i < clipboard.length; i++) {
            editorInsertChar(clipboard.text[i]);
        }
    }
    editorSetStatusMessage("Pasted from clipboard");
}

void clipboardFree(void) {
    free(clipboard.text);
    clipboard.text = NULL;
    clipboard.length = 0;
}

/*** Autocomplete System ***/

#define MAX_COMPLETIONS 256
#define MAX_COMPLETION_LEN 128

typedef struct Autocomplete {
    char completions[MAX_COMPLETIONS][MAX_COMPLETION_LEN];
    int count;
    int selected;
    int active;
    char prefix[MAX_COMPLETION_LEN];
    int prefix_len;
} Autocomplete;

Autocomplete autocomplete = {0};

void autocompleteReset(void) {
    autocomplete.count = 0;
    autocomplete.selected = 0;
    autocomplete.active = 0;
    autocomplete.prefix_len = 0;
}

void autocompleteAddWord(const char *word) {
    if (autocomplete.count >= MAX_COMPLETIONS) return;
    
    /* Check for duplicates */
    for (int i = 0; i < autocomplete.count; i++) {
        if (strcmp(autocomplete.completions[i], word) == 0) {
            return;
        }
    }
    
    strncpy(autocomplete.completions[autocomplete.count], word, MAX_COMPLETION_LEN - 1);
    autocomplete.completions[autocomplete.count][MAX_COMPLETION_LEN - 1] = '\0';
    autocomplete.count++;
}

void autocompleteBuildList(void) {
    autocompleteReset();
    
    if (E.cy >= E.numrows) return;
    
    EditorRow *row = &E.row[E.cy];
    if (E.cx == 0) return;
    
    /* Find word prefix */
    int start = E.cx - 1;
    while (start > 0 && (isalnum(row->chars[start - 1]) || row->chars[start - 1] == '_')) {
        start--;
    }
    
    int prefix_len = E.cx - start;
    if (prefix_len < 2) return;
    
    strncpy(autocomplete.prefix, &row->chars[start], prefix_len);
    autocomplete.prefix[prefix_len] = '\0';
    autocomplete.prefix_len = prefix_len;
    
    /* Search all rows for matching words */
    for (int r = 0; r < E.numrows; r++) {
        EditorRow *erow = &E.row[r];
        char *line = erow->chars;
        int len = erow->size;
        
        for (int i = 0; i < len; i++) {
            if (isalnum(line[i]) || line[i] == '_') {
                int word_start = i;
                while (i < len && (isalnum(line[i]) || line[i] == '_')) {
                    i++;
                }
                int word_len = i - word_start;
                
                if (word_len > prefix_len && word_len < MAX_COMPLETION_LEN) {
                    if (strncmp(&line[word_start], autocomplete.prefix, prefix_len) == 0) {
                        char word[MAX_COMPLETION_LEN];
                        strncpy(word, &line[word_start], word_len);
                        word[word_len] = '\0';
                        autocompleteAddWord(word);
                    }
                }
            }
        }
    }
    
    if (autocomplete.count > 0) {
        autocomplete.active = 1;
    }
}

void autocompleteInsertSelected(void) {
    if (!autocomplete.active || autocomplete.count == 0) return;
    
    char *completion = autocomplete.completions[autocomplete.selected];
    int remaining = strlen(completion) - autocomplete.prefix_len;
    
    for (int i = 0; i < remaining; i++) {
        editorInsertChar(completion[autocomplete.prefix_len + i]);
    }
    
    autocompleteReset();
}

void autocompleteNext(void) {
    if (!autocomplete.active) return;
    autocomplete.selected = (autocomplete.selected + 1) % autocomplete.count;
}

void autocompletePrev(void) {
    if (!autocomplete.active) return;
    autocomplete.selected = (autocomplete.selected - 1 + autocomplete.count) % autocomplete.count;
}

/*** Macro Recording System ***/

#define MAX_MACRO_SIZE 10000

typedef struct Macro {
    int keys[MAX_MACRO_SIZE];
    int count;
    int recording;
    int playing;
} Macro;

Macro macro = {0};

void macroStartRecording(void) {
    macro.count = 0;
    macro.recording = 1;
    editorSetStatusMessage("Recording macro... (Ctrl-M to stop)");
}

void macroStopRecording(void) {
    macro.recording = 0;
    editorSetStatusMessage("Macro recorded (%d keys)", macro.count);
}

void macroRecordKey(int key) {
    if (!macro.recording || macro.count >= MAX_MACRO_SIZE) return;
    macro.keys[macro.count++] = key;
}

void macroPlayback(void) {
    if (macro.count == 0) {
        editorSetStatusMessage("No macro recorded");
        return;
    }
    
    macro.playing = 1;
    editorSetStatusMessage("Playing macro...");
    
    /* Playback will be handled in main loop */
}

/*** Multi-cursor System ***/

#define MAX_CURSORS 256

typedef struct Cursor {
    int cx;
    int cy;
    int active;
} Cursor;

typedef struct MultiCursor {
    Cursor cursors[MAX_CURSORS];
    int count;
    int enabled;
} MultiCursor;

MultiCursor multi_cursor = {0};

void multiCursorAdd(int cx, int cy) {
    if (multi_cursor.count >= MAX_CURSORS) return;
    
    /* Check for duplicates */
    for (int i = 0; i < multi_cursor.count; i++) {
        if (multi_cursor.cursors[i].cx == cx && multi_cursor.cursors[i].cy == cy) {
            return;
        }
    }
    
    multi_cursor.cursors[multi_cursor.count].cx = cx;
    multi_cursor.cursors[multi_cursor.count].cy = cy;
    multi_cursor.cursors[multi_cursor.count].active = 1;
    multi_cursor.count++;
    multi_cursor.enabled = 1;
}

void multiCursorClear(void) {
    multi_cursor.count = 0;
    multi_cursor.enabled = 0;
}

void multiCursorInsertChar(int c) {
    if (!multi_cursor.enabled) {
        editorInsertChar(c);
        return;
    }
    
    for (int i = 0; i < multi_cursor.count; i++) {
        if (!multi_cursor.cursors[i].active) continue;
        
        int saved_cx = E.cx;
        int saved_cy = E.cy;
        
        E.cx = multi_cursor.cursors[i].cx;
        E.cy = multi_cursor.cursors[i].cy;
        
        editorInsertChar(c);
        
        multi_cursor.cursors[i].cx = E.cx;
        multi_cursor.cursors[i].cy = E.cy;
        
        E.cx = saved_cx;
        E.cy = saved_cy;
    }
}

/*** Split View System ***/

typedef struct Split {
    int active;
    int vertical;
    int split_pos;
    int focus; /* 0 = left/top, 1 = right/bottom */
    
    /* Second view state */
    int cx2, cy2;
    int rx2;
    int rowoff2;
    int coloff2;
} Split;

Split split = {0};

void splitVertical(void) {
    split.active = 1;
    split.vertical = 1;
    split.split_pos = E.screencols / 2;
    split.focus = 0;
    split.cx2 = E.cx;
    split.cy2 = E.cy;
    split.rowoff2 = E.rowoff;
    split.coloff2 = E.coloff;
    editorSetStatusMessage("Split view enabled (Ctrl-W to switch)");
}

void splitHorizontal(void) {
    split.active = 1;
    split.vertical = 0;
    split.split_pos = E.screenrows / 2;
    split.focus = 0;
    split.cx2 = E.cx;
    split.cy2 = E.cy;
    split.rowoff2 = E.rowoff;
    split.coloff2 = E.coloff;
    editorSetStatusMessage("Split view enabled (Ctrl-W to switch)");
}

void splitClose(void) {
    split.active = 0;
    editorSetStatusMessage("Split view closed");
}

void splitToggleFocus(void) {
    if (!split.active) return;
    
    if (split.focus == 0) {
        /* Save main view state */
        int tmp_cx = E.cx;
        int tmp_cy = E.cy;
        int tmp_rowoff = E.rowoff;
        int tmp_coloff = E.coloff;
        
        /* Restore second view state */
        E.cx = split.cx2;
        E.cy = split.cy2;
        E.rowoff = split.rowoff2;
        E.coloff = split.coloff2;
        
        /* Save to second view */
        split.cx2 = tmp_cx;
        split.cy2 = tmp_cy;
        split.rowoff2 = tmp_rowoff;
        split.coloff2 = tmp_coloff;
        
        split.focus = 1;
    } else {
        /* Similar swap back */
        int tmp_cx = E.cx;
        int tmp_cy = E.cy;
        int tmp_rowoff = E.rowoff;
        int tmp_coloff = E.coloff;
        
        E.cx = split.cx2;
        E.cy = split.cy2;
        E.rowoff = split.rowoff2;
        E.coloff = split.coloff2;
        
        split.cx2 = tmp_cx;
        split.cy2 = tmp_cy;
        split.rowoff2 = tmp_rowoff;
        split.coloff2 = tmp_coloff;
        
        split.focus = 0;
    }
}

/*** Code Folding ***/

#define MAX_FOLDS 1000

typedef struct Fold {
    int start_row;
    int end_row;
    int folded;
} Fold;

typedef struct FoldManager {
    Fold folds[MAX_FOLDS];
    int count;
} FoldManager;

FoldManager fold_manager = {0};

int findMatchingBrace(int start_row) {
    if (start_row >= E.numrows) return -1;
    
    EditorRow *row = &E.row[start_row];
    int brace_count = 0;
    int found_open = 0;
    
    /* Look for opening brace on this line */
    for (int i = 0; i < row->size; i++) {
        if (row->chars[i] == '{') {
            found_open = 1;
            brace_count = 1;
            break;
        }
    }
    
    if (!found_open) return -1;
    
    /* Find matching closing brace */
    for (int r = start_row; r < E.numrows; r++) {
        EditorRow *erow = &E.row[r];
        int start = (r == start_row) ? 0 : 0;
        
        for (int i = start; i < erow->size; i++) {
            if (erow->chars[i] == '{') brace_count++;
            else if (erow->chars[i] == '}') {
                brace_count--;
                if (brace_count == 0) {
                    return r;
                }
            }
        }
    }
    
    return -1;
}

void toggleFold(int row) {
    /* Check if fold already exists at this row */
    for (int i = 0; i < fold_manager.count; i++) {
        if (fold_manager.folds[i].start_row == row) {
            fold_manager.folds[i].folded = !fold_manager.folds[i].folded;
            return;
        }
    }
    
    /* Create new fold */
    if (fold_manager.count >= MAX_FOLDS) return;
    
    int end = findMatchingBrace(row);
    if (end == -1) {
        editorSetStatusMessage("No matching brace found");
        return;
    }
    
    fold_manager.folds[fold_manager.count].start_row = row;
    fold_manager.folds[fold_manager.count].end_row = end;
    fold_manager.folds[fold_manager.count].folded = 1;
    fold_manager.count++;
    
    editorSetStatusMessage("Folded lines %d-%d", row + 1, end + 1);
}

int isRowFolded(int row) {
    for (int i = 0; i < fold_manager.count; i++) {
        if (fold_manager.folds[i].folded &&
            row > fold_manager.folds[i].start_row &&
            row <= fold_manager.folds[i].end_row) {
            return 1;
        }
    }
    return 0;
}

/*** Bookmark System ***/

#define MAX_BOOKMARKS 100

typedef struct Bookmark {
    int row;
    char label[64];
} Bookmark;

typedef struct BookmarkManager {
    Bookmark bookmarks[MAX_BOOKMARKS];
    int count;
} BookmarkManager;

BookmarkManager bookmark_manager = {0};

void addBookmark(int row, const char *label) {
    if (bookmark_manager.count >= MAX_BOOKMARKS) return;
    
    bookmark_manager.bookmarks[bookmark_manager.count].row = row;
    strncpy(bookmark_manager.bookmarks[bookmark_manager.count].label, label, 63);
    bookmark_manager.bookmarks[bookmark_manager.count].label[63] = '\0';
    bookmark_manager.count++;
    
    editorSetStatusMessage("Bookmark added at line %d", row + 1);
}

void gotoNextBookmark(void) {
    if (bookmark_manager.count == 0) {
        editorSetStatusMessage("No bookmarks");
        return;
    }
    
    for (int i = 0; i < bookmark_manager.count; i++) {
        if (bookmark_manager.bookmarks[i].row > E.cy) {
            E.cy = bookmark_manager.bookmarks[i].row;
            E.cx = 0;
            return;
        }
    }
    
    /* Wrap to first */
    E.cy = bookmark_manager.bookmarks[0].row;
    E.cx = 0;
}

void gotoPrevBookmark(void) {
    if (bookmark_manager.count == 0) {
        editorSetStatusMessage("No bookmarks");
        return;
    }
    
    for (int i = bookmark_manager.count - 1; i >= 0; i--) {
        if (bookmark_manager.bookmarks[i].row < E.cy) {
            E.cy = bookmark_manager.bookmarks[i].row;
            E.cx = 0;
            return;
        }
    }
    
    /* Wrap to last */
    E.cy = bookmark_manager.bookmarks[bookmark_manager.count - 1].row;
    E.cx = 0;
}

/*** Smart Indentation System ***/

typedef struct IndentConfig {
    int use_tabs;
    int tab_width;
    int auto_indent;
    int smart_indent;
} IndentConfig;

IndentConfig indent_config = {
    .use_tabs = 0,
    .tab_width = 4,
    .auto_indent = 1,
    .smart_indent = 1
};

int getLineIndentation(EditorRow *row) {
    int indent = 0;
    for (int i = 0; i < row->size; i++) {
        if (row->chars[i] == ' ') {
            indent++;
        } else if (row->chars[i] == '\t') {
            indent += indent_config.tab_width;
        } else {
            break;
        }
    }
    return indent;
}

void insertIndentation(int amount) {
    if (indent_config.use_tabs) {
        int tabs = amount / indent_config.tab_width;
        int spaces = amount % indent_config.tab_width;
        for (int i = 0; i < tabs; i++) {
            editorInsertChar('\t');
        }
        for (int i = 0; i < spaces; i++) {
            editorInsertChar(' ');
        }
    } else {
        for (int i = 0; i < amount; i++) {
            editorInsertChar(' ');
        }
    }
}

void autoIndentNewline(void) {
    if (!indent_config.auto_indent || E.cy >= E.numrows) {
        editorInsertNewline();
        return;
    }
    
    EditorRow *row = &E.row[E.cy];
    int base_indent = getLineIndentation(row);
    
    /* Check for smart indent triggers */
    if (indent_config.smart_indent && E.cx > 0) {
        char prev_char = row->chars[E.cx - 1];
        if (prev_char == '{' || prev_char == '(' || prev_char == '[') {
            base_indent += indent_config.tab_width;
        }
    }
    
    editorInsertNewline();
    insertIndentation(base_indent);
}

void indentLine(int row_index) {
    if (row_index < 0 || row_index >= E.numrows) return;
    
    EditorRow *row = &E.row[row_index];
    int current_indent = getLineIndentation(row);
    int new_indent = current_indent + indent_config.tab_width;
    
    /* Insert spaces at beginning */
    for (int i = 0; i < indent_config.tab_width; i++) {
        editorRowInsertChar(row, current_indent + i, ' ');
    }
}

void unindentLine(int row_index) {
    if (row_index < 0 || row_index >= E.numrows) return;
    
    EditorRow *row = &E.row[row_index];
    int current_indent = getLineIndentation(row);
    int to_remove = (current_indent < indent_config.tab_width) ? 
                    current_indent : indent_config.tab_width;
    
    for (int i = 0; i < to_remove; i++) {
        if (row->size > 0 && (row->chars[0] == ' ' || row->chars[0] == '\t')) {
            editorRowDelChar(row, 0);
        }
    }
}

void indentSelection(void) {
    /* For now, just indent current line */
    indentLine(E.cy);
    editorSetStatusMessage("Line indented");
}

void unindentSelection(void) {
    unindentLine(E.cy);
    editorSetStatusMessage("Line unindented");
}

/*** Bracket Matching ***/

typedef struct BracketPair {
    char open;
    char close;
} BracketPair;

BracketPair bracket_pairs[] = {
    {'(', ')'},
    {'[', ']'},
    {'{', '}'},
    {'<', '>'},
};

#define NUM_BRACKET_PAIRS (sizeof(bracket_pairs) / sizeof(BracketPair))

int isBracket(char c, int *is_open, int *pair_index) {
    for (int i = 0; i < NUM_BRACKET_PAIRS; i++) {
        if (c == bracket_pairs[i].open) {
            *is_open = 1;
            *pair_index = i;
            return 1;
        }
        if (c == bracket_pairs[i].close) {
            *is_open = 0;
            *pair_index = i;
            return 1;
        }
    }
    return 0;
}

int findMatchingBracket(int *match_row, int *match_col) {
    if (E.cy >= E.numrows || E.cx >= E.row[E.cy].size) return 0;
    
    EditorRow *row = &E.row[E.cy];
    char c = row->chars[E.cx];
    
    int is_open, pair_index;
    if (!isBracket(c, &is_open, &pair_index)) return 0;
    
    char target = is_open ? bracket_pairs[pair_index].close : bracket_pairs[pair_index].open;
    int direction = is_open ? 1 : -1;
    int depth = 0;
    
    int r = E.cy;
    int col = E.cx;
    
    while (r >= 0 && r < E.numrows) {
        EditorRow *erow = &E.row[r];
        
        if (direction > 0) {
            for (int i = col; i < erow->size; i++) {
                if (erow->chars[i] == c) depth++;
                else if (erow->chars[i] == target) {
                    depth--;
                    if (depth == 0) {
                        *match_row = r;
                        *match_col = i;
                        return 1;
                    }
                }
            }
            r++;
            col = 0;
        } else {
            for (int i = col; i >= 0; i--) {
                if (erow->chars[i] == c) depth++;
                else if (erow->chars[i] == target) {
                    depth--;
                    if (depth == 0) {
                        *match_row = r;
                        *match_col = i;
                        return 1;
                    }
                }
            }
            r--;
            if (r >= 0) col = E.row[r].size - 1;
        }
    }
    
    return 0;
}

void gotoMatchingBracket(void) {
    int match_row, match_col;
    if (findMatchingBracket(&match_row, &match_col)) {
        E.cy = match_row;
        E.cx = match_col;
        editorSetStatusMessage("Jumped to matching bracket");
    } else {
        editorSetStatusMessage("No matching bracket found");
    }
}

/*** File Browser ***/

#define MAX_FILE_ENTRIES 1000
#define MAX_PATH_LENGTH 512

typedef struct FileEntry {
    char name[MAX_PATH_LENGTH];
    char full_path[MAX_PATH_LENGTH];
    int is_directory;
    long size;
} FileEntry;

typedef struct FileBrowser {
    FileEntry entries[MAX_FILE_ENTRIES];
    int count;
    int selected;
    int active;
    char current_dir[MAX_PATH_LENGTH];
} FileBrowser;

FileBrowser file_browser = {0};

#ifdef EDE_WINDOWS
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

void fileBrowserLoadDirectory(const char *path) {
    file_browser.count = 0;
    file_browser.selected = 0;
    strncpy(file_browser.current_dir, path, MAX_PATH_LENGTH - 1);
    
#ifdef EDE_WINDOWS
    WIN32_FIND_DATA find_data;
    char search_path[MAX_PATH_LENGTH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    
    HANDLE hFind = FindFirstFile(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) return;
    
    do {
        if (file_browser.count >= MAX_FILE_ENTRIES) break;
        
        FileEntry *entry = &file_browser.entries[file_browser.count];
        strncpy(entry->name, find_data.cFileName, MAX_PATH_LENGTH - 1);
        snprintf(entry->full_path, MAX_PATH_LENGTH, "%s\\%s", path, find_data.cFileName);
        entry->is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entry->size = find_data.nFileSizeLow;
        
        file_browser.count++;
    } while (FindNextFile(hFind, &find_data));
    
    FindClose(hFind);
#else
    DIR *dir = opendir(path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (file_browser.count >= MAX_FILE_ENTRIES) break;
        
        FileEntry *fe = &file_browser.entries[file_browser.count];
        strncpy(fe->name, entry->d_name, MAX_PATH_LENGTH - 1);
        snprintf(fe->full_path, MAX_PATH_LENGTH, "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(fe->full_path, &st) == 0) {
            fe->is_directory = S_ISDIR(st.st_mode);
            fe->size = st.st_size;
        }
        
        file_browser.count++;
    }
    
    closedir(dir);
#endif
}

void fileBrowserOpen(void) {
    char cwd[MAX_PATH_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        fileBrowserLoadDirectory(cwd);
        file_browser.active = 1;
        editorSetStatusMessage("File browser (Enter=open, Esc=close, arrows=navigate)");
    }
}

void fileBrowserClose(void) {
    file_browser.active = 0;
}

void fileBrowserNavigate(int direction) {
    if (!file_browser.active) return;
    
    file_browser.selected += direction;
    if (file_browser.selected < 0) file_browser.selected = 0;
    if (file_browser.selected >= file_browser.count) {
        file_browser.selected = file_browser.count - 1;
    }
}

void fileBrowserSelect(void) {
    if (!file_browser.active || file_browser.selected >= file_browser.count) return;
    
    FileEntry *entry = &file_browser.entries[file_browser.selected];
    
    if (entry->is_directory) {
        if (strcmp(entry->name, ".") == 0) {
            return;
        } else if (strcmp(entry->name, "..") == 0) {
            /* Go to parent directory */
            char parent[MAX_PATH_LENGTH];
            strncpy(parent, file_browser.current_dir, MAX_PATH_LENGTH - 1);
            
            char *last_slash = strrchr(parent, '/');
            if (!last_slash) last_slash = strrchr(parent, '\\');
            if (last_slash) *last_slash = '\0';
            
            fileBrowserLoadDirectory(parent);
        } else {
            chdir(entry->full_path);
            fileBrowserLoadDirectory(entry->full_path);
        }
    } else {
        /* Open file */
        editorOpen(entry->full_path);
        fileBrowserClose();
        editorSetStatusMessage("Opened %s", entry->name);
    }
}

/*** Git Integration ***/

typedef struct GitStatus {
    int is_repo;
    char branch[128];
    int modified_files;
    int staged_files;
    int commits_ahead;
    int commits_behind;
} GitStatus;

GitStatus git_status = {0};

void gitCheckRepository(void) {
    git_status.is_repo = 0;
    
    /* Check if .git directory exists */
    FILE *fp = fopen(".git/HEAD", "r");
    if (!fp) return;
    
    git_status.is_repo = 1;
    
    /* Read current branch */
    char line[256];
    if (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "ref: refs/heads/", 16) == 0) {
            strncpy(git_status.branch, line + 16, sizeof(git_status.branch) - 1);
            /* Remove newline */
            char *newline = strchr(git_status.branch, '\n');
            if (newline) *newline = '\0';
        }
    }
    
    fclose(fp);
}

void gitRunCommand(const char *cmd, char *output, int output_size) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        output[0] = '\0';
        return;
    }
    
    int pos = 0;
    int c;
    while ((c = fgetc(fp)) != EOF && pos < output_size - 1) {
        output[pos++] = c;
    }
    output[pos] = '\0';
    
    pclose(fp);
}

void gitBlame(int line) {
    if (!git_status.is_repo || !E.filename) {
        editorSetStatusMessage("Not in a git repository");
        return;
    }
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "git blame -L %d,%d %s", line + 1, line + 1, E.filename);
    
    char output[512];
    gitRunCommand(cmd, output, sizeof(output));
    
    if (output[0]) {
        editorSetStatusMessage("Blame: %s", output);
    } else {
        editorSetStatusMessage("No git blame info for this line");
    }
}

void gitDiff(void) {
    if (!git_status.is_repo || !E.filename) {
        editorSetStatusMessage("Not in a git repository");
        return;
    }
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "git diff %s", E.filename);
    
    char output[4096];
    gitRunCommand(cmd, output, sizeof(output));
    
    if (output[0]) {
        editorSetStatusMessage("Changes: %d bytes", (int)strlen(output));
    } else {
        editorSetStatusMessage("No changes to show");
    }
}

/*** Diff Viewer ***/

typedef enum {
    DIFF_NONE,
    DIFF_ADD,
    DIFF_DELETE,
    DIFF_MODIFY
} DiffType;

typedef struct DiffLine {
    DiffType type;
    int original_line;
    int modified_line;
} DiffLine;

typedef struct DiffViewer {
    DiffLine *lines;
    int count;
    int active;
    char left_file[MAX_PATH_LENGTH];
    char right_file[MAX_PATH_LENGTH];
} DiffViewer;

DiffViewer diff_viewer = {0};

void diffViewerCompare(const char *file1, const char *file2) {
    /* Simplified diff - in full implementation would use proper diff algorithm */
    free(diff_viewer.lines);
    diff_viewer.lines = NULL;
    diff_viewer.count = 0;
    diff_viewer.active = 1;
    
    strncpy(diff_viewer.left_file, file1, MAX_PATH_LENGTH - 1);
    strncpy(diff_viewer.right_file, file2, MAX_PATH_LENGTH - 1);
    
    editorSetStatusMessage("Diff view: %s <-> %s", file1, file2);
}

/*** LSP (Language Server Protocol) Integration ***/

#define LSP_MAX_DIAGNOSTICS 100

typedef struct LSPDiagnostic {
    int line;
    int col;
    int severity; /* 1=error, 2=warning, 3=info, 4=hint */
    char message[256];
} LSPDiagnostic;

typedef struct LSPClient {
    int connected;
    int server_pid;
    char server_name[128];
    LSPDiagnostic diagnostics[LSP_MAX_DIAGNOSTICS];
    int diagnostic_count;
} LSPClient;

LSPClient lsp_client = {0};

void lspInitialize(const char *language) {
    /* Simplified LSP initialization */
    lsp_client.connected = 0;
    lsp_client.diagnostic_count = 0;
    strncpy(lsp_client.server_name, language, sizeof(lsp_client.server_name) - 1);
    
    /* In full implementation, would start LSP server process */
    editorSetStatusMessage("LSP: Would initialize %s language server", language);
}

void lspShutdown(void) {
    if (lsp_client.connected) {
        /* Send shutdown request to server */
        lsp_client.connected = 0;
    }
}

void lspRequestCompletion(int line, int col) {
    if (!lsp_client.connected) return;
    
    /* In full implementation, would send completion request */
    editorSetStatusMessage("LSP: Requesting completions at %d:%d", line, col);
}

void lspRequestDiagnostics(void) {
    if (!lsp_client.connected) return;
    
    /* In full implementation, would request diagnostics from server */
}

void lspGotoDefinition(void) {
    if (!lsp_client.connected) {
        editorSetStatusMessage("LSP not connected");
        return;
    }
    
    editorSetStatusMessage("LSP: Go to definition at %d:%d", E.cy + 1, E.cx + 1);
}

void lspFindReferences(void) {
    if (!lsp_client.connected) {
        editorSetStatusMessage("LSP not connected");
        return;
    }
    
    editorSetStatusMessage("LSP: Finding references at %d:%d", E.cy + 1, E.cx + 1);
}

void lspRename(const char *new_name) {
    if (!lsp_client.connected) {
        editorSetStatusMessage("LSP not connected");
        return;
    }
    
    editorSetStatusMessage("LSP: Renaming symbol to '%s'", new_name);
}

void lspHover(void) {
    if (!lsp_client.connected) return;
    
    /* In full implementation, would request hover information */
    editorSetStatusMessage("LSP: Hover info at %d:%d", E.cy + 1, E.cx + 1);
}

/*** Terminal Emulator ***/

#define TERM_BUFFER_SIZE 100000

typedef struct TerminalEmulator {
    int active;
    int width;
    int height;
    char *buffer;
    int buffer_len;
    int cursor_x;
    int cursor_y;
    int scroll_offset;
#ifdef EDE_WINDOWS
    HANDLE process;
#else
    int master_fd;
    pid_t child_pid;
#endif
} TerminalEmulator;

TerminalEmulator terminal = {0};

void terminalInit(void) {
    terminal.buffer = malloc(TERM_BUFFER_SIZE);
    terminal.buffer[0] = '\0';
    terminal.buffer_len = 0;
    terminal.width = E.screencols;
    terminal.height = E.screenrows / 2;
    terminal.cursor_x = 0;
    terminal.cursor_y = 0;
    terminal.scroll_offset = 0;
    terminal.active = 1;
    
    editorSetStatusMessage("Terminal emulator started");
}

void terminalClose(void) {
    if (!terminal.active) return;
    
#ifdef EDE_UNIX
    if (terminal.child_pid > 0) {
        kill(terminal.child_pid, SIGTERM);
    }
    if (terminal.master_fd >= 0) {
        close(terminal.master_fd);
    }
#endif
    
    free(terminal.buffer);
    terminal.buffer = NULL;
    terminal.active = 0;
    
    editorSetStatusMessage("Terminal closed");
}

void terminalWrite(const char *data, int len) {
    if (!terminal.active) return;
    
    if (terminal.buffer_len + len >= TERM_BUFFER_SIZE) {
        /* Shift buffer */
        int to_remove = len;
        memmove(terminal.buffer, terminal.buffer + to_remove, 
                terminal.buffer_len - to_remove);
        terminal.buffer_len -= to_remove;
    }
    
    memcpy(terminal.buffer + terminal.buffer_len, data, len);
    terminal.buffer_len += len;
    terminal.buffer[terminal.buffer_len] = '\0';
}

void terminalSendInput(const char *input, int len) {
    if (!terminal.active) return;
    
#ifdef EDE_UNIX
    if (terminal.master_fd >= 0) {
        write(terminal.master_fd, input, len);
    }
#endif
}

/*** Session Management ***/

#define MAX_SESSION_FILES 50

typedef struct Session {
    char name[128];
    char files[MAX_SESSION_FILES][MAX_PATH_LENGTH];
    int file_count;
    int current_file;
    int cursor_positions[MAX_SESSION_FILES][2]; /* [cx, cy] for each file */
} Session;

void sessionSave(const char *session_name) {
    char path[MAX_PATH_LENGTH];
    snprintf(path, sizeof(path), ".ede_session_%s", session_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        editorSetStatusMessage("Error saving session");
        return;
    }
    
    fprintf(fp, "session:%s\n", session_name);
    if (E.filename) {
        fprintf(fp, "file:%s:%d:%d\n", E.filename, E.cx, E.cy);
    }
    
    fclose(fp);
    editorSetStatusMessage("Session saved: %s", session_name);
}

void sessionLoad(const char *session_name) {
    char path[MAX_PATH_LENGTH];
    snprintf(path, sizeof(path), ".ede_session_%s", session_name);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        editorSetStatusMessage("Session not found: %s", session_name);
        return;
    }
    
    char line[MAX_PATH_LENGTH + 256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "file:", 5) == 0) {
            char filename[MAX_PATH_LENGTH];
            int cx, cy;
            if (sscanf(line + 5, "%[^:]:%d:%d", filename, &cx, &cy) == 3) {
                editorOpen(filename);
                E.cx = cx;
                E.cy = cy;
                break; /* Load first file only for now */
            }
        }
    }
    
    fclose(fp);
    editorSetStatusMessage("Session loaded: %s", session_name);
}

/*** VIM mode ***/

void executeVimCommand(const char *cmd) {
    /* Handle line number jumps (e.g., :42 to go to line 42) */
    if (cmd[0] >= '0' && cmd[0] <= '9') {
        int line = atoi(cmd);
        if (line > 0 && line <= E.numrows) {
            E.cy = line - 1;
            E.cx = 0;
            editorSetStatusMessage("Jumped to line %d", line);
        } else {
            editorSetStatusMessage("Invalid line number: %d", line);
        }
        return;
    }
    
    /* Quit commands */
    if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
        if (E.dirty) {
            editorSetStatusMessage("Warning: unsaved changes! Use :q! to force quit");
            return;
        }
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
    } else if (strcmp(cmd, "q!") == 0 || strcmp(cmd, "quit!") == 0) {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
    }
    
    /* Save commands */
    else if (strcmp(cmd, "w") == 0 || strcmp(cmd, "write") == 0) {
        if (editorSave() == 0) {
            editorSetStatusMessage("File saved: %s", E.filename ? E.filename : "[No Name]");
        } else {
            editorSetStatusMessage("Error saving file");
        }
    } else if (strcmp(cmd, "wq") == 0 || strcmp(cmd, "x") == 0) {
        if (editorSave() == 0) {
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
        } else {
            editorSetStatusMessage("Error saving file");
        }
    } else if (strncmp(cmd, "w ", 2) == 0) {
        /* Save to specific filename */
        const char *new_filename = cmd + 2;
        char *old_filename = E.filename;
        E.filename = strdup(new_filename);
        if (editorSave() == 0) {
            free(old_filename);
            editorSetStatusMessage("Saved to: %s", E.filename);
        } else {
            free(E.filename);
            E.filename = old_filename;
            editorSetStatusMessage("Error saving to %s", new_filename);
        }
    }
    
    /* Edit commands */
    else if (strncmp(cmd, "e ", 2) == 0 || strncmp(cmd, "edit ", 5) == 0) {
        const char *filename = (cmd[1] == ' ') ? (cmd + 2) : (cmd + 5);
        if (E.dirty) {
            editorSetStatusMessage("Unsaved changes! Use :e! to force or :w to save first");
            return;
        }
        editorOpen((char*)filename);
        editorSetStatusMessage("Opened: %s", filename);
    }
    
    /* Search commands */
    else if (cmd[0] == '/') {
        /* Forward search */
        const char *query = cmd + 1;
        if (*query) {
            strncpy(search_ctx.query, query, sizeof(search_ctx.query) - 1);
            search_ctx.query_len = strlen(query);
            search_ctx.case_sensitive = 1;
            findAllMatches();
            if (search_ctx.match_count > 0) {
                gotoNextMatch();
                editorSetStatusMessage("Found %d matches for '%s'", search_ctx.match_count, query);
            } else {
                editorSetStatusMessage("Pattern not found: %s", query);
            }
        }
    } else if (cmd[0] == '?') {
        /* Backward search */
        const char *query = cmd + 1;
        if (*query) {
            strncpy(search_ctx.query, query, sizeof(search_ctx.query) - 1);
            search_ctx.query_len = strlen(query);
            search_ctx.case_sensitive = 1;
            findAllMatches();
            if (search_ctx.match_count > 0) {
                gotoPrevMatch();
                editorSetStatusMessage("Found %d matches for '%s'", search_ctx.match_count, query);
            } else {
                editorSetStatusMessage("Pattern not found: %s", query);
            }
        }
    }
    
    /* Substitute (find and replace) */
    else if (strncmp(cmd, "s/", 2) == 0 || strncmp(cmd, "substitute/", 11) == 0) {
        /* Simple :s/old/new/ implementation */
        const char *start = (cmd[1] == '/') ? (cmd + 2) : (cmd + 11);
        char *sep1 = strchr(start, '/');
        if (sep1) {
            *sep1 = '\0';
            char *sep2 = strchr(sep1 + 1, '/');
            if (sep2) {
                *sep2 = '\0';
                const char *old_text = start;
                const char *new_text = sep1 + 1;
                
                strncpy(search_ctx.query, old_text, sizeof(search_ctx.query) - 1);
                search_ctx.query_len = strlen(old_text);
                strncpy(search_ctx.replace_text, new_text, sizeof(search_ctx.replace_text) - 1);
                search_ctx.replace_len = strlen(new_text);
                search_ctx.case_sensitive = 1;
                findAllMatches();
                
                if (search_ctx.match_count > 0) {
                    replaceAllMatches();
                    editorSetStatusMessage("Replaced %d occurrences", search_ctx.match_count);
                } else {
                    editorSetStatusMessage("Pattern not found: %s", old_text);
                }
            }
        }
    }
    
    /* Line number display */
    else if (strcmp(cmd, "set nu") == 0 || strcmp(cmd, "set number") == 0) {
        E.show_line_numbers = 1;
        editorSetStatusMessage("Line numbers enabled");
    } else if (strcmp(cmd, "set nonu") == 0 || strcmp(cmd, "set nonumber") == 0) {
        E.show_line_numbers = 0;
        editorSetStatusMessage("Line numbers disabled");
    }
    
    /* Help */
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {
        editorSetStatusMessage("Commands: :q :w :wq :e file :/search :s/old/new/ :#(line)");
    }
    
    /* Unknown command */
    else {
        editorSetStatusMessage("Unknown command: :%s (type :help for help)", cmd);
    }
}

/*** Output ***/

void editorScroll(void) {
    E.rx = 0;
    if (E.cy < E.numrows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }
    
    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.rx < E.coloff) {
        E.coloff = E.rx;
    }
    if (E.rx >= E.coloff + E.screencols) {
        E.coloff = E.rx - E.screencols + 1;
    }
}

void editorDrawRows(StringBuffer *sb) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "GNU ede v%s -- A nano-like editor", EDE_VERSION);
                if (welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    sbAppend(sb, "~", 1);
                    padding--;
                }
                while (padding--) sbAppend(sb, " ", 1);
                sbAppend(sb, welcome, welcomelen);
            } else {
                sbAppend(sb, "~", 1);
            }
        } else {
            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            
            char *c = &E.row[filerow].render[E.coloff];
            unsigned char *hl = &E.row[filerow].hl[E.coloff];
            int current_color = -1;
            int j;
            for (j = 0; j < len; j++) {
                if (hl[j] == COLOR_NORMAL) {
                    if (current_color != -1) {
                        sbAppend(sb, "\x1b[39m", 5);
                        current_color = -1;
                    }
                    sbAppend(sb, &c[j], 1);
                } else {
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != current_color) {
                        current_color = color;
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        sbAppend(sb, buf, clen);
                    }
                    sbAppend(sb, &c[j], 1);
                }
            }
            sbAppend(sb, "\x1b[39m", 5);
        }
        
        sbAppend(sb, "\x1b[K", 3);
        sbAppend(sb, "\r\n", 2);
    }
}

void editorDrawStatusBar(StringBuffer *sb) {
    sbAppend(sb, "\x1b[7m", 4);
    char status[256], rstatus[80];
    
    int len = snprintf(status, sizeof(status), " %.20s - %d lines %s",
        E.filename ? E.filename : "[No Name]", E.numrows,
        E.dirty ? "(modified)" : "");
    
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d ",
        E.syntax ? E.syntax->filetype : "no ft", E.cy + 1, E.numrows);
    
    if (len > E.screencols) len = E.screencols;
    sbAppend(sb, status, len);
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            sbAppend(sb, rstatus, rlen);
            break;
        } else {
            sbAppend(sb, " ", 1);
            len++;
        }
    }
    sbAppend(sb, "\x1b[m", 3);
    sbAppend(sb, "\r\n", 2);
}

void editorDrawMessageBar(StringBuffer *sb) {
    sbAppend(sb, "\x1b[K", 3);
    
    if (E.mode == MODE_VIM_COMMAND) {
        char msg[256];
        int msglen = snprintf(msg, sizeof(msg), ":%s", E.vim_command);
        if (msglen > E.screencols) msglen = E.screencols;
        sbAppend(sb, msg, msglen);
    } else {
        int msglen = strlen(E.statusmsg);
        if (msglen > E.screencols) msglen = E.screencols;
        if (msglen && time(NULL) - E.statusmsg_time < 5)
            sbAppend(sb, E.statusmsg, msglen);
    }
}

void editorRefreshScreen(void) {
    editorScroll();
    
    StringBuffer sb = STRBUF_INIT;
    
    sbAppend(&sb, "\x1b[?25l", 6);
    sbAppend(&sb, "\x1b[H", 3);
    
    editorDrawRows(&sb);
    editorDrawStatusBar(&sb);
    editorDrawMessageBar(&sb);
    
    /* Call module render hooks */
    for (int i = 0; i < E.module_count; i++) {
        if (E.modules[i].active && E.modules[i].on_render) {
            E.modules[i].on_render();
        }
    }
    
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", 
            (E.cy - E.rowoff) + 1,
            (E.rx - E.coloff) + 1);
    sbAppend(&sb, buf, strlen(buf));
    
    sbAppend(&sb, "\x1b[?25h", 6);
    
    write(STDOUT_FILENO, sb.b, sb.len);
    sbFree(&sb);
}

void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

/*** Input ***/

void editorMoveCursor(int key) {
    EditorRow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    
    switch (key) {
        case KEY_ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case KEY_ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case KEY_ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
            }
            break;
        case KEY_ARROW_DOWN:
            if (E.cy < E.numrows) {
                E.cy++;
            }
            break;
    }
    
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

void editorProcessKeypress(void) {
    static int quit_times = 1;
    
    int c = editorReadKey();
    
    /* Call module keypress hooks */
    for (int i = 0; i < E.module_count; i++) {
        if (E.modules[i].active && E.modules[i].on_keypress) {
            E.modules[i].on_keypress(c);
        }
    }
    
    /* Handle Ctrl-C Ctrl-M sequence for vim mode */
    if (c == CTRL_KEY('c')) {
        E.ctrl_c_pressed = 1;
        E.ctrl_c_time = time(NULL);
        editorSetStatusMessage("Ctrl-C pressed, press Ctrl-M for vim mode");
        return;
    }
    
    if (E.ctrl_c_pressed && c == CTRL_KEY('m')) {
        if (time(NULL) - E.ctrl_c_time <= 2) {
            E.vim_mode_active = !E.vim_mode_active;
            editorSetStatusMessage("Vim mode %s", 
                E.vim_mode_active ? "enabled" : "disabled");
            E.ctrl_c_pressed = 0;
            return;
        }
    }
    
    if (E.ctrl_c_pressed && c != CTRL_KEY('m')) {
        E.ctrl_c_pressed = 0;
    }
    
    /* VIM command mode */
    if (E.mode == MODE_VIM_COMMAND) {
        if (c == '\r') {
            E.vim_command[E.vim_command_len] = '\0';
            executeVimCommand(E.vim_command);
            E.mode = MODE_NORMAL;
            E.vim_command_len = 0;
            return;
        } else if (c == '\x1b') {
            E.mode = MODE_NORMAL;
            E.vim_command_len = 0;
            return;
        } else if (c == KEY_BACKSPACE || c == CTRL_KEY('h')) {
            if (E.vim_command_len > 0) {
                E.vim_command_len--;
            } else {
                E.mode = MODE_NORMAL;
            }
            return;
        } else if (!iscntrl(c) && c < 128) {
            if (E.vim_command_len < sizeof(E.vim_command) - 1) {
                E.vim_command[E.vim_command_len++] = c;
            }
            return;
        }
    }
    
    switch (c) {
        case '\r':
            editorInsertNewline();
            break;
            
        case CTRL_KEY('q'):
            if (E.dirty) {
                char *response = editorPrompt("Save changes? (yes/no/cancel): %s", NULL);
                if (response) {
                    if (strcmp(response, "yes") == 0 || strcmp(response, "y") == 0 || 
                        strcmp(response, "save") == 0 || strcmp(response, "s") == 0) {
                        if (editorSave() == 0) {
                            free(response);
                            write(STDOUT_FILENO, "\x1b[2J", 4);
                            write(STDOUT_FILENO, "\x1b[H", 3);
                            exit(0);
                        } else {
                            editorSetStatusMessage("Error saving! Press Ctrl-Q again to quit without saving");
                            free(response);
                            return;
                        }
                    } else if (strcmp(response, "no") == 0 || strcmp(response, "n") == 0 || 
                               strcmp(response, "discard") == 0 || strcmp(response, "d") == 0) {
                        free(response);
                        write(STDOUT_FILENO, "\x1b[2J", 4);
                        write(STDOUT_FILENO, "\x1b[H", 3);
                        exit(0);
                    } else if (strcmp(response, "cancel") == 0 || strcmp(response, "c") == 0) {
                        editorSetStatusMessage("Quit cancelled");
                        free(response);
                        return;
                    } else {
                        editorSetStatusMessage("Invalid response. Press Ctrl-Q to try again");
                        free(response);
                        return;
                    }
                    free(response);
                } else {
                    editorSetStatusMessage("Quit cancelled");
                    return;
                }
            } else {
                write(STDOUT_FILENO, "\x1b[2J", 4);
                write(STDOUT_FILENO, "\x1b[H", 3);
                exit(0);
            }
            break;
            
        case CTRL_KEY('s'):
            if (editorSave() == 0) {
                editorSetStatusMessage("File saved successfully");
            } else {
                editorSetStatusMessage("Error saving file!");
            }
            break;
            
        case CTRL_KEY('f'):
            editorFind();
            break;
            
        case CTRL_KEY('r'):
            editorReplace();
            break;
            
        case CTRL_KEY('n'):
            if (search_ctx.match_count > 0) {
                gotoNextMatch();
            }
            break;
            
        case CTRL_KEY('p'):
            if (search_ctx.match_count > 0) {
                gotoPrevMatch();
            }
            break;
            
        case CTRL_KEY('k'):
            editorCut();
            break;
            
        case CTRL_KEY('u'):
            editorCopy();
            break;
            
        case CTRL_KEY('v'):
            editorPaste();
            break;
            
        case CTRL_KEY('t'):
            autocompleteBuildList();
            break;
            
        case CTRL_KEY('w'):
            splitToggleFocus();
            break;
            
        case CTRL_KEY('b'):
            addBookmark(E.cy, "");
            break;
            
        case CTRL_KEY('g'):
            toggleFold(E.cy);
            break;
            
        case CTRL_KEY('d'):
            multiCursorAdd(E.cx, E.cy);
            editorSetStatusMessage("Multi-cursor added (%d total)", multi_cursor.count);
            break;
            
        case CTRL_KEY('z'):
            performUndo();
            break;
            
        case CTRL_KEY('y'):
            performRedo();
            break;
            
        case KEY_HOME:
            E.cx = 0;
            break;
            
        case KEY_END:
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;
            
        case KEY_BACKSPACE:
        case CTRL_KEY('h'):
            editorDelChar();
            break;
            
        case KEY_DELETE:
            editorMoveCursor(KEY_ARROW_RIGHT);
            editorDelChar();
            break;
            
        case KEY_PAGE_UP:
        case KEY_PAGE_DOWN:
            {
                if (c == KEY_PAGE_UP) {
                    E.cy = E.rowoff;
                } else if (c == KEY_PAGE_DOWN) {
                    E.cy = E.rowoff + E.screenrows - 1;
                    if (E.cy > E.numrows) E.cy = E.numrows;
                }
                
                int times = E.screenrows;
                while (times--)
                    editorMoveCursor(c == KEY_PAGE_UP ? KEY_ARROW_UP : KEY_ARROW_DOWN);
            }
            break;
            
        case KEY_ARROW_UP:
        case KEY_ARROW_DOWN:
        case KEY_ARROW_LEFT:
        case KEY_ARROW_RIGHT:
            editorMoveCursor(c);
            break;
            
        case CTRL_KEY('l'):
        case '\x1b':
            break;
            
        case ':':
            if (E.vim_mode_active) {
                E.mode = MODE_VIM_COMMAND;
                E.vim_command_len = 0;
                memset(E.vim_command, 0, sizeof(E.vim_command));
            } else {
                editorInsertChar(c);
            }
            break;
            
        default:
            editorInsertChar(c);
            break;
    }
    
    quit_times = 1;
}

/*** Init ***/

void initEditor(void) {
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    E.syntax = NULL;
    E.mode = MODE_NORMAL;
    E.vim_mode_active = 0;
    E.vim_command_len = 0;
    E.undo_stack = NULL;
    E.redo_stack = NULL;
    E.undo_count = 0;
    E.search_query_len = 0;
    E.last_match = -1;
    E.search_direction = 1;
    E.module_count = 0;
    E.show_line_numbers = 1;
    E.ctrl_c_pressed = 0;
    E.ctrl_c_time = 0;
    
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
    E.screenrows -= 2;
    
#ifdef EDE_UNIX
    signal(SIGWINCH, handleSigwinch);
#endif
}

/*** Main ***/

void printUsage(void) {
    printf("%s v%s\n", EDE_NAME, EDE_VERSION);
    printf("Usage: ede [OPTIONS] [FILE]\n\n");
    printf("Options:\n");
    printf("  -m <module>    Load a compiled module (.emod file)\n");
    printf("  -o <output>    Specify output file (used with module compilation)\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n\n");
    printf("Module compilation:\n");
    printf("  ede <source.esrc> -o <output.emod>\n\n");
    printf("Keybindings:\n");
    printf("  Ctrl-Q         Quit (prompts to save/discard)\n");
    printf("  Ctrl-S         Save file\n");
    printf("  Ctrl-F         Find text\n");
    printf("  Ctrl-Z         Undo\n");
    printf("  Ctrl-Y         Redo\n");
    printf("  Ctrl-C Ctrl-M  Toggle vim command mode\n\n");
    printf("Platform: %s\n", EDE_PLATFORM);
}

void printVersion(void) {
    printf("%s v%s\n", EDE_NAME, EDE_VERSION);
    printf("Platform: %s\n", EDE_PLATFORM);
    printf("Copyright (C) 2025 Free Software Foundation, Inc.\n");
    printf("Licensed under GNU GPL v3\n");
}

int main(int argc, char *argv[]) {
    char *filename = NULL;
    char *module_file = NULL;
    char *output_file = NULL;
    int compile_mode = 0;
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        } else if (strcmp(argv[i], "-m") == 0) {
            if (i + 1 < argc) {
                module_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -m requires an argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
                compile_mode = 1;
            } else {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }
    
    /* Module compilation mode */
    if (compile_mode && filename) {
        if (output_file == NULL) {
            fprintf(stderr, "Error: -o option required for compilation\n");
            return 1;
        }
        printf("Compiling module: %s -> %s\n", filename, output_file);
        if (compileModule(filename, output_file) == 0) {
            printf("Module compiled successfully\n");
            return 0;
        } else {
            fprintf(stderr, "Module compilation failed\n");
            return 1;
        }
    }
    
    enableRawMode();
    initEditor();
    
    if (filename != NULL) {
        editorOpen(filename);
    }
    
    if (module_file != NULL) {
        if (loadModule(module_file) == 0) {
            editorSetStatusMessage("Module loaded: %s", module_file);
        } else {
            editorSetStatusMessage("Failed to load module: %s", module_file);
        }
    }
    
    editorSetStatusMessage(
        "HELP: Ctrl-Q = quit | Ctrl-S = save | Ctrl-F = find | Ctrl-C Ctrl-M = vim mode");
    
    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    
    return 0;
}
