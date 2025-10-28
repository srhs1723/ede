/* 
 * GNU ede v1.0 - Advanced modular text editor
 * Copyright (C) 2025 Free Software Foundation, Inc.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#define EDE_VERSION "1.0"
#define MAX_LINE_LENGTH 4096
#define MAX_FILES_PER_TAB 16
#define MAX_TABS 32
#define MAX_MODULES 16
#define INITIAL_BUFFER_SIZE 1024
#define MAX_UNDO_LEVELS 100
#define MAX_SEARCH_LENGTH 256
#define MAX_CLIPBOARD_SIZE (1024 * 1024)
#define SYNTAX_HIGHLIGHT_TYPES 32
#define MAX_BOOKMARKS 100
#define MAX_MACROS 50
#define MAX_MACRO_KEYS 1000

/* Editor modes */
typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND,
    MODE_SEARCH,
    MODE_REPLACE,
    MODE_EXIT_CONFIRM,
    MODE_VISUAL,
    MODE_VISUAL_LINE,
    MODE_VISUAL_BLOCK
} EditorMode;

/* Undo/Redo operation types */
typedef enum {
    OP_INSERT,
    OP_DELETE,
    OP_REPLACE
} OperationType;

/* Syntax token types */
typedef enum {
    STOKEN_NORMAL,
    STOKEN_KEYWORD,
    STOKEN_STRING,
    STOKEN_COMMENT,
    STOKEN_NUMBER,
    STOKEN_OPERATOR,
    STOKEN_PREPROCESSOR,
    STOKEN_FUNCTION,
    STOKEN_TYPE
} SyntaxTokenType;

/* Search direction */
typedef enum {
    SEARCH_FORWARD,
    SEARCH_BACKWARD
} SearchDirection;

/* File type */
typedef enum {
    FILETYPE_UNKNOWN,
    FILETYPE_C,
    FILETYPE_CPP,
    FILETYPE_PYTHON,
    FILETYPE_JAVA,
    FILETYPE_JAVASCRIPT,
    FILETYPE_HTML,
    FILETYPE_CSS,
    FILETYPE_JSON,
    FILETYPE_XML,
    FILETYPE_MARKDOWN
} FileType;

/* Undo/Redo entry */
typedef struct UndoEntry {
    OperationType type;
    size_t position;
    char *data;
    size_t data_len;
    struct UndoEntry *next;
    struct UndoEntry *prev;
} UndoEntry;

/* Text buffer using gap buffer */
typedef struct {
    char *content;
    size_t gap_start;
    size_t gap_end;
    size_t buffer_size;
    UndoEntry *undo_stack;
    UndoEntry *redo_stack;
    int undo_count;
    int redo_count;
} GapBuffer;

/* Syntax highlighting rule */
typedef struct {
    char *pattern;
    SyntaxTokenType type;
    int color;
} SyntaxRule;

/* Bookmark */
typedef struct {
    int line;
    char description[128];
    bool active;
} Bookmark;

/* Macro recording */
typedef struct {
    int keys[MAX_MACRO_KEYS];
    int key_count;
    bool recording;
} Macro;

/* Search state */
typedef struct {
    char pattern[MAX_SEARCH_LENGTH];
    char replace_text[MAX_SEARCH_LENGTH];
    SearchDirection direction;
    bool case_sensitive;
    bool regex_enabled;
    bool whole_word;
    int match_count;
    int current_match;
} SearchState;

/* Clipboard */
typedef struct {
    char *content;
    size_t size;
    bool is_line_mode;
} Clipboard;

/* Selection */
typedef struct {
    int start_row;
    int start_col;
    int end_row;
    int end_col;
    bool active;
} Selection;

/* File info */
typedef struct {
    char *filepath;
    GapBuffer *buffer;
    bool modified;
    int cursor_row;
    int cursor_col;
    int row_offset;
    int col_offset;
    FileType filetype;
    SyntaxRule *syntax_rules;
    int syntax_rule_count;
    Bookmark bookmarks[MAX_BOOKMARKS];
    int bookmark_count;
    Selection selection;
    SearchState search;
    time_t last_modified;
    int line_count;
    bool readonly;
} FileBuffer;

/* Tab structure */
typedef struct {
    char name[256];
    FileBuffer *files[MAX_FILES_PER_TAB];
    int file_count;
    int active_file;
    bool split_view;
    int split_ratio;
} Tab;

/* Module API - what modules can access */
typedef struct {
    /* Editor state queries */
    int (*get_cursor_row)(void);
    int (*get_cursor_col)(void);
    const char* (*get_current_file)(void);
    const char* (*get_current_line)(void);
    int (*get_line_count)(void);
    
    /* Buffer manipulation */
    void (*insert_text)(const char *text);
    void (*delete_range)(int start_row, int start_col, int end_row, int end_col);
    void (*replace_text)(const char *old_text, const char *new_text);
    
    /* UI functions */
    void (*set_status)(const char *message);
    void (*show_message)(const char *title, const char *message);
    int (*prompt)(const char *question, char *buffer, int max_len);
    
    /* File operations */
    bool (*save_file)(void);
    bool (*load_file)(const char *path);
    
    /* Syntax highlighting */
    void (*add_syntax_rule)(const char *pattern, int token_type, int color);
    void (*remove_syntax_rule)(const char *pattern);
    
    /* Utilities */
    void (*log)(const char *message);
    void* (*allocate)(size_t size);
    void (*deallocate)(void *ptr);
} ModuleAPI;

/* Module structure */
typedef struct {
    char name[256];
    char version[32];
    char author[128];
    char description[512];
    HMODULE handle;
    bool enabled;
    
    /* Module lifecycle */
    int (*init)(ModuleAPI *api);
    void (*cleanup)(void);
    
    /* Event hooks */
    void (*on_key)(int key);
    void (*on_save)(const char *filepath);
    void (*on_load)(const char *filepath);
    void (*on_cursor_move)(int row, int col);
    void (*on_mode_change)(int old_mode, int new_mode);
    void (*on_text_insert)(const char *text);
    void (*on_text_delete)(int start, int end);
    void (*on_tab_switch)(int old_tab, int new_tab);
    
    /* Custom commands */
    int (*execute_command)(const char *command, const char *args);
    
    /* Configuration */
    void (*set_config)(const char *key, const char *value);
    const char* (*get_config)(const char *key);
} Module;

/* Configuration */
typedef struct {
    bool show_line_numbers;
    bool syntax_highlighting;
    bool auto_indent;
    bool show_whitespace;
    int tab_width;
    bool use_spaces;
    bool word_wrap;
    bool show_status_bar;
    bool show_ruler;
    int undo_levels;
    bool backup_files;
    char backup_dir[MAX_PATH];
    int autosave_interval;
    bool highlight_current_line;
    bool show_matching_bracket;
    int scroll_margin;
} Config;

/* Command history */
typedef struct {
    char commands[100][256];
    int count;
    int current;
} CommandHistory;

/* Main editor state */
typedef struct {
    Tab tabs[MAX_TABS];
    int tab_count;
    int active_tab;
    EditorMode mode;
    EditorMode prev_mode;
    char status_message[256];
    char command_buffer[256];
    char exit_command[64];
    Module modules[MAX_MODULES];
    int module_count;
    int screen_rows;
    int screen_cols;
    Clipboard clipboard;
    Macro macros[MAX_MACROS];
    int macro_count;
    int current_macro;
    bool recording_macro;
    Config config;
    CommandHistory cmd_history;
    char log_file[MAX_PATH];
    FILE *log_fp;
    bool running;
    int last_key;
    time_t last_activity;
} EditorState;

/* Global state */
EditorState g_state;
ModuleAPI g_module_api;

/* ==== FORWARD DECLARATIONS ==== */
void editor_set_status_message(const char *fmt, ...);
void log_message(const char *fmt, ...);
FileBuffer* get_current_buffer(void);
void update_line_count(FileBuffer *fb);
void apply_syntax_highlighting(FileBuffer *fb);
char* buffer_get_line(GapBuffer *buf, int line_num, int *line_len);
void buffer_insert_char(GapBuffer *buf, char c, size_t pos);
void buffer_delete_char(GapBuffer *buf, size_t pos);
void buffer_save_file(GapBuffer *buf, const char *filepath);

/* ==== UTILITY FUNCTIONS ==== */

char* str_duplicate(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (dup) strcpy(dup, str);
    return dup;
}

int ede_min(int a, int b) {
    return a < b ? a : b;
}

int ede_max(int a, int b) {
    return a > b ? a : b;
}

void log_message(const char *fmt, ...) {
    if (!g_state.log_fp) return;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(g_state.log_fp, "[%s] ", time_str);
    
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_state.log_fp, fmt, ap);
    va_end(ap);
    
    fprintf(g_state.log_fp, "\n");
    fflush(g_state.log_fp);
}

FileType detect_filetype(const char *filepath) {
    if (!filepath) return FILETYPE_UNKNOWN;
    
    const char *ext = strrchr(filepath, '.');
    if (!ext) return FILETYPE_UNKNOWN;
    
    ext++;
    
    if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0) return FILETYPE_C;
    if (strcmp(ext, "cpp") == 0 || strcmp(ext, "hpp") == 0 || strcmp(ext, "cc") == 0) return FILETYPE_CPP;
    if (strcmp(ext, "py") == 0) return FILETYPE_PYTHON;
    if (strcmp(ext, "java") == 0) return FILETYPE_JAVA;
    if (strcmp(ext, "js") == 0) return FILETYPE_JAVASCRIPT;
    if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) return FILETYPE_HTML;
    if (strcmp(ext, "css") == 0) return FILETYPE_CSS;
    if (strcmp(ext, "json") == 0) return FILETYPE_JSON;
    if (strcmp(ext, "xml") == 0) return FILETYPE_XML;
    if (strcmp(ext, "md") == 0 || strcmp(ext, "markdown") == 0) return FILETYPE_MARKDOWN;
    
    return FILETYPE_UNKNOWN;
}

const char* get_filetype_name(FileType type) {
    switch (type) {
        case FILETYPE_C: return "C";
        case FILETYPE_CPP: return "C++";
        case FILETYPE_PYTHON: return "Python";
        case FILETYPE_JAVA: return "Java";
        case FILETYPE_JAVASCRIPT: return "JavaScript";
        case FILETYPE_HTML: return "HTML";
        case FILETYPE_CSS: return "CSS";
        case FILETYPE_JSON: return "JSON";
        case FILETYPE_XML: return "XML";
        case FILETYPE_MARKDOWN: return "Markdown";
        default: return "Unknown";
    }
}

void setup_syntax_rules(FileBuffer *fb) {
    /* C/C++ keywords */
    if (fb->filetype == FILETYPE_C || fb->filetype == FILETYPE_CPP) {
        static const char *c_keywords[] = {
            "if", "else", "while", "for", "do", "switch", "case", "default",
            "break", "continue", "return", "goto", "sizeof", "typedef",
            "struct", "union", "enum", "const", "static", "extern",
            "void", "char", "short", "int", "long", "float", "double",
            "signed", "unsigned", "bool", "true", "false", NULL
        };
        
        fb->syntax_rule_count = 0;
        for (int i = 0; c_keywords[i] != NULL; i++) {
            /* Would normally allocate and store rules */
        }
    }
}

/* ==== UNDO/REDO SYSTEM ==== */

UndoEntry* undo_create_entry(OperationType type, size_t position, const char *data, size_t data_len) {
    UndoEntry *entry = malloc(sizeof(UndoEntry));
    entry->type = type;
    entry->position = position;
    entry->data = malloc(data_len + 1);
    memcpy(entry->data, data, data_len);
    entry->data[data_len] = '\0';
    entry->data_len = data_len;
    entry->next = NULL;
    entry->prev = NULL;
    return entry;
}

void undo_push(GapBuffer *buf, OperationType type, size_t position, const char *data, size_t data_len) {
    if (buf->undo_count >= MAX_UNDO_LEVELS) {
        /* Remove oldest undo entry */
        UndoEntry *oldest = buf->undo_stack;
        while (oldest && oldest->next) oldest = oldest->next;
        if (oldest) {
            if (oldest->prev) oldest->prev->next = NULL;
            free(oldest->data);
            free(oldest);
            buf->undo_count--;
        }
    }
    
    UndoEntry *entry = undo_create_entry(type, position, data, data_len);
    entry->next = buf->undo_stack;
    if (buf->undo_stack) buf->undo_stack->prev = entry;
    buf->undo_stack = entry;
    buf->undo_count++;
    
    /* Clear redo stack */
    while (buf->redo_stack) {
        UndoEntry *next = buf->redo_stack->next;
        free(buf->redo_stack->data);
        free(buf->redo_stack);
        buf->redo_stack = next;
    }
    buf->redo_count = 0;
}

void undo_perform(GapBuffer *buf) {
    if (!buf->undo_stack) return;
    
    UndoEntry *entry = buf->undo_stack;
    buf->undo_stack = entry->next;
    if (buf->undo_stack) buf->undo_stack->prev = NULL;
    buf->undo_count--;
    
    /* Apply undo operation */
    if (entry->type == OP_INSERT) {
        /* Delete the inserted text */
        for (size_t i = 0; i < entry->data_len; i++) {
            buffer_delete_char(buf, entry->position);
        }
    } else if (entry->type == OP_DELETE) {
        /* Re-insert the deleted text */
        for (size_t i = 0; i < entry->data_len; i++) {
            buffer_insert_char(buf, entry->data[i], entry->position + i);
        }
    }
    
    /* Move to redo stack */
    entry->next = buf->redo_stack;
    if (buf->redo_stack) buf->redo_stack->prev = entry;
    buf->redo_stack = entry;
    buf->redo_count++;
}

void redo_perform(GapBuffer *buf) {
    if (!buf->redo_stack) return;
    
    UndoEntry *entry = buf->redo_stack;
    buf->redo_stack = entry->next;
    if (buf->redo_stack) buf->redo_stack->prev = NULL;
    buf->redo_count--;
    
    /* Apply redo operation */
    if (entry->type == OP_INSERT) {
        /* Re-insert the text */
        for (size_t i = 0; i < entry->data_len; i++) {
            buffer_insert_char(buf, entry->data[i], entry->position + i);
        }
    } else if (entry->type == OP_DELETE) {
        /* Delete again */
        for (size_t i = 0; i < entry->data_len; i++) {
            buffer_delete_char(buf, entry->position);
        }
    }
    
    /* Move back to undo stack */
    entry->next = buf->undo_stack;
    if (buf->undo_stack) buf->undo_stack->prev = entry;
    buf->undo_stack = entry;
    buf->undo_count++;
}

/* ==== CLIPBOARD OPERATIONS ==== */

void clipboard_init(void) {
    g_state.clipboard.content = NULL;
    g_state.clipboard.size = 0;
    g_state.clipboard.is_line_mode = false;
}

void clipboard_set(const char *text, size_t len, bool line_mode) {
    if (g_state.clipboard.content) {
        free(g_state.clipboard.content);
    }
    
    g_state.clipboard.content = malloc(len + 1);
    memcpy(g_state.clipboard.content, text, len);
    g_state.clipboard.content[len] = '\0';
    g_state.clipboard.size = len;
    g_state.clipboard.is_line_mode = line_mode;
    
    log_message("Clipboard set: %zu bytes", len);
}

const char* clipboard_get(size_t *len, bool *line_mode) {
    if (len) *len = g_state.clipboard.size;
    if (line_mode) *line_mode = g_state.clipboard.is_line_mode;
    return g_state.clipboard.content;
}

void clipboard_free(void) {
    if (g_state.clipboard.content) {
        free(g_state.clipboard.content);
        g_state.clipboard.content = NULL;
        g_state.clipboard.size = 0;
    }
}

/* ==== SEARCH OPERATIONS ==== */

void search_init(FileBuffer *fb) {
    fb->search.pattern[0] = '\0';
    fb->search.replace_text[0] = '\0';
    fb->search.direction = SEARCH_FORWARD;
    fb->search.case_sensitive = false;
    fb->search.regex_enabled = false;
    fb->search.whole_word = false;
    fb->search.match_count = 0;
    fb->search.current_match = 0;
}

bool search_match(const char *text, const char *pattern, bool case_sensitive) {
    if (case_sensitive) {
        return strstr(text, pattern) != NULL;
    } else {
        /* Case-insensitive search */
        char *text_lower = str_duplicate(text);
        char *pattern_lower = str_duplicate(pattern);
        
        for (char *p = text_lower; *p; p++) *p = tolower(*p);
        for (char *p = pattern_lower; *p; p++) *p = tolower(*p);
        
        bool found = strstr(text_lower, pattern_lower) != NULL;
        
        free(text_lower);
        free(pattern_lower);
        
        return found;
    }
}

int search_find_next(FileBuffer *fb) {
    if (fb->search.pattern[0] == '\0') return -1;
    
    int start_row = fb->cursor_row;
    int start_col = fb->cursor_col + 1;
    
    for (int row = start_row; row < fb->line_count; row++) {
        int line_len;
        char *line = buffer_get_line(fb->buffer, row, &line_len);
        if (!line) continue;
        
        for (int col = (row == start_row ? start_col : 0); col < line_len; col++) {
            if (search_match(&line[col], fb->search.pattern, fb->search.case_sensitive)) {
                fb->cursor_row = row;
                fb->cursor_col = col;
                fb->search.current_match++;
                return 1;
            }
        }
    }
    
    return 0;
}

int search_find_prev(FileBuffer *fb) {
    if (fb->search.pattern[0] == '\0') return -1;
    
    int start_row = fb->cursor_row;
    int start_col = fb->cursor_col - 1;
    
    for (int row = start_row; row >= 0; row--) {
        int line_len;
        char *line = buffer_get_line(fb->buffer, row, &line_len);
        if (!line) continue;
        
        for (int col = (row == start_row ? start_col : line_len - 1); col >= 0; col--) {
            if (search_match(&line[col], fb->search.pattern, fb->search.case_sensitive)) {
                fb->cursor_row = row;
                fb->cursor_col = col;
                fb->search.current_match--;
                return 1;
            }
        }
    }
    
    return 0;
}

void search_replace_current(FileBuffer *fb) {
    if (fb->search.pattern[0] == '\0' || fb->search.replace_text[0] == '\0') return;
    
    int line_len;
    char *line = buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
    if (!line) return;
    
    /* Delete pattern */
    size_t pattern_len = strlen(fb->search.pattern);
    size_t pos = fb->cursor_row * MAX_LINE_LENGTH + fb->cursor_col;
    for (size_t i = 0; i < pattern_len; i++) {
        buffer_delete_char(fb->buffer, pos);
    }
    
    /* Insert replacement */
    size_t replace_len = strlen(fb->search.replace_text);
    for (size_t i = 0; i < replace_len; i++) {
        buffer_insert_char(fb->buffer, fb->search.replace_text[i], pos + i);
    }
    
    fb->modified = true;
    log_message("Replaced '%s' with '%s'", fb->search.pattern, fb->search.replace_text);
}

int search_replace_all(FileBuffer *fb) {
    int count = 0;
    fb->cursor_row = 0;
    fb->cursor_col = 0;
    
    while (search_find_next(fb) > 0) {
        search_replace_current(fb);
        count++;
    }
    
    return count;
}

/* ==== BOOKMARK OPERATIONS ==== */

void bookmark_toggle(FileBuffer *fb, int line) {
    /* Check if bookmark exists */
    for (int i = 0; i < fb->bookmark_count; i++) {
        if (fb->bookmarks[i].line == line && fb->bookmarks[i].active) {
            fb->bookmarks[i].active = false;
            log_message("Bookmark removed at line %d", line);
            return;
        }
    }
    
    /* Add new bookmark */
    if (fb->bookmark_count < MAX_BOOKMARKS) {
        fb->bookmarks[fb->bookmark_count].line = line;
        fb->bookmarks[fb->bookmark_count].active = true;
        snprintf(fb->bookmarks[fb->bookmark_count].description, 128, "Line %d", line);
        fb->bookmark_count++;
        log_message("Bookmark added at line %d", line);
    }
}

int bookmark_next(FileBuffer *fb) {
    int closest = -1;
    int min_distance = INT_MAX;
    
    for (int i = 0; i < fb->bookmark_count; i++) {
        if (!fb->bookmarks[i].active) continue;
        
        int distance = fb->bookmarks[i].line - fb->cursor_row;
        if (distance > 0 && distance < min_distance) {
            min_distance = distance;
            closest = fb->bookmarks[i].line;
        }
    }
    
    return closest;
}

int bookmark_prev(FileBuffer *fb) {
    int closest = -1;
    int min_distance = INT_MAX;
    
    for (int i = 0; i < fb->bookmark_count; i++) {
        if (!fb->bookmarks[i].active) continue;
        
        int distance = fb->cursor_row - fb->bookmarks[i].line;
        if (distance > 0 && distance < min_distance) {
            min_distance = distance;
            closest = fb->bookmarks[i].line;
        }
    }
    
    return closest;
}

/* ==== MACRO SYSTEM ==== */

void macro_start_recording(int slot) {
    if (slot < 0 || slot >= MAX_MACROS) return;
    
    g_state.current_macro = slot;
    g_state.macros[slot].key_count = 0;
    g_state.macros[slot].recording = true;
    g_state.recording_macro = true;
    
    editor_set_status_message("Recording macro %d...", slot);
    log_message("Started recording macro %d", slot);
}

void macro_stop_recording(void) {
    if (!g_state.recording_macro) return;
    
    g_state.macros[g_state.current_macro].recording = false;
    g_state.recording_macro = false;
    
    editor_set_status_message("Macro %d recorded (%d keys)", 
        g_state.current_macro, 
        g_state.macros[g_state.current_macro].key_count);
    log_message("Stopped recording macro %d", g_state.current_macro);
}

void macro_record_key(int key) {
    if (!g_state.recording_macro) return;
    
    Macro *macro = &g_state.macros[g_state.current_macro];
    if (macro->key_count < MAX_MACRO_KEYS) {
        macro->keys[macro->key_count++] = key;
    }
}

void macro_play(int slot) {
    if (slot < 0 || slot >= MAX_MACROS) return;
    if (g_state.macros[slot].key_count == 0) return;
    
    editor_set_status_message("Playing macro %d...", slot);
    log_message("Playing macro %d", slot);
    
    /* Would replay recorded keys */
    for (int i = 0; i < g_state.macros[slot].key_count; i++) {
        /* Process each recorded key */
        /* editor_process_key(g_state.macros[slot].keys[i]); */
    }
}

/* ==== BUFFER FUNCTIONS ==== */

GapBuffer *buffer_create(void) {
    GapBuffer *buf = malloc(sizeof(GapBuffer));
    buf->buffer_size = INITIAL_BUFFER_SIZE;
    buf->content = malloc(buf->buffer_size);
    buf->gap_start = 0;
    buf->gap_end = buf->buffer_size;
    return buf;
}

void buffer_grow(GapBuffer *buf, size_t min_size) {
    size_t new_size = buf->buffer_size * 2;
    while (new_size < min_size) new_size *= 2;
    
    char *new_content = malloc(new_size);
    memcpy(new_content, buf->content, buf->gap_start);
    size_t gap_size = buf->gap_end - buf->gap_start;
    buf->gap_end = new_size - (buf->buffer_size - buf->gap_end);
    memcpy(new_content + buf->gap_end, buf->content + buf->buffer_size - (buf->buffer_size - buf->gap_end), 
           buf->buffer_size - buf->gap_end);
    
    free(buf->content);
    buf->content = new_content;
    buf->buffer_size = new_size;
}

void buffer_insert_char(GapBuffer *buf, char c, size_t pos) {
    if (buf->gap_end - buf->gap_start == 0) {
        buffer_grow(buf, buf->buffer_size + 1);
    }
    
    if (pos < buf->gap_start) {
        memmove(buf->content + pos + 1, buf->content + pos, buf->gap_start - pos);
        buf->gap_start = pos + 1;
    } else if (pos > buf->gap_start) {
        size_t actual_pos = pos + (buf->gap_end - buf->gap_start);
        memmove(buf->content + buf->gap_end, buf->content + actual_pos, pos - buf->gap_start);
        buf->gap_end += pos - buf->gap_start;
        buf->gap_start = pos;
    }
    
    buf->content[buf->gap_start++] = c;
}

void buffer_delete_char(GapBuffer *buf, size_t pos) {
    if (pos >= buf->gap_start) {
        size_t actual_pos = pos + (buf->gap_end - buf->gap_start);
        if (actual_pos < buf->buffer_size) {
            buf->gap_end++;
        }
    } else if (pos < buf->gap_start && pos >= 0) {
        buf->gap_start--;
    }
}

void buffer_load_file(GapBuffer *buf, const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size > buf->buffer_size) {
        buffer_grow(buf, size);
    }
    
    fread(buf->content, 1, size, f);
    buf->gap_start = size;
    fclose(f);
}

void buffer_save_file(GapBuffer *buf, const char *filepath) {
    FILE *f = fopen(filepath, "wb");
    if (!f) return;
    
    fwrite(buf->content, 1, buf->gap_start, f);
    fwrite(buf->content + buf->gap_end, 1, buf->buffer_size - buf->gap_end, f);
    fclose(f);
}

void buffer_free(GapBuffer *buf) {
    free(buf->content);
    free(buf);
}

char *buffer_get_line(GapBuffer *buf, int line_num, int *line_len) {
    static char line_buffer[MAX_LINE_LENGTH];
    int current_line = 0;
    size_t pos = 0;
    int col = 0;
    
    while (pos < buf->gap_start || pos >= buf->gap_end) {
        char c;
        if (pos < buf->gap_start) {
            c = buf->content[pos];
        } else if (pos >= buf->gap_end && pos < buf->buffer_size) {
            c = buf->content[pos];
        } else {
            break;
        }
        
        if (current_line == line_num) {
            if (c == '\n') {
                line_buffer[col] = '\0';
                *line_len = col;
                return line_buffer;
            }
            line_buffer[col++] = c;
            if (col >= MAX_LINE_LENGTH - 1) break;
        } else if (c == '\n') {
            current_line++;
        }
        
        pos++;
        if (pos == buf->gap_start) pos = buf->gap_end;
    }
    
    if (current_line == line_num) {
        line_buffer[col] = '\0';
        *line_len = col;
        return line_buffer;
    }
    
    *line_len = 0;
    return NULL;
}

/* ==== TAB FUNCTIONS ==== */

void tab_init(Tab *tab, const char *name) {
    strncpy(tab->name, name, sizeof(tab->name) - 1);
    tab->file_count = 0;
    tab->active_file = 0;
    memset(tab->files, 0, sizeof(tab->files));
}

void tab_add_file(Tab *tab, const char *filepath) {
    if (tab->file_count >= MAX_FILES_PER_TAB) return;
    
    FileBuffer *fb = malloc(sizeof(FileBuffer));
    fb->filepath = filepath ? strdup(filepath) : NULL;
    fb->buffer = buffer_create();
    fb->modified = false;
    fb->cursor_row = 0;
    fb->cursor_col = 0;
    fb->row_offset = 0;
    fb->col_offset = 0;
    
    if (filepath) {
        buffer_load_file(fb->buffer, filepath);
    }
    
    tab->files[tab->file_count++] = fb;
}

void tab_close_file(Tab *tab, int file_idx) {
    if (file_idx < 0 || file_idx >= tab->file_count) return;
    
    FileBuffer *fb = tab->files[file_idx];
    if (fb->filepath) free(fb->filepath);
    buffer_free(fb->buffer);
    free(fb);
    
    for (int i = file_idx; i < tab->file_count - 1; i++) {
        tab->files[i] = tab->files[i + 1];
    }
    tab->file_count--;
}

/* ==== MODULE API IMPLEMENTATIONS ==== */

/* API function: get_cursor_row */
int api_get_cursor_row(void) {
    FileBuffer *fb = get_current_buffer();
    return fb ? fb->cursor_row : 0;
}

/* API function: get_cursor_col */
int api_get_cursor_col(void) {
    FileBuffer *fb = get_current_buffer();
    return fb ? fb->cursor_col : 0;
}

/* API function: get_current_file */
const char* api_get_current_file(void) {
    FileBuffer *fb = get_current_buffer();
    return fb ? fb->filepath : NULL;
}

/* API function: get_current_line */
const char* api_get_current_line(void) {
    FileBuffer *fb = get_current_buffer();
    if (!fb) return NULL;
    
    int line_len;
    return buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
}

/* API function: get_line_count */
int api_get_line_count(void) {
    FileBuffer *fb = get_current_buffer();
    return fb ? fb->line_count : 0;
}

/* API function: insert_text */
void api_insert_text(const char *text) {
    FileBuffer *fb = get_current_buffer();
    if (!fb || !text) return;
    
    size_t pos = fb->cursor_row * MAX_LINE_LENGTH + fb->cursor_col;
    for (const char *p = text; *p; p++) {
        buffer_insert_char(fb->buffer, *p, pos++);
        fb->cursor_col++;
    }
    fb->modified = true;
    log_message("Module inserted text: %s", text);
}

/* API function: delete_range */
void api_delete_range(int start_row, int start_col, int end_row, int end_col) {
    FileBuffer *fb = get_current_buffer();
    if (!fb) return;
    
    size_t start_pos = start_row * MAX_LINE_LENGTH + start_col;
    size_t end_pos = end_row * MAX_LINE_LENGTH + end_col;
    
    for (size_t pos = start_pos; pos < end_pos; pos++) {
        buffer_delete_char(fb->buffer, start_pos);
    }
    
    fb->modified = true;
    log_message("Module deleted range: (%d,%d) to (%d,%d)", start_row, start_col, end_row, end_col);
}

/* API function: replace_text */
void api_replace_text(const char *old_text, const char *new_text) {
    FileBuffer *fb = get_current_buffer();
    if (!fb) return;
    
    strncpy(fb->search.pattern, old_text, MAX_SEARCH_LENGTH - 1);
    strncpy(fb->search.replace_text, new_text, MAX_SEARCH_LENGTH - 1);
    
    int count = search_replace_all(fb);
    editor_set_status_message("Replaced %d occurrences", count);
    log_message("Module replaced '%s' with '%s': %d occurrences", old_text, new_text, count);
}

/* API function: set_status */
void api_set_status(const char *message) {
    editor_set_status_message("%s", message);
}

/* API function: show_message */
void api_show_message(const char *title, const char *message) {
    MessageBoxA(NULL, message, title, MB_OK | MB_ICONINFORMATION);
    log_message("Module message: [%s] %s", title, message);
}

/* API function: prompt */
int api_prompt(const char *question, char *buffer, int max_len) {
    /* Simple console prompt */
    printf("%s: ", question);
    if (fgets(buffer, max_len, stdin)) {
        /* Remove trailing newline */
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        return (int)strlen(buffer);
    }
    return 0;
}

/* API function: save_file */
bool api_save_file(void) {
    FileBuffer *fb = get_current_buffer();
    if (!fb || !fb->filepath) return false;
    
    buffer_save_file(fb->buffer, fb->filepath);
    fb->modified = false;
    log_message("Module saved file: %s", fb->filepath);
    return true;
}

/* API function: load_file */
bool api_load_file(const char *path) {
    Tab *tab = &g_state.tabs[g_state.active_tab];
    tab_add_file(tab, path);
    log_message("Module loaded file: %s", path);
    return true;
}

/* API function: add_syntax_rule */
void api_add_syntax_rule(const char *pattern, int token_type, int color) {
    FileBuffer *fb = get_current_buffer();
    if (!fb) return;
    
    log_message("Module added syntax rule: %s (type=%d, color=%d)", pattern, token_type, color);
}

/* API function: remove_syntax_rule */
void api_remove_syntax_rule(const char *pattern) {
    FileBuffer *fb = get_current_buffer();
    if (!fb) return;
    
    log_message("Module removed syntax rule: %s", pattern);
}

/* API function: log */
void api_log(const char *message) {
    log_message("[MODULE] %s", message);
}

/* API function: allocate */
void* api_allocate(size_t size) {
    void *ptr = malloc(size);
    log_message("Module allocated %zu bytes at %p", size, ptr);
    return ptr;
}

/* API function: deallocate */
void api_deallocate(void *ptr) {
    log_message("Module deallocated %p", ptr);
    free(ptr);
}

/* Initialize module API */
void module_api_init(void) {
    g_module_api.get_cursor_row = api_get_cursor_row;
    g_module_api.get_cursor_col = api_get_cursor_col;
    g_module_api.get_current_file = api_get_current_file;
    g_module_api.get_current_line = api_get_current_line;
    g_module_api.get_line_count = api_get_line_count;
    
    g_module_api.insert_text = api_insert_text;
    g_module_api.delete_range = api_delete_range;
    g_module_api.replace_text = api_replace_text;
    
    g_module_api.set_status = api_set_status;
    g_module_api.show_message = api_show_message;
    g_module_api.prompt = api_prompt;
    
    g_module_api.save_file = api_save_file;
    g_module_api.load_file = api_load_file;
    
    g_module_api.add_syntax_rule = api_add_syntax_rule;
    g_module_api.remove_syntax_rule = api_remove_syntax_rule;
    
    g_module_api.log = api_log;
    g_module_api.allocate = api_allocate;
    g_module_api.deallocate = api_deallocate;
    
    log_message("Module API initialized");
}

/* ==== MODULE FUNCTIONS ==== */

int module_load(const char *emod_path) {
    if (g_state.module_count >= MAX_MODULES) {
        log_message("Cannot load module: max modules reached");
        return -1;
    }
    
    Module *mod = &g_state.modules[g_state.module_count];
    memset(mod, 0, sizeof(Module));
    
    mod->handle = LoadLibraryA(emod_path);
    
    if (!mod->handle) {
        DWORD error = GetLastError();
        log_message("Failed to load module %s: error %lu", emod_path, error);
        return -1;
    }
    
    /* Load module metadata */
    const char* (*get_name)(void) = (const char*(*)(void))GetProcAddress(mod->handle, "get_module_name");
    const char* (*get_version)(void) = (const char*(*)(void))GetProcAddress(mod->handle, "get_module_version");
    const char* (*get_author)(void) = (const char*(*)(void))GetProcAddress(mod->handle, "get_module_author");
    const char* (*get_description)(void) = (const char*(*)(void))GetProcAddress(mod->handle, "get_module_description");
    
    if (get_name) strncpy(mod->name, get_name(), sizeof(mod->name) - 1);
    else strncpy(mod->name, emod_path, sizeof(mod->name) - 1);
    
    if (get_version) strncpy(mod->version, get_version(), sizeof(mod->version) - 1);
    else strncpy(mod->version, "1.0", sizeof(mod->version) - 1);
    
    if (get_author) strncpy(mod->author, get_author(), sizeof(mod->author) - 1);
    else strncpy(mod->author, "Unknown", sizeof(mod->author) - 1);
    
    if (get_description) strncpy(mod->description, get_description(), sizeof(mod->description) - 1);
    else strncpy(mod->description, "No description", sizeof(mod->description) - 1);
    
    /* Load function pointers */
    mod->init = (int(*)(ModuleAPI*))GetProcAddress(mod->handle, "module_init");
    mod->cleanup = (void(*)(void))GetProcAddress(mod->handle, "module_cleanup");
    mod->on_key = (void(*)(int))GetProcAddress(mod->handle, "module_on_key");
    mod->on_save = (void(*)(const char*))GetProcAddress(mod->handle, "module_on_save");
    mod->on_load = (void(*)(const char*))GetProcAddress(mod->handle, "module_on_load");
    mod->on_cursor_move = (void(*)(int, int))GetProcAddress(mod->handle, "module_on_cursor_move");
    mod->on_mode_change = (void(*)(int, int))GetProcAddress(mod->handle, "module_on_mode_change");
    mod->on_text_insert = (void(*)(const char*))GetProcAddress(mod->handle, "module_on_text_insert");
    mod->on_text_delete = (void(*)(int, int))GetProcAddress(mod->handle, "module_on_text_delete");
    mod->on_tab_switch = (void(*)(int, int))GetProcAddress(mod->handle, "module_on_tab_switch");
    mod->execute_command = (int(*)(const char*, const char*))GetProcAddress(mod->handle, "module_execute_command");
    mod->set_config = (void(*)(const char*, const char*))GetProcAddress(mod->handle, "module_set_config");
    mod->get_config = (const char*(*)(const char*))GetProcAddress(mod->handle, "module_get_config");
    
    mod->enabled = true;
    
    /* Initialize module */
    if (mod->init) {
        int result = mod->init(&g_module_api);
        if (result != 0) {
            log_message("Module %s initialization failed: %d", mod->name, result);
            FreeLibrary(mod->handle);
            return -1;
        }
    }
    
    g_state.module_count++;
    log_message("Loaded module: %s v%s by %s", mod->name, mod->version, mod->author);
    editor_set_status_message("Loaded module: %s v%s", mod->name, mod->version);
    
    return 0;
}

void module_unload(int index) {
    if (index < 0 || index >= g_state.module_count) return;
    
    Module *mod = &g_state.modules[index];
    
    if (mod->cleanup) {
        mod->cleanup();
    }
    
    if (mod->handle) {
        FreeLibrary(mod->handle);
    }
    
    log_message("Unloaded module: %s", mod->name);
    
    /* Shift remaining modules */
    for (int i = index; i < g_state.module_count - 1; i++) {
        g_state.modules[i] = g_state.modules[i + 1];
    }
    
    g_state.module_count--;
}

void module_unload_all(void) {
    for (int i = g_state.module_count - 1; i >= 0; i--) {
        module_unload(i);
    }
    log_message("All modules unloaded");
}

void module_enable(int index) {
    if (index < 0 || index >= g_state.module_count) return;
    g_state.modules[index].enabled = true;
    log_message("Enabled module: %s", g_state.modules[index].name);
}

void module_disable(int index) {
    if (index < 0 || index >= g_state.module_count) return;
    g_state.modules[index].enabled = false;
    log_message("Disabled module: %s", g_state.modules[index].name);
}

void module_trigger_on_key(int key) {
    for (int i = 0; i < g_state.module_count; i++) {
        if (g_state.modules[i].enabled && g_state.modules[i].on_key) {
            g_state.modules[i].on_key(key);
        }
    }
}

void module_trigger_on_save(const char *filepath) {
    for (int i = 0; i < g_state.module_count; i++) {
        if (g_state.modules[i].enabled && g_state.modules[i].on_save) {
            g_state.modules[i].on_save(filepath);
        }
    }
}

void module_trigger_on_load(const char *filepath) {
    for (int i = 0; i < g_state.module_count; i++) {
        if (g_state.modules[i].enabled && g_state.modules[i].on_load) {
            g_state.modules[i].on_load(filepath);
        }
    }
}

void module_trigger_on_cursor_move(int row, int col) {
    for (int i = 0; i < g_state.module_count; i++) {
        if (g_state.modules[i].enabled && g_state.modules[i].on_cursor_move) {
            g_state.modules[i].on_cursor_move(row, col);
        }
    }
}

void module_trigger_on_mode_change(int old_mode, int new_mode) {
    for (int i = 0; i < g_state.module_count; i++) {
        if (g_state.modules[i].enabled && g_state.modules[i].on_mode_change) {
            g_state.modules[i].on_mode_change(old_mode, new_mode);
        }
    }
}

/* ==== ESRC COMPILER ==== */

/* ESRC Token types */
typedef enum {
    ESRC_TOKEN_EOF,
    ESRC_TOKEN_IDENTIFIER,
    ESRC_TOKEN_NUMBER,
    ESRC_TOKEN_STRING,
    ESRC_TOKEN_KEYWORD,
    ESRC_TOKEN_OPERATOR,
    ESRC_TOKEN_LPAREN,
    ESRC_TOKEN_RPAREN,
    ESRC_TOKEN_LBRACE,
    ESRC_TOKEN_RBRACE,
    ESRC_TOKEN_SEMICOLON,
    ESRC_TOKEN_COMMA
} EsrcTokenType;

typedef struct {
    EsrcTokenType type;
    char *value;
    int line;
    int column;
} EsrcToken;

typedef struct {
    const char *source;
    size_t position;
    int line;
    int column;
    EsrcToken current_token;
} EsrcLexer;

typedef struct {
    char *name;
    char *type;
    int offset;
} EsrcVariable;

typedef struct {
    char *name;
    char *return_type;
    EsrcVariable *params;
    int param_count;
    char *body;
} EsrcFunction;

typedef struct {
    EsrcFunction *functions;
    int function_count;
    EsrcVariable *globals;
    int global_count;
    char **dependencies;
    int dependency_count;
} EsrcModule;

typedef struct {
    EsrcModule module;
    FILE *output;
    int error_count;
    char error_message[512];
} EsrcCompiler;

/* Lexer functions */
void esrc_lexer_init(EsrcLexer *lexer, const char *source) {
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->current_token.type = ESRC_TOKEN_EOF;
    lexer->current_token.value = NULL;
}

char esrc_lexer_peek(EsrcLexer *lexer) {
    if (lexer->position >= strlen(lexer->source)) return '\0';
    return lexer->source[lexer->position];
}

char esrc_lexer_advance(EsrcLexer *lexer) {
    if (lexer->position >= strlen(lexer->source)) return '\0';
    char c = lexer->source[lexer->position++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

void esrc_lexer_skip_whitespace(EsrcLexer *lexer) {
    while (isspace(esrc_lexer_peek(lexer))) {
        esrc_lexer_advance(lexer);
    }
}

void esrc_lexer_skip_comment(EsrcLexer *lexer) {
    if (esrc_lexer_peek(lexer) == '/' && lexer->source[lexer->position + 1] == '/') {
        while (esrc_lexer_peek(lexer) != '\n' && esrc_lexer_peek(lexer) != '\0') {
            esrc_lexer_advance(lexer);
        }
    }
}

EsrcToken esrc_lexer_read_identifier(EsrcLexer *lexer) {
    EsrcToken token;
    token.line = lexer->line;
    token.column = lexer->column;
    
    char buffer[256];
    int i = 0;
    
    while (isalnum(esrc_lexer_peek(lexer)) || esrc_lexer_peek(lexer) == '_') {
        if (i < 255) buffer[i++] = esrc_lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    /* Check for keywords */
    const char *keywords[] = {"function", "var", "if", "else", "while", "for", "return", "import", NULL};
    bool is_keyword = false;
    for (int j = 0; keywords[j] != NULL; j++) {
        if (strcmp(buffer, keywords[j]) == 0) {
            is_keyword = true;
            break;
        }
    }
    
    token.type = is_keyword ? ESRC_TOKEN_KEYWORD : ESRC_TOKEN_IDENTIFIER;
    token.value = str_duplicate(buffer);
    
    return token;
}

EsrcToken esrc_lexer_read_number(EsrcLexer *lexer) {
    EsrcToken token;
    token.line = lexer->line;
    token.column = lexer->column;
    token.type = ESRC_TOKEN_NUMBER;
    
    char buffer[256];
    int i = 0;
    
    while (isdigit(esrc_lexer_peek(lexer)) || esrc_lexer_peek(lexer) == '.') {
        if (i < 255) buffer[i++] = esrc_lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    token.value = str_duplicate(buffer);
    return token;
}

EsrcToken esrc_lexer_read_string(EsrcLexer *lexer) {
    EsrcToken token;
    token.line = lexer->line;
    token.column = lexer->column;
    token.type = ESRC_TOKEN_STRING;
    
    char buffer[1024];
    int i = 0;
    
    esrc_lexer_advance(lexer); /* Skip opening quote */
    
    while (esrc_lexer_peek(lexer) != '"' && esrc_lexer_peek(lexer) != '\0') {
        if (esrc_lexer_peek(lexer) == '\\') {
            esrc_lexer_advance(lexer);
            char escaped = esrc_lexer_advance(lexer);
            switch (escaped) {
                case 'n': buffer[i++] = '\n'; break;
                case 't': buffer[i++] = '\t'; break;
                case '\\': buffer[i++] = '\\'; break;
                case '"': buffer[i++] = '"'; break;
                default: buffer[i++] = escaped;
            }
        } else {
            if (i < 1023) buffer[i++] = esrc_lexer_advance(lexer);
        }
    }
    
    esrc_lexer_advance(lexer); /* Skip closing quote */
    buffer[i] = '\0';
    
    token.value = str_duplicate(buffer);
    return token;
}

EsrcToken esrc_lexer_next_token(EsrcLexer *lexer) {
    esrc_lexer_skip_whitespace(lexer);
    esrc_lexer_skip_comment(lexer);
    esrc_lexer_skip_whitespace(lexer);
    
    if (esrc_lexer_peek(lexer) == '\0') {
        EsrcToken token = {ESRC_TOKEN_EOF, NULL, lexer->line, lexer->column};
        return token;
    }
    
    if (isalpha(esrc_lexer_peek(lexer)) || esrc_lexer_peek(lexer) == '_') {
        return esrc_lexer_read_identifier(lexer);
    }
    
    if (isdigit(esrc_lexer_peek(lexer))) {
        return esrc_lexer_read_number(lexer);
    }
    
    if (esrc_lexer_peek(lexer) == '"') {
        return esrc_lexer_read_string(lexer);
    }
    
    EsrcToken token;
    token.line = lexer->line;
    token.column = lexer->column;
    
    char c = esrc_lexer_advance(lexer);
    char buf[2] = {c, '\0'};
    token.value = str_duplicate(buf);
    
    switch (c) {
        case '(': token.type = ESRC_TOKEN_LPAREN; break;
        case ')': token.type = ESRC_TOKEN_RPAREN; break;
        case '{': token.type = ESRC_TOKEN_LBRACE; break;
        case '}': token.type = ESRC_TOKEN_RBRACE; break;
        case ';': token.type = ESRC_TOKEN_SEMICOLON; break;
        case ',': token.type = ESRC_TOKEN_COMMA; break;
        default: token.type = ESRC_TOKEN_OPERATOR;
    }
    
    return token;
}

/* Compiler functions */
void esrc_compiler_init(EsrcCompiler *compiler, FILE *output) {
    compiler->output = output;
    compiler->error_count = 0;
    compiler->error_message[0] = '\0';
    compiler->module.function_count = 0;
    compiler->module.global_count = 0;
    compiler->module.dependency_count = 0;
    compiler->module.functions = NULL;
    compiler->module.globals = NULL;
    compiler->module.dependencies = NULL;
}

void esrc_compiler_error(EsrcCompiler *compiler, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(compiler->error_message, sizeof(compiler->error_message), fmt, ap);
    va_end(ap);
    compiler->error_count++;
    printf("ERROR: %s\n", compiler->error_message);
}

void esrc_compile_header(EsrcCompiler *compiler, const char *module_name) {
    fprintf(compiler->output, "/* Generated by ESRC compiler */\n");
    fprintf(compiler->output, "#include <windows.h>\n");
    fprintf(compiler->output, "#include <stdio.h>\n\n");
    fprintf(compiler->output, "/* Module: %s */\n\n", module_name);
}

void esrc_compile_metadata(EsrcCompiler *compiler, const char *name, const char *version, const char *author, const char *description) {
    fprintf(compiler->output, "__declspec(dllexport) const char* get_module_name(void) { return \"%s\"; }\n", name);
    fprintf(compiler->output, "__declspec(dllexport) const char* get_module_version(void) { return \"%s\"; }\n", version);
    fprintf(compiler->output, "__declspec(dllexport) const char* get_module_author(void) { return \"%s\"; }\n", author);
    fprintf(compiler->output, "__declspec(dllexport) const char* get_module_description(void) { return \"%s\"; }\n\n", description);
}

void esrc_compile_init_function(EsrcCompiler *compiler) {
    fprintf(compiler->output, "__declspec(dllexport) int module_init(void* api) {\n");
    fprintf(compiler->output, "    /* Initialize module */\n");
    fprintf(compiler->output, "    return 0;\n");
    fprintf(compiler->output, "}\n\n");
}

void esrc_compile_cleanup_function(EsrcCompiler *compiler) {
    fprintf(compiler->output, "__declspec(dllexport) void module_cleanup(void) {\n");
    fprintf(compiler->output, "    /* Cleanup module */\n");
    fprintf(compiler->output, "}\n\n");
}

int esrc_parse_and_compile(EsrcCompiler *compiler, const char *source) {
    EsrcLexer lexer;
    esrc_lexer_init(&lexer, source);
    
    EsrcToken token = esrc_lexer_next_token(&lexer);
    
    while (token.type != ESRC_TOKEN_EOF) {
        if (token.type == ESRC_TOKEN_KEYWORD && strcmp(token.value, "function") == 0) {
            /* Parse function declaration */
            free(token.value);
            token = esrc_lexer_next_token(&lexer);
            
            if (token.type != ESRC_TOKEN_IDENTIFIER) {
                esrc_compiler_error(compiler, "Expected function name");
                return -1;
            }
            
            char *func_name = str_duplicate(token.value);
            fprintf(compiler->output, "__declspec(dllexport) void %s(void) {\n", func_name);
            fprintf(compiler->output, "    /* Function implementation */\n");
            fprintf(compiler->output, "}\n\n");
            
            free(func_name);
        }
        
        if (token.value) free(token.value);
        token = esrc_lexer_next_token(&lexer);
    }
    
    return compiler->error_count > 0 ? -1 : 0;
}

int esrc_compile(const char *esrc_path, const char *emod_path) {
    log_message("Compiling ESRC: %s -> %s", esrc_path, emod_path);
    
    /* Read source file */
    FILE *f = fopen(esrc_path, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open %s\n", esrc_path);
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    
    /* Create intermediate C file */
    char c_path[MAX_PATH];
    snprintf(c_path, MAX_PATH, "%s.c", emod_path);
    
    FILE *output = fopen(c_path, "w");
    if (!output) {
        fprintf(stderr, "ERROR: Cannot create %s\n", c_path);
        free(source);
        return -1;
    }
    
    EsrcCompiler compiler;
    esrc_compiler_init(&compiler, output);
    
    /* Extract module name from path */
    const char *module_name = strrchr(esrc_path, '\\');
    if (!module_name) module_name = strrchr(esrc_path, '/');
    module_name = module_name ? module_name + 1 : esrc_path;
    
    /* Generate C code */
    esrc_compile_header(&compiler, module_name);
    esrc_compile_metadata(&compiler, module_name, "1.0", "ESRC", "Compiled module");
    
    int result = esrc_parse_and_compile(&compiler, source);
    
    esrc_compile_init_function(&compiler);
    esrc_compile_cleanup_function(&compiler);
    
    fclose(output);
    free(source);
    
    if (result != 0) {
        fprintf(stderr, "Compilation failed with %d errors\n", compiler.error_count);
        log_message("ESRC compilation failed: %d errors", compiler.error_count);
        return -1;
    }
    
    /* Compile C to DLL using system compiler */
    char compile_cmd[MAX_PATH * 3];
    snprintf(compile_cmd, sizeof(compile_cmd), 
             "gcc -shared -o \"%s\" \"%s\" -Wl,--out-implib,\"%s.a\"",
             emod_path, c_path, emod_path);
    
    printf("Compiling: %s\n", compile_cmd);
    int compile_result = system(compile_cmd);
    
    if (compile_result != 0) {
        fprintf(stderr, "ERROR: C compilation failed\n");
        log_message("C compilation failed for %s", c_path);
        return -1;
    }
    
    /* Clean up intermediate file */
    remove(c_path);
    
    printf("Successfully compiled %s\n", emod_path);
    log_message("Successfully compiled ESRC module: %s", emod_path);
    
    return 0;
}

/* ==== TERMINAL FUNCTIONS ==== */

void terminal_init(void) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hConsole, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, mode);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    g_state.screen_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    g_state.screen_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void terminal_cleanup(void) {
    printf("\033[2J\033[H");
}

void terminal_clear(void) {
    printf("\033[H");  /* Move cursor to home without clearing */
}

int terminal_get_key(void) {
    if (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            ch = _getch(); /* Extended key */
        }
        return ch;
    }
    return -1;
}

/* ==== EDITOR FUNCTIONS ==== */

void editor_set_status_message(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_state.status_message, sizeof(g_state.status_message), fmt, ap);
    va_end(ap);
}

void editor_init(void) {
    g_state.tab_count = 0;
    g_state.active_tab = 0;
    g_state.mode = MODE_NORMAL;
    g_state.exit_command[0] = '\0';
    g_state.module_count = 0;
    
    terminal_init();
    
    /* Create initial tab */
    tab_init(&g_state.tabs[g_state.tab_count++], "untitled");
    
    editor_set_status_message("GNU ede v%s - Press Ctrl+Q to exit", EDE_VERSION);
}

void editor_cleanup(void) {
    module_unload_all();
    
    for (int t = 0; t < g_state.tab_count; t++) {
        for (int f = 0; f < g_state.tabs[t].file_count; f++) {
            tab_close_file(&g_state.tabs[t], f);
        }
    }
}

void editor_refresh_screen(void) {
    /* Hide cursor during refresh */
    printf("\033[?25l");
    
    /* Move to home position */
    printf("\033[H");
    
    /* Draw tabs */
    printf("\033[7m");
    for (int i = 0; i < g_state.tab_count; i++) {
        if (i == g_state.active_tab) {
            printf("[%d:%s*]", i + 1, g_state.tabs[i].name);
        } else {
            printf(" %d:%s ", i + 1, g_state.tabs[i].name);
        }
    }
    printf("\033[K");  /* Clear to end of line */
    printf("\033[0m\n");
    
    /* Draw file content */
    Tab *tab = &g_state.tabs[g_state.active_tab];
    if (tab->file_count > 0) {
        FileBuffer *fb = tab->files[tab->active_file];
        
        for (int i = 0; i < g_state.screen_rows - 3; i++) {
            int line_len;
            char *line = buffer_get_line(fb->buffer, i + fb->row_offset, &line_len);
            if (line) {
                printf("%s", line);
            } else {
                printf("~");
            }
            printf("\033[K\n");  /* Clear to end of line after each line */
        }
        
        /* Status bar */
        printf("\033[7m%-*s\033[0m", g_state.screen_cols, g_state.status_message);
        printf("\033[K\n");  /* Clear rest of status line */
        
        /* Command line */
        if (g_state.mode == MODE_EXIT_CONFIRM) {
            printf("> %s", g_state.exit_command);
        } else {
            printf("Row:%d Col:%d %s %s", 
                   fb->cursor_row + 1, 
                   fb->cursor_col + 1,
                   fb->modified ? "[+]" : "",
                   fb->filepath ? fb->filepath : "[No Name]");
        }
        printf("\033[K");  /* Clear to end of line */
        
        /* Position cursor at edit location */
        int screen_row = fb->cursor_row - fb->row_offset + 1;  /* +1 for tab line */
        int screen_col = fb->cursor_col - fb->col_offset;
        printf("\033[%d;%dH", screen_row + 1, screen_col + 1);
    }
    
    /* Show cursor */
    printf("\033[?25h");
    
    fflush(stdout);
}

void editor_process_key(int key) {
    if (g_state.mode == MODE_EXIT_CONFIRM) {
        if (key == '\r' || key == '\n') {
            return;
        } else if (key == 27) { /* ESC */
            g_state.mode = MODE_NORMAL;
            g_state.exit_command[0] = '\0';
            editor_set_status_message("Exit cancelled");
        } else if (key == 8) { /* Backspace */
            size_t len = strlen(g_state.exit_command);
            if (len > 0) g_state.exit_command[len - 1] = '\0';
        } else if (key >= 32 && key < 127) {
            size_t len = strlen(g_state.exit_command);
            if (len < sizeof(g_state.exit_command) - 1) {
                g_state.exit_command[len] = (char)key;
                g_state.exit_command[len + 1] = '\0';
            }
        }
        return;
    }
    
    /* Ctrl+Q */
    if (key == 17) {
        g_state.mode = MODE_EXIT_CONFIRM;
        g_state.exit_command[0] = '\0';
        editor_set_status_message("Type 'save' or 'discard' to exit:");
        return;
    }
    
    /* Ctrl+S */
    if (key == 19) {
        Tab *tab = &g_state.tabs[g_state.active_tab];
        if (tab->file_count > 0) {
            FileBuffer *fb = tab->files[tab->active_file];
            if (fb->filepath) {
                buffer_save_file(fb->buffer, fb->filepath);
                fb->modified = false;
                editor_set_status_message("Saved: %s", fb->filepath);
                
                for (int i = 0; i < g_state.module_count; i++) {
                    if (g_state.modules[i].on_save) {
                        g_state.modules[i].on_save(fb->filepath);
                    }
                }
            }
        }
        return;
    }
    
    /* Ctrl+T */
    if (key == 20) {
        if (g_state.tab_count < MAX_TABS) {
            tab_init(&g_state.tabs[g_state.tab_count], "untitled");
            g_state.active_tab = g_state.tab_count;
            g_state.tab_count++;
            tab_add_file(&g_state.tabs[g_state.active_tab], NULL);
            editor_set_status_message("New tab created");
        }
        return;
    }
    
    /* Ctrl+W */
    if (key == 23) {
        if (g_state.tab_count > 1) {
            g_state.tab_count--;
            if (g_state.active_tab >= g_state.tab_count) {
                g_state.active_tab = g_state.tab_count - 1;
            }
            editor_set_status_message("Tab closed");
        }
        return;
    }
    
    /* Tab key - switch files */
    if (key == 9) {
        Tab *tab = &g_state.tabs[g_state.active_tab];
        if (tab->file_count > 1) {
            tab->active_file = (tab->active_file + 1) % tab->file_count;
            editor_set_status_message("Switched to file %d", tab->active_file + 1);
        }
        return;
    }
    
    /* Regular text input */
    Tab *tab = &g_state.tabs[g_state.active_tab];
    if (tab->file_count > 0) {
        FileBuffer *fb = tab->files[tab->active_file];
        
        if (key >= 32 && key < 127) {
            size_t pos = fb->cursor_row * MAX_LINE_LENGTH + fb->cursor_col;
            buffer_insert_char(fb->buffer, (char)key, pos);
            fb->cursor_col++;
            fb->modified = true;
        } else if (key == 8) { /* Backspace */
            if (fb->cursor_col > 0) {
                size_t pos = fb->cursor_row * MAX_LINE_LENGTH + fb->cursor_col - 1;
                buffer_delete_char(fb->buffer, pos);
                fb->cursor_col--;
                fb->modified = true;
            }
        }
    }
}

/* ==== CONFIGURATION SYSTEM ==== */

void config_set_defaults(void) {
    g_state.config.show_line_numbers = true;
    g_state.config.syntax_highlighting = true;
    g_state.config.auto_indent = true;
    g_state.config.show_whitespace = false;
    g_state.config.tab_width = 4;
    g_state.config.use_spaces = true;
    g_state.config.word_wrap = false;
    g_state.config.show_status_bar = true;
    g_state.config.show_ruler = true;
    g_state.config.undo_levels = MAX_UNDO_LEVELS;
    g_state.config.backup_files = true;
    snprintf(g_state.config.backup_dir, MAX_PATH, "%s\\.ede_backup", getenv("USERPROFILE"));
    g_state.config.autosave_interval = 300; /* 5 minutes */
    g_state.config.highlight_current_line = true;
    g_state.config.show_matching_bracket = true;
    g_state.config.scroll_margin = 5;
    
    log_message("Configuration set to defaults");
}

void config_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        log_message("Config file not found: %s", path);
        return;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        /* Remove comments */
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';
        
        /* Parse key=value */
        char *equals = strchr(line, '=');
        if (!equals) continue;
        
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        
        /* Trim whitespace */
        while (isspace(*key)) key++;
        while (isspace(*value)) value++;
        
        char *end = key + strlen(key) - 1;
        while (end > key && isspace(*end)) *end-- = '\0';
        
        end = value + strlen(value) - 1;
        while (end > value && isspace(*end)) *end-- = '\0';
        
        /* Apply configuration */
        if (strcmp(key, "show_line_numbers") == 0) {
            g_state.config.show_line_numbers = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "syntax_highlighting") == 0) {
            g_state.config.syntax_highlighting = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "auto_indent") == 0) {
            g_state.config.auto_indent = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "show_whitespace") == 0) {
            g_state.config.show_whitespace = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "tab_width") == 0) {
            g_state.config.tab_width = atoi(value);
        } else if (strcmp(key, "use_spaces") == 0) {
            g_state.config.use_spaces = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "word_wrap") == 0) {
            g_state.config.word_wrap = (strcmp(value, "true") == 0);
        }
    }
    
    fclose(f);
    log_message("Configuration loaded from %s", path);
}

void config_save(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) {
        log_message("Cannot save config to %s", path);
        return;
    }
    
    fprintf(f, "# GNU ede configuration file\n\n");
    fprintf(f, "show_line_numbers=%s\n", g_state.config.show_line_numbers ? "true" : "false");
    fprintf(f, "syntax_highlighting=%s\n", g_state.config.syntax_highlighting ? "true" : "false");
    fprintf(f, "auto_indent=%s\n", g_state.config.auto_indent ? "true" : "false");
    fprintf(f, "show_whitespace=%s\n", g_state.config.show_whitespace ? "true" : "false");
    fprintf(f, "tab_width=%d\n", g_state.config.tab_width);
    fprintf(f, "use_spaces=%s\n", g_state.config.use_spaces ? "true" : "false");
    fprintf(f, "word_wrap=%s\n", g_state.config.word_wrap ? "true" : "false");
    
    fclose(f);
    log_message("Configuration saved to %s", path);
}

/* ==== FILE BROWSER ==== */

typedef struct {
    char path[MAX_PATH];
    bool is_directory;
    long size;
    time_t modified;
} FileEntry;

typedef struct {
    char current_path[MAX_PATH];
    FileEntry *entries;
    int entry_count;
    int selected;
    int offset;
} FileBrowser;

FileBrowser g_file_browser;

void filebrowser_init(const char *start_path) {
    strncpy(g_file_browser.current_path, start_path, MAX_PATH - 1);
    g_file_browser.entries = NULL;
    g_file_browser.entry_count = 0;
    g_file_browser.selected = 0;
    g_file_browser.offset = 0;
}

void filebrowser_scan(void) {
    /* Free previous entries */
    if (g_file_browser.entries) {
        free(g_file_browser.entries);
    }
    
    g_file_browser.entry_count = 0;
    g_file_browser.entries = malloc(sizeof(FileEntry) * 1000);
    
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", g_file_browser.current_path);
    
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) return;
    
    /* Add parent directory */
    strcpy(g_file_browser.entries[g_file_browser.entry_count].path, "..");
    g_file_browser.entries[g_file_browser.entry_count].is_directory = true;
    g_file_browser.entries[g_file_browser.entry_count].size = 0;
    g_file_browser.entries[g_file_browser.entry_count].modified = 0;
    g_file_browser.entry_count++;
    
    do {
        if (strcmp(find_data.cFileName, ".") == 0) continue;
        if (strcmp(find_data.cFileName, "..") == 0) continue;
        
        FileEntry *entry = &g_file_browser.entries[g_file_browser.entry_count];
        strncpy(entry->path, find_data.cFileName, MAX_PATH - 1);
        entry->is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entry->size = ((long)find_data.nFileSizeHigh << 32) | find_data.nFileSizeLow;
        
        FILETIME ft = find_data.ftLastWriteTime;
        ULARGE_INTEGER ull;
        ull.LowPart = ft.dwLowDateTime;
        ull.HighPart = ft.dwHighDateTime;
        entry->modified = ull.QuadPart / 10000000ULL - 11644473600ULL;
        
        g_file_browser.entry_count++;
        
        if (g_file_browser.entry_count >= 1000) break;
    } while (FindNextFileA(hFind, &find_data));
    
    FindClose(hFind);
    log_message("File browser scanned: %d entries in %s", g_file_browser.entry_count, g_file_browser.current_path);
}

void filebrowser_enter_selected(void) {
    if (g_file_browser.selected < 0 || g_file_browser.selected >= g_file_browser.entry_count) return;
    
    FileEntry *entry = &g_file_browser.entries[g_file_browser.selected];
    
    if (entry->is_directory) {
        if (strcmp(entry->path, "..") == 0) {
            /* Go up one directory */
            char *last_slash = strrchr(g_file_browser.current_path, '\\');
            if (last_slash) *last_slash = '\0';
        } else {
            /* Enter directory */
            size_t len = strlen(g_file_browser.current_path);
            snprintf(g_file_browser.current_path + len, MAX_PATH - len, "\\%s", entry->path);
        }
        filebrowser_scan();
        g_file_browser.selected = 0;
    } else {
        /* Open file */
        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", g_file_browser.current_path, entry->path);
        tab_add_file(&g_state.tabs[g_state.active_tab], full_path);
        log_message("Opened file from browser: %s", full_path);
    }
}

void filebrowser_render(void) {
    printf("\n=== File Browser: %s ===\n\n", g_file_browser.current_path);
    
    int start = g_file_browser.offset;
    int end = min(g_file_browser.entry_count, start + g_state.screen_rows - 10);
    
    for (int i = start; i < end; i++) {
        FileEntry *entry = &g_file_browser.entries[i];
        
        if (i == g_file_browser.selected) {
            printf("\033[7m"); /* Invert */
        }
        
        if (entry->is_directory) {
            printf("[DIR]  %s\n", entry->path);
        } else {
            printf("[FILE] %s (%ld bytes)\n", entry->path, entry->size);
        }
        
        if (i == g_file_browser.selected) {
            printf("\033[0m"); /* Reset */
        }
    }
    
    printf("\nUse arrows to navigate, Enter to select, Esc to cancel\n");
}

/* ==== CURSOR MOVEMENT ==== */

void cursor_move_up(FileBuffer *fb) {
    if (fb->cursor_row > 0) {
        fb->cursor_row--;
        
        /* Adjust cursor_col if new line is shorter */
        int line_len;
        char *line = buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
        if (line && fb->cursor_col > line_len) {
            fb->cursor_col = line_len;
        }
        
        /* Adjust viewport */
        if (fb->cursor_row < fb->row_offset) {
            fb->row_offset = fb->cursor_row;
        }
        
        module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
    }
}

void cursor_move_down(FileBuffer *fb) {
    if (fb->cursor_row < fb->line_count - 1) {
        fb->cursor_row++;
        
        /* Adjust cursor_col if new line is shorter */
        int line_len;
        char *line = buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
        if (line && fb->cursor_col > line_len) {
            fb->cursor_col = line_len;
        }
        
        /* Adjust viewport */
        if (fb->cursor_row >= fb->row_offset + g_state.screen_rows - 5) {
            fb->row_offset++;
        }
        
        module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
    }
}

void cursor_move_left(FileBuffer *fb) {
    if (fb->cursor_col > 0) {
        fb->cursor_col--;
        module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
    } else if (fb->cursor_row > 0) {
        /* Move to end of previous line */
        fb->cursor_row--;
        int line_len;
        buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
        fb->cursor_col = line_len;
        module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
    }
}

void cursor_move_right(FileBuffer *fb) {
    int line_len;
    char *line = buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
    
    if (fb->cursor_col < line_len) {
        fb->cursor_col++;
        module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
    } else if (fb->cursor_row < fb->line_count - 1) {
        /* Move to start of next line */
        fb->cursor_row++;
        fb->cursor_col = 0;
        module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
    }
}

void cursor_move_word_forward(FileBuffer *fb) {
    int line_len;
    char *line = buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
    if (!line) return;
    
    /* Skip current word */
    while (fb->cursor_col < line_len && isalnum(line[fb->cursor_col])) {
        fb->cursor_col++;
    }
    
    /* Skip whitespace */
    while (fb->cursor_col < line_len && isspace(line[fb->cursor_col])) {
        fb->cursor_col++;
    }
    
    module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
}

void cursor_move_word_backward(FileBuffer *fb) {
    if (fb->cursor_col == 0) return;
    
    int line_len;
    char *line = buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
    if (!line) return;
    
    fb->cursor_col--;
    
    /* Skip whitespace */
    while (fb->cursor_col > 0 && isspace(line[fb->cursor_col])) {
        fb->cursor_col--;
    }
    
    /* Skip word */
    while (fb->cursor_col > 0 && isalnum(line[fb->cursor_col - 1])) {
        fb->cursor_col--;
    }
    
    module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
}

void cursor_move_line_start(FileBuffer *fb) {
    fb->cursor_col = 0;
    module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
}

void cursor_move_line_end(FileBuffer *fb) {
    int line_len;
    buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
    fb->cursor_col = line_len;
    module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
}

void cursor_move_buffer_start(FileBuffer *fb) {
    fb->cursor_row = 0;
    fb->cursor_col = 0;
    fb->row_offset = 0;
    module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
}

void cursor_move_buffer_end(FileBuffer *fb) {
    fb->cursor_row = fb->line_count - 1;
    fb->cursor_col = 0;
    fb->row_offset = max(0, fb->line_count - g_state.screen_rows + 5);
    module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
}

void cursor_page_up(FileBuffer *fb) {
    int page_size = g_state.screen_rows - 5;
    fb->cursor_row = max(0, fb->cursor_row - page_size);
    fb->row_offset = max(0, fb->row_offset - page_size);
    module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
}

void cursor_page_down(FileBuffer *fb) {
    int page_size = g_state.screen_rows - 5;
    fb->cursor_row = min(fb->line_count - 1, fb->cursor_row + page_size);
    fb->row_offset = min(fb->line_count - 1, fb->row_offset + page_size);
    module_trigger_on_cursor_move(fb->cursor_row, fb->cursor_col);
}

/* ==== AUTO-COMPLETE ==== */

typedef struct {
    char **suggestions;
    int suggestion_count;
    int selected;
    bool active;
} AutoComplete;

AutoComplete g_autocomplete;

void autocomplete_init(void) {
    g_autocomplete.suggestions = NULL;
    g_autocomplete.suggestion_count = 0;
    g_autocomplete.selected = 0;
    g_autocomplete.active = false;
}

void autocomplete_generate(FileBuffer *fb) {
    /* Free previous suggestions */
    if (g_autocomplete.suggestions) {
        for (int i = 0; i < g_autocomplete.suggestion_count; i++) {
            free(g_autocomplete.suggestions[i]);
        }
        free(g_autocomplete.suggestions);
    }
    
    g_autocomplete.suggestion_count = 0;
    g_autocomplete.suggestions = malloc(sizeof(char*) * 100);
    g_autocomplete.selected = 0;
    
    /* Get current word */
    int line_len;
    char *line = buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
    if (!line) return;
    
    char word[256];
    int word_len = 0;
    
    int col = fb->cursor_col - 1;
    while (col >= 0 && (isalnum(line[col]) || line[col] == '_')) {
        col--;
    }
    col++;
    
    while (col < fb->cursor_col && word_len < 255) {
        word[word_len++] = line[col++];
    }
    word[word_len] = '\0';
    
    if (word_len < 2) return;
    
    /* Simple keyword suggestions based on file type */
    if (fb->filetype == FILETYPE_C || fb->filetype == FILETYPE_CPP) {
        const char *keywords[] = {
            "if", "else", "while", "for", "switch", "case", "break",
            "continue", "return", "struct", "typedef", "sizeof",
            "printf", "scanf", "malloc", "free", "memcpy", "strlen",
            NULL
        };
        
        for (int i = 0; keywords[i] != NULL && g_autocomplete.suggestion_count < 100; i++) {
            if (strncmp(word, keywords[i], word_len) == 0) {
                g_autocomplete.suggestions[g_autocomplete.suggestion_count++] = str_duplicate(keywords[i]);
            }
        }
    }
    
    g_autocomplete.active = (g_autocomplete.suggestion_count > 0);
    log_message("Autocomplete: %d suggestions for '%s'", g_autocomplete.suggestion_count, word);
}

void autocomplete_accept(FileBuffer *fb) {
    if (!g_autocomplete.active || g_autocomplete.suggestion_count == 0) return;
    
    const char *suggestion = g_autocomplete.suggestions[g_autocomplete.selected];
    
    /* Delete current partial word */
    int line_len;
    char *line = buffer_get_line(fb->buffer, fb->cursor_row, &line_len);
    if (!line) return;
    
    int col = fb->cursor_col - 1;
    while (col >= 0 && (isalnum(line[col]) || line[col] == '_')) {
        size_t pos = fb->cursor_row * MAX_LINE_LENGTH + col;
        buffer_delete_char(fb->buffer, pos);
        col--;
    }
    fb->cursor_col = col + 1;
    
    /* Insert suggestion */
    size_t pos = fb->cursor_row * MAX_LINE_LENGTH + fb->cursor_col;
    for (const char *p = suggestion; *p; p++) {
        buffer_insert_char(fb->buffer, *p, pos++);
        fb->cursor_col++;
    }
    
    fb->modified = true;
    g_autocomplete.active = false;
    
    log_message("Autocomplete accepted: %s", suggestion);
}

void autocomplete_render(void) {
    if (!g_autocomplete.active) return;
    
    printf("\n--- Autocomplete ---\n");
    for (int i = 0; i < g_autocomplete.suggestion_count; i++) {
        if (i == g_autocomplete.selected) printf("> ");
        else printf("  ");
        printf("%s\n", g_autocomplete.suggestions[i]);
    }
}

/* ==== COMMAND EXECUTION ==== */

void command_execute(const char *cmd) {
    if (strlen(cmd) == 0) return;
    
    log_message("Executing command: %s", cmd);
    
    /* Save command to history */
    if (g_state.cmd_history.count < 100) {
        strncpy(g_state.cmd_history.commands[g_state.cmd_history.count++], cmd, 255);
    }
    
    char *cmd_copy = str_duplicate(cmd);
    char *command = strtok(cmd_copy, " ");
    char *args = strtok(NULL, "");
    
    if (strcmp(command, "save") == 0) {
        FileBuffer *fb = get_current_buffer();
        if (fb && fb->filepath) {
            buffer_save_file(fb->buffer, fb->filepath);
            fb->modified = false;
            editor_set_status_message("Saved: %s", fb->filepath);
        }
    } else if (strcmp(command, "quit") == 0 || strcmp(command, "q") == 0) {
        g_state.running = false;
    } else if (strcmp(command, "wq") == 0) {
        FileBuffer *fb = get_current_buffer();
        if (fb && fb->filepath) {
            buffer_save_file(fb->buffer, fb->filepath);
        }
        g_state.running = false;
    } else if (strcmp(command, "open") == 0 || strcmp(command, "e") == 0) {
        if (args) {
            tab_add_file(&g_state.tabs[g_state.active_tab], args);
            editor_set_status_message("Opened: %s", args);
        }
    } else if (strcmp(command, "help") == 0) {
        editor_set_status_message("Commands: save, quit, open, search, replace, set");
    } else if (strcmp(command, "search") == 0 || strcmp(command, "find") == 0) {
        if (args) {
            FileBuffer *fb = get_current_buffer();
            if (fb) {
                strncpy(fb->search.pattern, args, MAX_SEARCH_LENGTH - 1);
                int count = search_find_next(fb);
                editor_set_status_message("Searching: %s", args);
            }
        }
    } else if (strcmp(command, "replace") == 0) {
        /* Parse: replace old new */
        if (args) {
            char *old_text = strtok(args, " ");
            char *new_text = strtok(NULL, "");
            if (old_text && new_text) {
                FileBuffer *fb = get_current_buffer();
                if (fb) {
                    strncpy(fb->search.pattern, old_text, MAX_SEARCH_LENGTH - 1);
                    strncpy(fb->search.replace_text, new_text, MAX_SEARCH_LENGTH - 1);
                    int count = search_replace_all(fb);
                    editor_set_status_message("Replaced %d occurrences", count);
                }
            }
        }
    } else if (strcmp(command, "set") == 0) {
        if (args) {
            char *key = strtok(args, "=");
            char *value = strtok(NULL, "");
            if (key && value) {
                /* Apply setting */
                if (strcmp(key, "tab_width") == 0) {
                    g_state.config.tab_width = atoi(value);
                    editor_set_status_message("Tab width set to %d", g_state.config.tab_width);
                } else if (strcmp(key, "syntax") == 0) {
                    g_state.config.syntax_highlighting = (strcmp(value, "on") == 0);
                    editor_set_status_message("Syntax highlighting: %s", value);
                }
            }
        }
    } else if (strcmp(command, "module") == 0) {
        if (args) {
            if (strncmp(args, "load ", 5) == 0) {
                module_load(args + 5);
            } else if (strcmp(args, "list") == 0) {
                printf("\nLoaded modules:\n");
                for (int i = 0; i < g_state.module_count; i++) {
                    printf("%d. %s v%s (%s)\n", i + 1, 
                           g_state.modules[i].name,
                           g_state.modules[i].version,
                           g_state.modules[i].enabled ? "enabled" : "disabled");
                }
                Sleep(2000);
            }
        }
    } else {
        /* Try module commands */
        bool found = false;
        for (int i = 0; i < g_state.module_count; i++) {
            if (g_state.modules[i].execute_command) {
                if (g_state.modules[i].execute_command(command, args) == 0) {
                    found = true;
                    break;
                }
            }
        }
        
        if (!found) {
            editor_set_status_message("Unknown command: %s", command);
        }
    }
    
    free(cmd_copy);
}

/* ==== SPLIT VIEW ==== */

void split_view_horizontal(void) {
    Tab *tab = &g_state.tabs[g_state.active_tab];
    tab->split_view = true;
    tab->split_ratio = 50;
    log_message("Enabled horizontal split view");
    editor_set_status_message("Split view enabled");
}

void split_view_vertical(void) {
    Tab *tab = &g_state.tabs[g_state.active_tab];
    tab->split_view = true;
    tab->split_ratio = 50;
    log_message("Enabled vertical split view");
    editor_set_status_message("Split view enabled");
}

void split_view_close(void) {
    Tab *tab = &g_state.tabs[g_state.active_tab];
    tab->split_view = false;
    log_message("Disabled split view");
    editor_set_status_message("Split view disabled");
}

/* ==== HELPER FUNCTIONS ==== */

FileBuffer* get_current_buffer(void) {
    if (g_state.tab_count == 0) return NULL;
    Tab *tab = &g_state.tabs[g_state.active_tab];
    if (tab->file_count == 0) return NULL;
    return tab->files[tab->active_file];
}

void update_line_count(FileBuffer *fb) {
    if (!fb || !fb->buffer) return;
    
    int count = 0;
    for (size_t pos = 0; pos < fb->buffer->gap_start; pos++) {
        if (fb->buffer->content[pos] == '\n') count++;
    }
    for (size_t pos = fb->buffer->gap_end; pos < fb->buffer->buffer_size; pos++) {
        if (fb->buffer->content[pos] == '\n') count++;
    }
    
    fb->line_count = count + 1;
}

void apply_syntax_highlighting(FileBuffer *fb) {
    if (!fb || !g_state.config.syntax_highlighting) return;
    /* Syntax highlighting would be applied during rendering */
}

void editor_run(void) {
    g_state.running = true;
    
    while (g_state.running) {
        editor_refresh_screen();
        
        Sleep(50);
        int key = terminal_get_key();
        
        if (key != -1) {
            g_state.last_key = key;
            g_state.last_activity = time(NULL);
            
            /* Record macro */
            if (g_state.recording_macro) {
                macro_record_key(key);
            }
            
            /* Notify modules */
            module_trigger_on_key(key);
            
            editor_process_key(key);
            
            /* Handle exit */
            if (g_state.mode == MODE_EXIT_CONFIRM) {
                if (strcmp(g_state.exit_command, "save") == 0) {
                    for (int t = 0; t < g_state.tab_count; t++) {
                        for (int f = 0; f < g_state.tabs[t].file_count; f++) {
                            FileBuffer *fb = g_state.tabs[t].files[f];
                            if (fb->modified && fb->filepath) {
                                buffer_save_file(fb->buffer, fb->filepath);
                                module_trigger_on_save(fb->filepath);
                            }
                        }
                    }
                    break;
                } else if (strcmp(g_state.exit_command, "discard") == 0) {
                    break;
                }
            }
        }
        
        /* Auto-save */
        if (g_state.config.autosave_interval > 0) {
            static time_t last_autosave = 0;
            time_t now = time(NULL);
            if (now - last_autosave > g_state.config.autosave_interval) {
                for (int t = 0; t < g_state.tab_count; t++) {
                    for (int f = 0; f < g_state.tabs[t].file_count; f++) {
                        FileBuffer *fb = g_state.tabs[t].files[f];
                        if (fb->modified && fb->filepath) {
                            buffer_save_file(fb->buffer, fb->filepath);
                            log_message("Auto-saved: %s", fb->filepath);
                        }
                    }
                }
                last_autosave = now;
            }
        }
    }
}

/* ==== VISUAL MODE ==== */

void visual_mode_start(FileBuffer *fb) {
    fb->selection.active = true;
    fb->selection.start_row = fb->cursor_row;
    fb->selection.start_col = fb->cursor_col;
    fb->selection.end_row = fb->cursor_row;
    fb->selection.end_col = fb->cursor_col;
    g_state.mode = MODE_VISUAL;
    editor_set_status_message("-- VISUAL --");
    log_message("Visual mode started");
}

void visual_mode_update(FileBuffer *fb) {
    fb->selection.end_row = fb->cursor_row;
    fb->selection.end_col = fb->cursor_col;
}

void visual_mode_copy(FileBuffer *fb) {
    if (!fb->selection.active) return;
    
    int start_row = min(fb->selection.start_row, fb->selection.end_row);
    int end_row = max(fb->selection.start_row, fb->selection.end_row);
    int start_col = min(fb->selection.start_col, fb->selection.end_col);
    int end_col = max(fb->selection.start_col, fb->selection.end_col);
    
    char buffer[65536];
    int buf_pos = 0;
    
    for (int row = start_row; row <= end_row && buf_pos < 65535; row++) {
        int line_len;
        char *line = buffer_get_line(fb->buffer, row, &line_len);
        if (!line) continue;
        
        int col_start = (row == start_row) ? start_col : 0;
        int col_end = (row == end_row) ? end_col : line_len;
        
        for (int col = col_start; col < col_end && buf_pos < 65535; col++) {
            buffer[buf_pos++] = line[col];
        }
        
        if (row < end_row && buf_pos < 65535) {
            buffer[buf_pos++] = '\n';
        }
    }
    
    clipboard_set(buffer, buf_pos, false);
    editor_set_status_message("Copied %d bytes", buf_pos);
    log_message("Copied selection: %d bytes", buf_pos);
}

void visual_mode_cut(FileBuffer *fb) {
    visual_mode_copy(fb);
    
    int start_row = min(fb->selection.start_row, fb->selection.end_row);
    int end_row = max(fb->selection.start_row, fb->selection.end_row);
    int start_col = min(fb->selection.start_col, fb->selection.end_col);
    int end_col = max(fb->selection.start_col, fb->selection.end_col);
    
    api_delete_range(start_row, start_col, end_row, end_col);
    
    fb->cursor_row = start_row;
    fb->cursor_col = start_col;
    fb->modified = true;
    
    editor_set_status_message("Cut selection");
    log_message("Cut selection");
}

void visual_mode_paste(FileBuffer *fb) {
    size_t len;
    bool line_mode;
    const char *text = clipboard_get(&len, &line_mode);
    
    if (!text || len == 0) return;
    
    api_insert_text(text);
    editor_set_status_message("Pasted %zu bytes", len);
    log_message("Pasted: %zu bytes", len);
}

void visual_mode_end(FileBuffer *fb) {
    fb->selection.active = false;
    g_state.mode = MODE_NORMAL;
    editor_set_status_message("");
}

/* ==== DIFF SYSTEM ==== */

typedef enum {
    DIFF_EQUAL,
    DIFF_INSERT,
    DIFF_DELETE,
    DIFF_CHANGE
} DiffType;

typedef struct {
    DiffType type;
    int line_num;
    char *text;
} DiffLine;

typedef struct {
    DiffLine *lines;
    int line_count;
    char *file1;
    char *file2;
} DiffResult;

DiffResult g_diff_result;

void diff_files(const char *file1, const char *file2) {
    log_message("Diffing: %s vs %s", file1, file2);
    
    FILE *f1 = fopen(file1, "r");
    FILE *f2 = fopen(file2, "r");
    
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        editor_set_status_message("Cannot open files for diff");
        return;
    }
    
    g_diff_result.file1 = str_duplicate(file1);
    g_diff_result.file2 = str_duplicate(file2);
    g_diff_result.lines = malloc(sizeof(DiffLine) * 10000);
    g_diff_result.line_count = 0;
    
    char line1[4096], line2[4096];
    int line_num = 0;
    
    while (fgets(line1, sizeof(line1), f1) && fgets(line2, sizeof(line2), f2)) {
        line_num++;
        
        DiffLine *diff = &g_diff_result.lines[g_diff_result.line_count++];
        diff->line_num = line_num;
        
        if (strcmp(line1, line2) == 0) {
            diff->type = DIFF_EQUAL;
            diff->text = str_duplicate(line1);
        } else {
            diff->type = DIFF_CHANGE;
            char buffer[8192];
            snprintf(buffer, sizeof(buffer), "- %s+ %s", line1, line2);
            diff->text = str_duplicate(buffer);
        }
    }
    
    fclose(f1);
    fclose(f2);
    
    editor_set_status_message("Diff complete: %d lines", g_diff_result.line_count);
    log_message("Diff completed: %d differences", g_diff_result.line_count);
}

void diff_render(void) {
    printf("\n=== DIFF: %s vs %s ===\n\n", g_diff_result.file1, g_diff_result.file2);
    
    for (int i = 0; i < g_diff_result.line_count && i < 50; i++) {
        DiffLine *line = &g_diff_result.lines[i];
        
        switch (line->type) {
            case DIFF_EQUAL:
                printf("  %s", line->text);
                break;
            case DIFF_INSERT:
                printf("\033[32m+ %s\033[0m", line->text);
                break;
            case DIFF_DELETE:
                printf("\033[31m- %s\033[0m", line->text);
                break;
            case DIFF_CHANGE:
                printf("\033[33m%s\033[0m", line->text);
                break;
        }
    }
}

/* ==== GIT INTEGRATION ==== */

typedef struct {
    char repo_path[MAX_PATH];
    bool is_repo;
    char current_branch[256];
    int uncommitted_changes;
} GitState;

GitState g_git_state;

void git_init(const char *repo_path) {
    strncpy(g_git_state.repo_path, repo_path, MAX_PATH - 1);
    
    char git_dir[MAX_PATH];
    snprintf(git_dir, MAX_PATH, "%s\\.git", repo_path);
    
    g_git_state.is_repo = (GetFileAttributesA(git_dir) != INVALID_FILE_ATTRIBUTES);
    
    if (g_git_state.is_repo) {
        /* Read current branch */
        char head_file[MAX_PATH];
        snprintf(head_file, MAX_PATH, "%s\\HEAD", git_dir);
        
        FILE *f = fopen(head_file, "r");
        if (f) {
            char line[512];
            if (fgets(line, sizeof(line), f)) {
                char *ref = strstr(line, "refs/heads/");
                if (ref) {
                    ref += 11;
                    char *newline = strchr(ref, '\n');
                    if (newline) *newline = '\0';
                    strncpy(g_git_state.current_branch, ref, 255);
                } else {
                    strcpy(g_git_state.current_branch, "detached");
                }
            }
            fclose(f);
        }
        
        log_message("Git repo detected: %s (branch: %s)", repo_path, g_git_state.current_branch);
    }
}

void git_status(void) {
    if (!g_git_state.is_repo) {
        editor_set_status_message("Not a git repository");
        return;
    }
    
    char cmd[MAX_PATH + 50];
    snprintf(cmd, sizeof(cmd), "git -C \"%s\" status --short", g_git_state.repo_path);
    
    FILE *fp = _popen(cmd, "r");
    if (!fp) {
        editor_set_status_message("Git command failed");
        return;
    }
    
    printf("\n=== Git Status ===\n");
    printf("Branch: %s\n\n", g_git_state.current_branch);
    
    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < 50) {
        printf("%s", line);
        count++;
    }
    
    _pclose(fp);
    
    editor_set_status_message("Git status: %d changes", count);
    Sleep(3000);
}

void git_commit(const char *message) {
    if (!g_git_state.is_repo) return;
    
    char cmd[MAX_PATH + 512];
    snprintf(cmd, sizeof(cmd), "git -C \"%s\" commit -am \"%s\"", g_git_state.repo_path, message);
    
    int result = system(cmd);
    
    if (result == 0) {
        editor_set_status_message("Committed: %s", message);
        log_message("Git commit: %s", message);
    } else {
        editor_set_status_message("Commit failed");
    }
}

void git_diff_current_file(FileBuffer *fb) {
    if (!g_git_state.is_repo || !fb || !fb->filepath) return;
    
    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "git -C \"%s\" diff \"%s\"", g_git_state.repo_path, fb->filepath);
    
    FILE *fp = _popen(cmd, "r");
    if (!fp) return;
    
    printf("\n=== Git Diff: %s ===\n\n", fb->filepath);
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '+' && line[1] != '+') {
            printf("\033[32m%s\033[0m", line);
        } else if (line[0] == '-' && line[1] != '-') {
            printf("\033[31m%s\033[0m", line);
        } else if (line[0] == '@') {
            printf("\033[36m%s\033[0m", line);
        } else {
            printf("%s", line);
        }
    }
    
    _pclose(fp);
    Sleep(5000);
}

/* ==== SESSION MANAGEMENT ==== */

typedef struct {
    char filepath[MAX_PATH];
    int cursor_row;
    int cursor_col;
} SessionFile;

typedef struct {
    SessionFile files[100];
    int file_count;
    int active_file;
} Session;

void session_save(const char *session_file) {
    FILE *f = fopen(session_file, "w");
    if (!f) {
        log_message("Cannot save session to %s", session_file);
        return;
    }
    
    fprintf(f, "# EDE Session File\n");
    fprintf(f, "version=%s\n", EDE_VERSION);
    fprintf(f, "tabs=%d\n", g_state.tab_count);
    fprintf(f, "active_tab=%d\n\n", g_state.active_tab);
    
    for (int t = 0; t < g_state.tab_count; t++) {
        Tab *tab = &g_state.tabs[t];
        fprintf(f, "[tab:%d]\n", t);
        fprintf(f, "name=%s\n", tab->name);
        fprintf(f, "files=%d\n", tab->file_count);
        
        for (int i = 0; i < tab->file_count; i++) {
            FileBuffer *fb = tab->files[i];
            if (fb->filepath) {
                fprintf(f, "file=%s\n", fb->filepath);
                fprintf(f, "cursor=%d,%d\n", fb->cursor_row, fb->cursor_col);
            }
        }
        fprintf(f, "\n");
    }
    
    fclose(f);
    editor_set_status_message("Session saved: %s", session_file);
    log_message("Session saved to %s", session_file);
}

void session_load(const char *session_file) {
    FILE *f = fopen(session_file, "r");
    if (!f) {
        log_message("Cannot load session from %s", session_file);
        return;
    }
    
    char line[512];
    int current_tab = -1;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#') continue;
        
        if (strncmp(line, "[tab:", 5) == 0) {
            current_tab = atoi(line + 5);
            if (current_tab >= g_state.tab_count) {
                tab_init(&g_state.tabs[g_state.tab_count++], "restored");
            }
        } else if (strncmp(line, "file=", 5) == 0) {
            if (current_tab >= 0) {
                char *path = line + 5;
                char *newline = strchr(path, '\n');
                if (newline) *newline = '\0';
                tab_add_file(&g_state.tabs[current_tab], path);
            }
        }
    }
    
    fclose(f);
    editor_set_status_message("Session loaded: %s", session_file);
    log_message("Session loaded from %s", session_file);
}

/* ==== SNIPPET SYSTEM ==== */

typedef struct {
    char name[64];
    char trigger[32];
    char content[1024];
    char language[32];
} Snippet;

typedef struct {
    Snippet snippets[200];
    int snippet_count;
} SnippetManager;

SnippetManager g_snippet_manager;

void snippet_init(void) {
    g_snippet_manager.snippet_count = 0;
    
    /* Add some default snippets */
    Snippet *s;
    
    /* C snippets */
    s = &g_snippet_manager.snippets[g_snippet_manager.snippet_count++];
    strcpy(s->name, "Main Function");
    strcpy(s->trigger, "main");
    strcpy(s->content, "int main(int argc, char **argv) {\n    \n    return 0;\n}");
    strcpy(s->language, "c");
    
    s = &g_snippet_manager.snippets[g_snippet_manager.snippet_count++];
    strcpy(s->name, "For Loop");
    strcpy(s->trigger, "for");
    strcpy(s->content, "for (int i = 0; i < n; i++) {\n    \n}");
    strcpy(s->language, "c");
    
    s = &g_snippet_manager.snippets[g_snippet_manager.snippet_count++];
    strcpy(s->name, "If Statement");
    strcpy(s->trigger, "if");
    strcpy(s->content, "if (condition) {\n    \n}");
    strcpy(s->language, "c");
    
    s = &g_snippet_manager.snippets[g_snippet_manager.snippet_count++];
    strcpy(s->name, "Printf");
    strcpy(s->trigger, "pf");
    strcpy(s->content, "printf(\"%s\\n\", );");
    strcpy(s->language, "c");
    
    log_message("Snippet system initialized: %d snippets", g_snippet_manager.snippet_count);
}

void snippet_insert(FileBuffer *fb, const char *trigger) {
    for (int i = 0; i < g_snippet_manager.snippet_count; i++) {
        Snippet *s = &g_snippet_manager.snippets[i];
        if (strcmp(s->trigger, trigger) == 0) {
            /* Check language match */
            const char *lang = get_filetype_name(fb->filetype);
            if (strcasecmp(s->language, lang) != 0 && strcmp(s->language, "*") != 0) {
                continue;
            }
            
            /* Insert snippet */
            api_insert_text(s->content);
            editor_set_status_message("Inserted snippet: %s", s->name);
            log_message("Inserted snippet: %s", s->name);
            return;
        }
    }
}

void snippet_list(void) {
    printf("\n=== Available Snippets ===\n\n");
    
    for (int i = 0; i < g_snippet_manager.snippet_count && i < 50; i++) {
        Snippet *s = &g_snippet_manager.snippets[i];
        printf("%s (%s) - %s\n", s->trigger, s->language, s->name);
    }
    
    printf("\nPress any key to continue...\n");
    _getch();
}

/* ==== PLUGIN LOADER ==== */

typedef struct {
    char name[256];
    char path[MAX_PATH];
    bool loaded;
    void *data;
} Plugin;

typedef struct {
    Plugin plugins[50];
    int plugin_count;
} PluginManager;

PluginManager g_plugin_manager;

void plugin_scan_directory(const char *plugin_dir) {
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*.emod", plugin_dir);
    
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) return;
    
    do {
        if (g_plugin_manager.plugin_count >= 50) break;
        
        Plugin *p = &g_plugin_manager.plugins[g_plugin_manager.plugin_count++];
        strncpy(p->name, find_data.cFileName, 255);
        snprintf(p->path, MAX_PATH, "%s\\%s", plugin_dir, find_data.cFileName);
        p->loaded = false;
        p->data = NULL;
        
    } while (FindNextFileA(hFind, &find_data));
    
    FindClose(hFind);
    
    log_message("Plugin scan: found %d plugins in %s", g_plugin_manager.plugin_count, plugin_dir);
}

void plugin_load_all(void) {
    for (int i = 0; i < g_plugin_manager.plugin_count; i++) {
        Plugin *p = &g_plugin_manager.plugins[i];
        if (!p->loaded) {
            if (module_load(p->path) == 0) {
                p->loaded = true;
                log_message("Plugin loaded: %s", p->name);
            }
        }
    }
}

void plugin_list(void) {
    printf("\n=== Plugins ===\n\n");
    
    for (int i = 0; i < g_plugin_manager.plugin_count; i++) {
        Plugin *p = &g_plugin_manager.plugins[i];
        printf("%d. %s [%s]\n", i + 1, p->name, p->loaded ? "LOADED" : "not loaded");
        printf("   Path: %s\n\n", p->path);
    }
    
    printf("Press any key to continue...\n");
    _getch();
}

/* ==== PERFORMANCE MONITOR ==== */

typedef struct {
    time_t start_time;
    int keystrokes;
    int files_opened;
    int files_saved;
    int searches;
    int undo_operations;
    int redo_operations;
    long bytes_edited;
} PerformanceStats;

PerformanceStats g_perf_stats;

void perf_init(void) {
    memset(&g_perf_stats, 0, sizeof(PerformanceStats));
    g_perf_stats.start_time = time(NULL);
}

void perf_record_keystroke(void) {
    g_perf_stats.keystrokes++;
}

void perf_record_file_open(void) {
    g_perf_stats.files_opened++;
}

void perf_record_file_save(void) {
    g_perf_stats.files_saved++;
}

void perf_record_search(void) {
    g_perf_stats.searches++;
}

void perf_show_stats(void) {
    time_t now = time(NULL);
    long uptime = (long)(now - g_perf_stats.start_time);
    
    printf("\n=== Performance Statistics ===\n\n");
    printf("Uptime: %ld seconds\n", uptime);
    printf("Keystrokes: %d\n", g_perf_stats.keystrokes);
    printf("Files opened: %d\n", g_perf_stats.files_opened);
    printf("Files saved: %d\n", g_perf_stats.files_saved);
    printf("Searches: %d\n", g_perf_stats.searches);
    printf("Undo operations: %d\n", g_perf_stats.undo_operations);
    printf("Redo operations: %d\n", g_perf_stats.redo_operations);
    printf("Bytes edited: %ld\n", g_perf_stats.bytes_edited);
    
    if (uptime > 0) {
        printf("\nAverage keystrokes/second: %.2f\n", (float)g_perf_stats.keystrokes / uptime);
    }
    
    printf("\nPress any key to continue...\n");
    _getch();
}

/* ==== THEME SYSTEM ==== */

typedef struct {
    int fg_normal;
    int bg_normal;
    int fg_keyword;
    int bg_keyword;
    int fg_string;
    int bg_string;
    int fg_comment;
    int bg_comment;
    int fg_number;
    int bg_number;
    int fg_status;
    int bg_status;
} ColorTheme;

typedef struct {
    ColorTheme themes[10];
    int theme_count;
    int active_theme;
} ThemeManager;

ThemeManager g_theme_manager;

void theme_init(void) {
    g_theme_manager.theme_count = 0;
    
    /* Default theme */
    ColorTheme *t = &g_theme_manager.themes[g_theme_manager.theme_count++];
    t->fg_normal = 7;
    t->bg_normal = 0;
    t->fg_keyword = 14;
    t->bg_keyword = 0;
    t->fg_string = 10;
    t->bg_string = 0;
    t->fg_comment = 8;
    t->bg_comment = 0;
    t->fg_number = 11;
    t->bg_number = 0;
    t->fg_status = 0;
    t->bg_status = 7;
    
    /* Dark theme */
    t = &g_theme_manager.themes[g_theme_manager.theme_count++];
    t->fg_normal = 15;
    t->bg_normal = 0;
    t->fg_keyword = 12;
    t->bg_keyword = 0;
    t->fg_string = 10;
    t->bg_string = 0;
    t->fg_comment = 8;
    t->bg_comment = 0;
    t->fg_number = 11;
    t->bg_number = 0;
    t->fg_status = 15;
    t->bg_status = 4;
    
    g_theme_manager.active_theme = 0;
    log_message("Theme system initialized: %d themes", g_theme_manager.theme_count);
}

void theme_apply(int theme_id) {
    if (theme_id < 0 || theme_id >= g_theme_manager.theme_count) return;
    
    g_theme_manager.active_theme = theme_id;
    editor_set_status_message("Theme changed");
    log_message("Applied theme: %d", theme_id);
}

/* ==== ADVANCED RENDERING ==== */

void render_line_numbers(FileBuffer *fb, int screen_row, int file_row) {
    if (!g_state.config.show_line_numbers) return;
    
    if (file_row < fb->line_count) {
        printf("\033[90m%4d \033[0m", file_row + 1);
    } else {
        printf("     ");
    }
}

void render_syntax_highlighted_line(FileBuffer *fb, const char *line, int line_len) {
    if (!g_state.config.syntax_highlighting) {
        printf("%s", line);
        return;
    }
    
    ColorTheme *theme = &g_theme_manager.themes[g_theme_manager.active_theme];
    
    for (int i = 0; i < line_len; i++) {
        char c = line[i];
        
        /* Simple syntax highlighting */
        if (c == '"') {
            /* String */
            printf("\033[%dm", theme->fg_string);
            printf("%c", c);
            i++;
            while (i < line_len && line[i] != '"') {
                printf("%c", line[i++]);
            }
            if (i < line_len) printf("%c", line[i]);
            printf("\033[0m");
        } else if (c == '/' && i + 1 < line_len && line[i + 1] == '/') {
            /* Comment */
            printf("\033[%dm", theme->fg_comment);
            printf("%s", line + i);
            printf("\033[0m");
            break;
        } else if (isdigit(c)) {
            /* Number */
            printf("\033[%dm%c\033[0m", theme->fg_number, c);
        } else {
            printf("%c", c);
        }
    }
}

void render_status_line(void) {
    ColorTheme *theme = &g_theme_manager.themes[g_theme_manager.active_theme];
    
    printf("\033[%d;%dm", theme->fg_status + 30, theme->bg_status + 40);
    printf("%-*s", g_state.screen_cols, g_state.status_message);
    printf("\033[0m");
}

void render_ruler(FileBuffer *fb) {
    if (!g_state.config.show_ruler) return;
    
    printf("\033[90m"); /* Gray */
    for (int i = 0; i < g_state.screen_cols; i++) {
        if (i % 10 == 0) {
            printf("%d", (i / 10) % 10);
        } else if (i % 5 == 0) {
            printf("|");
        } else {
            printf(".");
        }
    }
    printf("\033[0m\n");
}

/* ==== MAIN ==== */

void print_banner(void) {
    printf("GNU ede v%s\n", EDE_VERSION);
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS] [FILE...]\n", prog_name);
    printf("\nOptions:\n");
    printf("  -o FILE    Compile .esrc to .emod\n");
    printf("  -m FILE    Load .emod module\n");
    printf("  -h         Show help\n");
    printf("  -v         Show version\n");
    printf("  -s FILE    Load session\n");
    printf("  -c FILE    Load config\n");
}

/* ==== ERROR HANDLING ==== */

typedef struct {
    char message[512];
    char file[MAX_PATH];
    int line;
    time_t timestamp;
} ErrorEntry;

typedef struct {
    ErrorEntry errors[100];
    int error_count;
} ErrorLog;

ErrorLog g_error_log;

void error_log_init(void) {
    g_error_log.error_count = 0;
}

void error_log_add(const char *message, const char *file, int line) {
    if (g_error_log.error_count >= 100) return;
    
    ErrorEntry *e = &g_error_log.errors[g_error_log.error_count++];
    strncpy(e->message, message, 511);
    strncpy(e->file, file ? file : "unknown", MAX_PATH - 1);
    e->line = line;
    e->timestamp = time(NULL);
    
    log_message("ERROR: %s (%s:%d)", message, file, line);
}

void error_log_show(void) {
    printf("\n=== Error Log (%d errors) ===\n\n", g_error_log.error_count);
    
    for (int i = max(0, g_error_log.error_count - 20); i < g_error_log.error_count; i++) {
        ErrorEntry *e = &g_error_log.errors[i];
        printf("%d. %s\n", i + 1, e->message);
        printf("   Location: %s:%d\n", e->file, e->line);
        
        char time_str[64];
        struct tm *tm_info = localtime(&e->timestamp);
        strftime(time_str, 64, "%Y-%m-%d %H:%M:%S", tm_info);
        printf("   Time: %s\n\n", time_str);
    }
    
    printf("Press any key to continue...\n");
    _getch();
}

/* ==== DEBUGGING SYSTEM ==== */

typedef enum {
    DEBUG_LEVEL_OFF,
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_WARN,
    DEBUG_LEVEL_INFO,
    DEBUG_LEVEL_DEBUG,
    DEBUG_LEVEL_TRACE
} DebugLevel;

typedef struct {
    DebugLevel level;
    bool enabled;
    FILE *debug_file;
    char debug_log_path[MAX_PATH];
} DebugSystem;

DebugSystem g_debug_system;

void debug_init(void) {
    g_debug_system.level = DEBUG_LEVEL_INFO;
    g_debug_system.enabled = false;
    g_debug_system.debug_file = NULL;
    snprintf(g_debug_system.debug_log_path, MAX_PATH, "%s\\.ede_debug.log", getenv("USERPROFILE"));
}

void debug_enable(void) {
    if (g_debug_system.debug_file) return;
    
    g_debug_system.debug_file = fopen(g_debug_system.debug_log_path, "a");
    if (g_debug_system.debug_file) {
        g_debug_system.enabled = true;
        fprintf(g_debug_system.debug_file, "\n=== Debug session started ===\n");
        log_message("Debug mode enabled: %s", g_debug_system.debug_log_path);
    }
}

void debug_disable(void) {
    if (g_debug_system.debug_file) {
        fprintf(g_debug_system.debug_file, "=== Debug session ended ===\n\n");
        fclose(g_debug_system.debug_file);
        g_debug_system.debug_file = NULL;
    }
    g_debug_system.enabled = false;
}

void debug_log(DebugLevel level, const char *func, int line, const char *fmt, ...) {
    if (!g_debug_system.enabled || level > g_debug_system.level) return;
    
    const char *level_str[] = {"OFF", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, 32, "%H:%M:%S", tm_info);
    
    fprintf(g_debug_system.debug_file, "[%s] [%s] %s:%d: ", time_str, level_str[level], func, line);
    
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_debug_system.debug_file, fmt, ap);
    va_end(ap);
    
    fprintf(g_debug_system.debug_file, "\n");
    fflush(g_debug_system.debug_file);
}

#define DEBUG_ERROR(...) debug_log(DEBUG_LEVEL_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)
#define DEBUG_WARN(...) debug_log(DEBUG_LEVEL_WARN, __FUNCTION__, __LINE__, __VA_ARGS__)
#define DEBUG_INFO(...) debug_log(DEBUG_LEVEL_INFO, __FUNCTION__, __LINE__, __VA_ARGS__)
#define DEBUG_DEBUG(...) debug_log(DEBUG_LEVEL_DEBUG, __FUNCTION__, __LINE__, __VA_ARGS__)
#define DEBUG_TRACE(...) debug_log(DEBUG_LEVEL_TRACE, __FUNCTION__, __LINE__, __VA_ARGS__)

/* ==== BACKUP SYSTEM ==== */

void backup_create(const char *filepath) {
    if (!g_state.config.backup_files) return;
    
    char backup_path[MAX_PATH];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, 64, "%Y%m%d_%H%M%S", tm_info);
    
    const char *filename = strrchr(filepath, '\\');
    if (!filename) filename = filepath;
    else filename++;
    
    snprintf(backup_path, MAX_PATH, "%s\\%s.%s.bak", 
             g_state.config.backup_dir, filename, timestamp);
    
    /* Create backup directory if needed */
    CreateDirectoryA(g_state.config.backup_dir, NULL);
    
    /* Copy file */
    if (CopyFileA(filepath, backup_path, FALSE)) {
        log_message("Backup created: %s", backup_path);
    }
}

void backup_restore_list(void) {
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*.bak", g_state.config.backup_dir);
    
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No backups found\n");
        return;
    }
    
    printf("\n=== Available Backups ===\n\n");
    
    int count = 0;
    do {
        printf("%d. %s\n", ++count, find_data.cFileName);
        
        FILETIME ft = find_data.ftLastWriteTime;
        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        printf("   Modified: %04d-%02d-%02d %02d:%02d:%02d\n", 
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        printf("   Size: %ld bytes\n\n", find_data.nFileSizeLow);
        
    } while (FindNextFileA(hFind, &find_data) && count < 50);
    
    FindClose(hFind);
    
    printf("Press any key to continue...\n");
    _getch();
}

/* ==== WORKSPACE MANAGEMENT ==== */

typedef struct {
    char name[256];
    char root_path[MAX_PATH];
    char files[50][MAX_PATH];
    int file_count;
} Workspace;

typedef struct {
    Workspace workspaces[20];
    int workspace_count;
    int active_workspace;
} WorkspaceManager;

WorkspaceManager g_workspace_manager;

void workspace_init(void) {
    g_workspace_manager.workspace_count = 0;
    g_workspace_manager.active_workspace = -1;
}

void workspace_create(const char *name, const char *root_path) {
    if (g_workspace_manager.workspace_count >= 20) return;
    
    Workspace *ws = &g_workspace_manager.workspaces[g_workspace_manager.workspace_count++];
    strncpy(ws->name, name, 255);
    strncpy(ws->root_path, root_path, MAX_PATH - 1);
    ws->file_count = 0;
    
    log_message("Workspace created: %s at %s", name, root_path);
}

void workspace_add_file(Workspace *ws, const char *filepath) {
    if (!ws || ws->file_count >= 50) return;
    
    strncpy(ws->files[ws->file_count++], filepath, MAX_PATH - 1);
}

void workspace_open(int workspace_id) {
    if (workspace_id < 0 || workspace_id >= g_workspace_manager.workspace_count) return;
    
    Workspace *ws = &g_workspace_manager.workspaces[workspace_id];
    g_workspace_manager.active_workspace = workspace_id;
    
    /* Load all files in workspace */
    for (int i = 0; i < ws->file_count; i++) {
        tab_add_file(&g_state.tabs[g_state.active_tab], ws->files[i]);
    }
    
    editor_set_status_message("Opened workspace: %s", ws->name);
    log_message("Workspace opened: %s", ws->name);
}

void workspace_save(int workspace_id, const char *save_path) {
    if (workspace_id < 0 || workspace_id >= g_workspace_manager.workspace_count) return;
    
    Workspace *ws = &g_workspace_manager.workspaces[workspace_id];
    
    FILE *f = fopen(save_path, "w");
    if (!f) return;
    
    fprintf(f, "# EDE Workspace: %s\n", ws->name);
    fprintf(f, "name=%s\n", ws->name);
    fprintf(f, "root=%s\n\n", ws->root_path);
    
    fprintf(f, "[files]\n");
    for (int i = 0; i < ws->file_count; i++) {
        fprintf(f, "%s\n", ws->files[i]);
    }
    
    fclose(f);
    log_message("Workspace saved: %s to %s", ws->name, save_path);
}

void workspace_list(void) {
    printf("\n=== Workspaces ===\n\n");
    
    for (int i = 0; i < g_workspace_manager.workspace_count; i++) {
        Workspace *ws = &g_workspace_manager.workspaces[i];
        printf("%d. %s%s\n", i + 1, ws->name, 
               (i == g_workspace_manager.active_workspace) ? " [ACTIVE]" : "");
        printf("   Root: %s\n", ws->root_path);
        printf("   Files: %d\n\n", ws->file_count);
    }
    
    printf("Press any key to continue...\n");
    _getch();
}

/* ==== PROJECT TEMPLATES ==== */

typedef struct {
    char name[128];
    char description[256];
    char files[20][MAX_PATH];
    char contents[20][4096];
    int file_count;
} ProjectTemplate;

typedef struct {
    ProjectTemplate templates[10];
    int template_count;
} TemplateManager;

TemplateManager g_template_manager;

void template_init(void) {
    g_template_manager.template_count = 0;
    
    /* C Project Template */
    ProjectTemplate *t = &g_template_manager.templates[g_template_manager.template_count++];
    strcpy(t->name, "C Project");
    strcpy(t->description, "Basic C project with main.c and Makefile");
    t->file_count = 2;
    
    strcpy(t->files[0], "main.c");
    strcpy(t->contents[0], 
           "#include <stdio.h>\n\n"
           "int main(int argc, char **argv) {\n"
           "    printf(\"Hello, World!\\n\");\n"
           "    return 0;\n"
           "}\n");
    
    strcpy(t->files[1], "Makefile");
    strcpy(t->contents[1],
           "CC=gcc\n"
           "CFLAGS=-Wall -O2\n\n"
           "all: main\n\n"
           "main: main.c\n"
           "\t$(CC) $(CFLAGS) -o main main.c\n\n"
           "clean:\n"
           "\trm -f main\n");
    
    log_message("Template system initialized: %d templates", g_template_manager.template_count);
}

void template_create_project(int template_id, const char *project_path) {
    if (template_id < 0 || template_id >= g_template_manager.template_count) return;
    
    ProjectTemplate *t = &g_template_manager.templates[template_id];
    
    /* Create project directory */
    CreateDirectoryA(project_path, NULL);
    
    /* Create files */
    for (int i = 0; i < t->file_count; i++) {
        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", project_path, t->files[i]);
        
        FILE *f = fopen(full_path, "w");
        if (f) {
            fprintf(f, "%s", t->contents[i]);
            fclose(f);
            log_message("Created: %s", full_path);
        }
    }
    
    editor_set_status_message("Project created: %s", t->name);
}

void template_list(void) {
    printf("\n=== Project Templates ===\n\n");
    
    for (int i = 0; i < g_template_manager.template_count; i++) {
        ProjectTemplate *t = &g_template_manager.templates[i];
        printf("%d. %s\n", i + 1, t->name);
        printf("   %s\n", t->description);
        printf("   Files: %d\n\n", t->file_count);
    }
    
    printf("Press any key to continue...\n");
    _getch();
}

/* ==== TAG SYSTEM (CTAGS INTEGRATION) ==== */

typedef struct {
    char symbol[256];
    char file[MAX_PATH];
    int line;
    char kind[32];
} Tag;

typedef struct {
    Tag *tags;
    int tag_count;
    bool loaded;
} TagDatabase;

TagDatabase g_tag_db;

void tags_init(void) {
    g_tag_db.tags = NULL;
    g_tag_db.tag_count = 0;
    g_tag_db.loaded = false;
}

void tags_load(const char *tags_file) {
    FILE *f = fopen(tags_file, "r");
    if (!f) {
        log_message("Cannot load tags from %s", tags_file);
        return;
    }
    
    if (g_tag_db.tags) free(g_tag_db.tags);
    
    g_tag_db.tags = malloc(sizeof(Tag) * 10000);
    g_tag_db.tag_count = 0;
    
    char line[1024];
    while (fgets(line, sizeof(line), f) && g_tag_db.tag_count < 10000) {
        if (line[0] == '!') continue; /* Skip comments */
        
        Tag *tag = &g_tag_db.tags[g_tag_db.tag_count++];
        
        /* Parse tag line: symbol\tfile\tline;\"\tkind */
        char *tab1 = strchr(line, '\t');
        if (!tab1) continue;
        *tab1 = '\0';
        strncpy(tag->symbol, line, 255);
        
        char *tab2 = strchr(tab1 + 1, '\t');
        if (!tab2) continue;
        *tab2 = '\0';
        strncpy(tag->file, tab1 + 1, MAX_PATH - 1);
        
        tag->line = atoi(tab2 + 1);
        strcpy(tag->kind, "unknown");
    }
    
    fclose(f);
    g_tag_db.loaded = true;
    
    editor_set_status_message("Loaded %d tags", g_tag_db.tag_count);
    log_message("Loaded %d tags from %s", g_tag_db.tag_count, tags_file);
}

void tags_find(const char *symbol) {
    if (!g_tag_db.loaded) {
        editor_set_status_message("No tags loaded");
        return;
    }
    
    printf("\n=== Tags matching '%s' ===\n\n", symbol);
    
    int found = 0;
    for (int i = 0; i < g_tag_db.tag_count && found < 50; i++) {
        Tag *tag = &g_tag_db.tags[i];
        if (strstr(tag->symbol, symbol)) {
            printf("%s\n", tag->symbol);
            printf("  File: %s:%d\n", tag->file, tag->line);
            printf("  Kind: %s\n\n", tag->kind);
            found++;
        }
    }
    
    if (found == 0) {
        printf("No tags found\n");
    }
    
    printf("Press any key to continue...\n");
    _getch();
}

void tags_jump(const char *symbol) {
    if (!g_tag_db.loaded) return;
    
    for (int i = 0; i < g_tag_db.tag_count; i++) {
        Tag *tag = &g_tag_db.tags[i];
        if (strcmp(tag->symbol, symbol) == 0) {
            /* Open file and jump to line */
            tab_add_file(&g_state.tabs[g_state.active_tab], tag->file);
            
            FileBuffer *fb = get_current_buffer();
            if (fb) {
                fb->cursor_row = tag->line - 1;
                fb->cursor_col = 0;
                editor_set_status_message("Jumped to: %s", tag->symbol);
            }
            return;
        }
    }
    
    editor_set_status_message("Tag not found: %s", symbol);
}

/* ==== BUILD SYSTEM ==== */

typedef struct {
    char command[512];
    char working_dir[MAX_PATH];
    int exit_code;
    char output[4096];
    time_t last_build;
    bool success;
} BuildResult;

BuildResult g_build_result;

void build_execute(const char *command) {
    log_message("Build command: %s", command);
    
    strncpy(g_build_result.command, command, 511);
    GetCurrentDirectoryA(MAX_PATH, g_build_result.working_dir);
    g_build_result.last_build = time(NULL);
    
    printf("\n=== Building ===\n");
    printf("Command: %s\n", command);
    printf("Working directory: %s\n\n", g_build_result.working_dir);
    
    FILE *fp = _popen(command, "r");
    if (!fp) {
        editor_set_status_message("Build failed: cannot execute command");
        g_build_result.success = false;
        return;
    }
    
    int output_pos = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp) && output_pos < 4000) {
        printf("%s", line);
        int len = strlen(line);
        if (output_pos + len < 4096) {
            strcpy(g_build_result.output + output_pos, line);
            output_pos += len;
        }
    }
    g_build_result.output[output_pos] = '\0';
    
    g_build_result.exit_code = _pclose(fp);
    g_build_result.success = (g_build_result.exit_code == 0);
    
    printf("\nBuild %s (exit code: %d)\n", 
           g_build_result.success ? "SUCCESS" : "FAILED", 
           g_build_result.exit_code);
    
    editor_set_status_message("Build %s", g_build_result.success ? "succeeded" : "failed");
    log_message("Build %s: %d", g_build_result.success ? "succeeded" : "failed", g_build_result.exit_code);
    
    printf("\nPress any key to continue...\n");
    _getch();
}

void build_show_result(void) {
    printf("\n=== Last Build Result ===\n\n");
    printf("Command: %s\n", g_build_result.command);
    printf("Status: %s\n", g_build_result.success ? "SUCCESS" : "FAILED");
    printf("Exit code: %d\n", g_build_result.exit_code);
    
    char time_str[64];
    struct tm *tm_info = localtime(&g_build_result.last_build);
    strftime(time_str, 64, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("Time: %s\n\n", time_str);
    
    printf("Output:\n%s\n", g_build_result.output);
    
    printf("Press any key to continue...\n");
    _getch();
}

/* ==== MENU SYSTEM ==== */

typedef struct {
    char label[64];
    void (*callback)(void);
    int key;
} MenuItem;

typedef struct {
    char title[128];
    MenuItem items[20];
    int item_count;
    int selected;
    bool active;
} Menu;

Menu g_main_menu;
Menu g_file_menu;
Menu g_edit_menu;
Menu g_search_menu;
Menu g_tools_menu;
Menu g_help_menu;

void menu_callback_new_file(void) {
    if (g_state.tab_count < MAX_TABS) {
        tab_init(&g_state.tabs[g_state.tab_count], "untitled");
        g_state.active_tab = g_state.tab_count;
        g_state.tab_count++;
        tab_add_file(&g_state.tabs[g_state.active_tab], NULL);
        editor_set_status_message("New file created");
    }
}

void menu_callback_open_file(void) {
    printf("\nEnter filename: ");
    char filename[MAX_PATH];
    if (fgets(filename, MAX_PATH, stdin)) {
        size_t len = strlen(filename);
        if (len > 0 && filename[len - 1] == '\n') filename[len - 1] = '\0';
        tab_add_file(&g_state.tabs[g_state.active_tab], filename);
        perf_record_file_open();
    }
}

void menu_callback_save_file(void) {
    FileBuffer *fb = get_current_buffer();
    if (fb && fb->filepath) {
        backup_create(fb->filepath);
        buffer_save_file(fb->buffer, fb->filepath);
        fb->modified = false;
        module_trigger_on_save(fb->filepath);
        perf_record_file_save();
        editor_set_status_message("Saved: %s", fb->filepath);
    }
}

void menu_callback_save_as(void) {
    printf("\nSave as: ");
    char filename[MAX_PATH];
    if (fgets(filename, MAX_PATH, stdin)) {
        size_t len = strlen(filename);
        if (len > 0 && filename[len - 1] == '\n') filename[len - 1] = '\0';
        FileBuffer *fb = get_current_buffer();
        if (fb) {
            if (fb->filepath) free(fb->filepath);
            fb->filepath = str_duplicate(filename);
            buffer_save_file(fb->buffer, fb->filepath);
            fb->modified = false;
            editor_set_status_message("Saved as: %s", filename);
        }
    }
}

void menu_callback_close_file(void) {
    Tab *tab = &g_state.tabs[g_state.active_tab];
    if (tab->file_count > 0) {
        tab_close_file(tab, tab->active_file);
        if (tab->file_count == 0) {
            tab_add_file(tab, NULL);
        }
        editor_set_status_message("File closed");
    }
}

void menu_callback_exit(void) {
    g_state.running = false;
}

void menu_callback_undo(void) {
    FileBuffer *fb = get_current_buffer();
    if (fb) {
        undo_perform(fb->buffer);
        g_perf_stats.undo_operations++;
        editor_set_status_message("Undo");
    }
}

void menu_callback_redo(void) {
    FileBuffer *fb = get_current_buffer();
    if (fb) {
        redo_perform(fb->buffer);
        g_perf_stats.redo_operations++;
        editor_set_status_message("Redo");
    }
}

void menu_callback_copy(void) {
    FileBuffer *fb = get_current_buffer();
    if (fb && fb->selection.active) {
        visual_mode_copy(fb);
    }
}

void menu_callback_cut(void) {
    FileBuffer *fb = get_current_buffer();
    if (fb && fb->selection.active) {
        visual_mode_cut(fb);
    }
}

void menu_callback_paste(void) {
    FileBuffer *fb = get_current_buffer();
    if (fb) {
        visual_mode_paste(fb);
    }
}

void menu_callback_find(void) {
    g_state.mode = MODE_SEARCH;
    editor_set_status_message("Search: ");
}

void menu_callback_replace(void) {
    g_state.mode = MODE_REPLACE;
    editor_set_status_message("Replace: ");
}

void menu_callback_goto_line(void) {
    printf("\nGo to line: ");
    char line_str[32];
    if (fgets(line_str, 32, stdin)) {
        int line = atoi(line_str);
        FileBuffer *fb = get_current_buffer();
        if (fb && line > 0 && line <= fb->line_count) {
            fb->cursor_row = line - 1;
            fb->cursor_col = 0;
            editor_set_status_message("Jumped to line %d", line);
        }
    }
}

void menu_callback_build(void) {
    printf("\nBuild command: ");
    char cmd[512];
    if (fgets(cmd, 512, stdin)) {
        size_t len = strlen(cmd);
        if (len > 0 && cmd[len - 1] == '\n') cmd[len - 1] = '\0';
        build_execute(cmd);
    }
}

void menu_callback_git_status(void) {
    git_status();
}

void menu_callback_git_diff(void) {
    FileBuffer *fb = get_current_buffer();
    if (fb) {
        git_diff_current_file(fb);
    }
}

void menu_callback_plugins(void) {
    plugin_list();
}

void menu_callback_modules(void) {
    printf("\n=== Loaded Modules ===\n\n");
    for (int i = 0; i < g_state.module_count; i++) {
        Module *m = &g_state.modules[i];
        printf("%d. %s v%s\n", i + 1, m->name, m->version);
        printf("   Author: %s\n", m->author);
        printf("   %s\n", m->description);
        printf("   Status: %s\n\n", m->enabled ? "Enabled" : "Disabled");
    }
    printf("Press any key to continue...\n");
    _getch();
}

void menu_callback_perf_stats(void) {
    perf_show_stats();
}

void menu_callback_help(void) {
    printf("\n=== GNU ede v%s Help ===\n\n", EDE_VERSION);
    printf("File Operations:\n");
    printf("  Ctrl+N      New file\n");
    printf("  Ctrl+O      Open file\n");
    printf("  Ctrl+S      Save file\n");
    printf("  Ctrl+Q      Quit\n\n");
    
    printf("Editing:\n");
    printf("  Ctrl+Z      Undo\n");
    printf("  Ctrl+Y      Redo\n");
    printf("  Ctrl+C      Copy\n");
    printf("  Ctrl+X      Cut\n");
    printf("  Ctrl+V      Paste\n\n");
    
    printf("Navigation:\n");
    printf("  Ctrl+F      Find\n");
    printf("  Ctrl+H      Replace\n");
    printf("  Ctrl+G      Go to line\n\n");
    
    printf("Tabs:\n");
    printf("  Ctrl+T      New tab\n");
    printf("  Ctrl+W      Close tab\n");
    printf("  Ctrl+Tab    Next tab\n\n");
    
    printf("Visual Mode:\n");
    printf("  v           Start visual mode\n");
    printf("  V           Line visual mode\n");
    printf("  Esc         Exit visual mode\n\n");
    
    printf("Press any key to continue...\n");
    _getch();
}

void menu_callback_about(void) {
    printf("\n=== About GNU ede ===\n\n");
    printf("GNU ede v%s\n", EDE_VERSION);
    printf("Advanced modular text editor\n\n");
    printf("Copyright (C) 2025 Free Software Foundation, Inc.\n");
    printf("This is free software; you are free to change and redistribute it.\n");
    printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
    printf("Features:\n");
    printf("  - Multi-file, multi-tab editing\n");
    printf("  - Plugin/module system with ESRC language\n");
    printf("  - Syntax highlighting\n");
    printf("  - Git integration\n");
    printf("  - Build system\n");
    printf("  - Undo/Redo\n");
    printf("  - Search & Replace\n");
    printf("  - Macros\n");
    printf("  - And much more!\n\n");
    printf("Press any key to continue...\n");
    _getch();
}

void menu_init(void) {
    /* File Menu */
    strcpy(g_file_menu.title, "File");
    g_file_menu.item_count = 0;
    g_file_menu.selected = 0;
    g_file_menu.active = false;
    
    MenuItem *item = &g_file_menu.items[g_file_menu.item_count++];
    strcpy(item->label, "New (Ctrl+N)");
    item->callback = menu_callback_new_file;
    item->key = 14;
    
    item = &g_file_menu.items[g_file_menu.item_count++];
    strcpy(item->label, "Open (Ctrl+O)");
    item->callback = menu_callback_open_file;
    item->key = 15;
    
    item = &g_file_menu.items[g_file_menu.item_count++];
    strcpy(item->label, "Save (Ctrl+S)");
    item->callback = menu_callback_save_file;
    item->key = 19;
    
    item = &g_file_menu.items[g_file_menu.item_count++];
    strcpy(item->label, "Save As...");
    item->callback = menu_callback_save_as;
    item->key = 0;
    
    item = &g_file_menu.items[g_file_menu.item_count++];
    strcpy(item->label, "Close");
    item->callback = menu_callback_close_file;
    item->key = 0;
    
    item = &g_file_menu.items[g_file_menu.item_count++];
    strcpy(item->label, "Exit (Ctrl+Q)");
    item->callback = menu_callback_exit;
    item->key = 17;
    
    /* Edit Menu */
    strcpy(g_edit_menu.title, "Edit");
    g_edit_menu.item_count = 0;
    g_edit_menu.selected = 0;
    g_edit_menu.active = false;
    
    item = &g_edit_menu.items[g_edit_menu.item_count++];
    strcpy(item->label, "Undo (Ctrl+Z)");
    item->callback = menu_callback_undo;
    item->key = 26;
    
    item = &g_edit_menu.items[g_edit_menu.item_count++];
    strcpy(item->label, "Redo (Ctrl+Y)");
    item->callback = menu_callback_redo;
    item->key = 25;
    
    item = &g_edit_menu.items[g_edit_menu.item_count++];
    strcpy(item->label, "Copy (Ctrl+C)");
    item->callback = menu_callback_copy;
    item->key = 3;
    
    item = &g_edit_menu.items[g_edit_menu.item_count++];
    strcpy(item->label, "Cut (Ctrl+X)");
    item->callback = menu_callback_cut;
    item->key = 24;
    
    item = &g_edit_menu.items[g_edit_menu.item_count++];
    strcpy(item->label, "Paste (Ctrl+V)");
    item->callback = menu_callback_paste;
    item->key = 22;
    
    /* Search Menu */
    strcpy(g_search_menu.title, "Search");
    g_search_menu.item_count = 0;
    g_search_menu.selected = 0;
    g_search_menu.active = false;
    
    item = &g_search_menu.items[g_search_menu.item_count++];
    strcpy(item->label, "Find (Ctrl+F)");
    item->callback = menu_callback_find;
    item->key = 6;
    
    item = &g_search_menu.items[g_search_menu.item_count++];
    strcpy(item->label, "Replace (Ctrl+H)");
    item->callback = menu_callback_replace;
    item->key = 8;
    
    item = &g_search_menu.items[g_search_menu.item_count++];
    strcpy(item->label, "Go to Line (Ctrl+G)");
    item->callback = menu_callback_goto_line;
    item->key = 7;
    
    /* Tools Menu */
    strcpy(g_tools_menu.title, "Tools");
    g_tools_menu.item_count = 0;
    g_tools_menu.selected = 0;
    g_tools_menu.active = false;
    
    item = &g_tools_menu.items[g_tools_menu.item_count++];
    strcpy(item->label, "Build");
    item->callback = menu_callback_build;
    item->key = 0;
    
    item = &g_tools_menu.items[g_tools_menu.item_count++];
    strcpy(item->label, "Git Status");
    item->callback = menu_callback_git_status;
    item->key = 0;
    
    item = &g_tools_menu.items[g_tools_menu.item_count++];
    strcpy(item->label, "Git Diff");
    item->callback = menu_callback_git_diff;
    item->key = 0;
    
    item = &g_tools_menu.items[g_tools_menu.item_count++];
    strcpy(item->label, "Plugins");
    item->callback = menu_callback_plugins;
    item->key = 0;
    
    item = &g_tools_menu.items[g_tools_menu.item_count++];
    strcpy(item->label, "Modules");
    item->callback = menu_callback_modules;
    item->key = 0;
    
    item = &g_tools_menu.items[g_tools_menu.item_count++];
    strcpy(item->label, "Performance Stats");
    item->callback = menu_callback_perf_stats;
    item->key = 0;
    
    /* Help Menu */
    strcpy(g_help_menu.title, "Help");
    g_help_menu.item_count = 0;
    g_help_menu.selected = 0;
    g_help_menu.active = false;
    
    item = &g_help_menu.items[g_help_menu.item_count++];
    strcpy(item->label, "Help (F1)");
    item->callback = menu_callback_help;
    item->key = 0;
    
    item = &g_help_menu.items[g_help_menu.item_count++];
    strcpy(item->label, "About");
    item->callback = menu_callback_about;
    item->key = 0;
    
    log_message("Menu system initialized");
}

void menu_render(Menu *menu) {
    if (!menu->active) return;
    
    printf("\n=== %s Menu ===\n\n", menu->title);
    
    for (int i = 0; i < menu->item_count; i++) {
        if (i == menu->selected) {
            printf("\033[7m"); /* Invert */
        }
        printf("%d. %s\n", i + 1, menu->items[i].label);
        if (i == menu->selected) {
            printf("\033[0m"); /* Reset */
        }
    }
    
    printf("\nUse arrows to navigate, Enter to select, Esc to cancel\n");
}

void menu_handle_key(Menu *menu, int key) {
    if (key == 72) { /* Up arrow */
        menu->selected = (menu->selected - 1 + menu->item_count) % menu->item_count;
    } else if (key == 80) { /* Down arrow */
        menu->selected = (menu->selected + 1) % menu->item_count;
    } else if (key == '\r' || key == '\n') {
        if (menu->items[menu->selected].callback) {
            menu->items[menu->selected].callback();
        }
        menu->active = false;
    } else if (key == 27) { /* Esc */
        menu->active = false;
    }
}

/* ==== EXTENDED KEY BINDINGS ==== */

typedef struct {
    int key;
    const char *description;
    void (*callback)(void);
} KeyBinding;

typedef struct {
    KeyBinding bindings[100];
    int binding_count;
} KeyBindingManager;

KeyBindingManager g_keybindings;

void keybinding_init(void) {
    g_keybindings.binding_count = 0;
    
    /* Register default keybindings */
    KeyBinding *kb;
    
    kb = &g_keybindings.bindings[g_keybindings.binding_count++];
    kb->key = 14; /* Ctrl+N */
    kb->description = "New file";
    kb->callback = menu_callback_new_file;
    
    kb = &g_keybindings.bindings[g_keybindings.binding_count++];
    kb->key = 15; /* Ctrl+O */
    kb->description = "Open file";
    kb->callback = menu_callback_open_file;
    
    kb = &g_keybindings.bindings[g_keybindings.binding_count++];
    kb->key = 26; /* Ctrl+Z */
    kb->description = "Undo";
    kb->callback = menu_callback_undo;
    
    kb = &g_keybindings.bindings[g_keybindings.binding_count++];
    kb->key = 25; /* Ctrl+Y */
    kb->description = "Redo";
    kb->callback = menu_callback_redo;
    
    kb = &g_keybindings.bindings[g_keybindings.binding_count++];
    kb->key = 6; /* Ctrl+F */
    kb->description = "Find";
    kb->callback = menu_callback_find;
    
    kb = &g_keybindings.bindings[g_keybindings.binding_count++];
    kb->key = 7; /* Ctrl+G */
    kb->description = "Go to line";
    kb->callback = menu_callback_goto_line;
    
    log_message("Keybindings initialized: %d bindings", g_keybindings.binding_count);
}

void keybinding_handle(int key) {
    for (int i = 0; i < g_keybindings.binding_count; i++) {
        if (g_keybindings.bindings[i].key == key) {
            if (g_keybindings.bindings[i].callback) {
                g_keybindings.bindings[i].callback();
            }
            return;
        }
    }
}

void keybinding_list(void) {
    printf("\n=== Key Bindings ===\n\n");
    
    for (int i = 0; i < g_keybindings.binding_count; i++) {
        KeyBinding *kb = &g_keybindings.bindings[i];
        printf("Ctrl+%c: %s\n", 'A' + kb->key - 1, kb->description);
    }
    
    printf("\nPress any key to continue...\n");
    _getch();
}

/* ==== STATUS BAR EXTENSIONS ==== */

void statusbar_render_detailed(void) {
    FileBuffer *fb = get_current_buffer();
    if (!fb) return;
    
    ColorTheme *theme = &g_theme_manager.themes[g_theme_manager.active_theme];
    
    /* Main status bar */
    printf("\033[%d;%dm", theme->fg_status + 30, theme->bg_status + 40);
    
    /* Left side - file info */
    printf(" %s%s ", 
           fb->filepath ? fb->filepath : "[No Name]",
           fb->modified ? " [+]" : "");
    
    /* Middle - position */
    printf("| Line %d/%d Col %d ", 
           fb->cursor_row + 1, fb->line_count, fb->cursor_col + 1);
    
    /* Right side - file type and mode */
    printf("| %s ", get_filetype_name(fb->filetype));
    
    switch (g_state.mode) {
        case MODE_NORMAL: printf("[NORMAL]"); break;
        case MODE_INSERT: printf("[INSERT]"); break;
        case MODE_VISUAL: printf("[VISUAL]"); break;
        case MODE_COMMAND: printf("[COMMAND]"); break;
        case MODE_SEARCH: printf("[SEARCH]"); break;
        default: printf("[???]");
    }
    
    /* Git branch if available */
    if (g_git_state.is_repo) {
        printf(" | git:%s ", g_git_state.current_branch);
    }
    
    /* Module count */
    if (g_state.module_count > 0) {
        printf("| %d mod ", g_state.module_count);
    }
    
    printf("\033[0m\n");
}

/* ==== EXTENDED HELP SYSTEM ==== */

void help_show_commands(void) {
    printf("\n=== EDE Commands ===\n\n");
    printf(":save, :w          Save current file\n");
    printf(":quit, :q          Quit editor\n");
    printf(":wq                Save and quit\n");
    printf(":open FILE, :e     Open file\n");
    printf(":search TEXT, :f   Search for text\n");
    printf(":replace A B       Replace A with B\n");
    printf(":set KEY=VALUE     Set configuration\n");
    printf(":module load FILE  Load module\n");
    printf(":module list       List modules\n");
    printf(":help              Show help\n");
    printf(":git status        Git status\n");
    printf(":git commit MSG    Git commit\n");
    printf(":build CMD         Run build command\n");
    printf(":session save F    Save session\n");
    printf(":session load F    Load session\n");
    printf(":snippets          List snippets\n");
    printf(":plugins           List plugins\n");
    printf(":stats             Performance stats\n");
    printf(":errors            Show error log\n");
    printf(":tags load FILE    Load tags\n");
    printf(":tags find SYM     Find tag\n");
    printf(":diff FILE1 FILE2  Diff two files\n");
    printf("\nPress any key to continue...\n");
    _getch();
}

void help_show_vim_commands(void) {
    printf("\n=== Vim-like Commands ===\n\n");
    printf("Movement:\n");
    printf("  h, j, k, l       Left, Down, Up, Right\n");
    printf("  w, b             Word forward/backward\n");
    printf("  0, $             Line start/end\n");
    printf("  gg, G            Buffer start/end\n");
    printf("  Ctrl+U, Ctrl+D   Page up/down\n\n");
    
    printf("Editing:\n");
    printf("  i                Enter insert mode\n");
    printf("  a                Append (insert after cursor)\n");
    printf("  o, O             Open line below/above\n");
    printf("  x                Delete character\n");
    printf("  dd               Delete line\n");
    printf("  yy               Yank (copy) line\n");
    printf("  p                Paste\n\n");
    
    printf("Visual Mode:\n");
    printf("  v                Visual character mode\n");
    printf("  V                Visual line mode\n");
    printf("  Ctrl+V           Visual block mode\n");
    printf("  y                Yank selection\n");
    printf("  d                Delete selection\n\n");
    
    printf("Press any key to continue...\n");
    _getch();
}

void help_show_modules(void) {
    printf("\n=== EDE Module System ===\n\n");
    printf("Modules extend EDE functionality through DLLs.\n\n");
    
    printf("Module API Functions:\n");
    printf("  get_cursor_row/col()        Get cursor position\n");
    printf("  get_current_file()          Get current file path\n");
    printf("  get_current_line()          Get line at cursor\n");
    printf("  insert_text(text)           Insert text\n");
    printf("  delete_range(...)           Delete text range\n");
    printf("  set_status(msg)             Set status message\n");
    printf("  save_file()                 Save current file\n");
    printf("  load_file(path)             Load file\n\n");
    
    printf("Event Hooks:\n");
    printf("  on_key(key)                 Key press\n");
    printf("  on_save(file)               File saved\n");
    printf("  on_load(file)               File loaded\n");
    printf("  on_cursor_move(row, col)    Cursor moved\n\n");
    
    printf("ESRC Language:\n");
    printf("  Compile: ede module.esrc -o module.emod\n");
    printf("  Load: ede -m module.emod file.txt\n\n");
    
    printf("Press any key to continue...\n");
    _getch();
}

/* ==== FINAL INITIALIZATION ==== */

void init_all_systems(void) {
    error_log_init();
    debug_init();
    perf_init();
    clipboard_init();
    autocomplete_init();
    snippet_init();
    theme_init();
    workspace_init();
    template_init();
    tags_init();
    filebrowser_init(".");
    menu_init();
    keybinding_init();
    
    log_message("All systems initialized");
}

int main(int argc, char **argv) {
    char *output_file = NULL;
    char *module_file = NULL;
    char *session_file = NULL;
    char *config_file = NULL;
    bool compile_mode = false;
    bool debug_mode = false;
    
    /* Simple arg parsing */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
            compile_mode = true;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            module_file = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            session_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            config_file = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_banner();
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0) {
            print_banner();
            return 0;
        }
    }
    
    /* Compile mode */
    if (compile_mode) {
        if (argc < 3) {
            fprintf(stderr, "Error: No input .esrc file\n");
            return 1;
        }
        return esrc_compile(argv[1], output_file);
    }
    
    /* Normal editor mode */
    print_banner();
    
    /* Initialize all systems */
    error_log_init();
    debug_init();
    
    if (debug_mode) {
        debug_enable();
        DEBUG_INFO("EDE starting in debug mode");
    }
    
    perf_init();
    clipboard_init();
    autocomplete_init();
    snippet_init();
    theme_init();
    workspace_init();
    template_init();
    tags_init();
    
    /* Setup log file */
    char log_path[MAX_PATH];
    snprintf(log_path, MAX_PATH, "%s\\.ede.log", getenv("USERPROFILE"));
    g_state.log_fp = fopen(log_path, "a");
    
    log_message("=== EDE v%s started ===", EDE_VERSION);
    log_message("Arguments: %d", argc);
    for (int i = 0; i < argc; i++) {
        log_message("  argv[%d] = %s", i, argv[i]);
    }
    
    /* Load configuration */
    config_set_defaults();
    if (config_file) {
        config_load(config_file);
    } else {
        char default_config[MAX_PATH];
        snprintf(default_config, MAX_PATH, "%s\\.ederc", getenv("USERPROFILE"));
        config_load(default_config);
    }
    
    /* Initialize editor */
    editor_init();
    module_api_init();
    
    /* Initialize git if in a repo */
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    git_init(cwd);
    
    /* Scan for plugins */
    char plugin_dir[MAX_PATH];
    snprintf(plugin_dir, MAX_PATH, "%s\\.ede\\plugins", getenv("USERPROFILE"));
    plugin_scan_directory(plugin_dir);
    
    /* Load module if specified */
    if (module_file) {
        module_load(module_file);
    }
    
    /* Load session if specified */
    if (session_file) {
        session_load(session_file);
    }
    
    /* Open files from args */
    bool has_files = false;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            if (!has_files) {
                tab_add_file(&g_state.tabs[g_state.active_tab], argv[i]);
                has_files = true;
                perf_record_file_open();
            } else {
                if (g_state.tab_count < MAX_TABS) {
                    tab_init(&g_state.tabs[g_state.tab_count], argv[i]);
                    g_state.active_tab = g_state.tab_count;
                    g_state.tab_count++;
                    tab_add_file(&g_state.tabs[g_state.active_tab], argv[i]);
                    perf_record_file_open();
                }
            }
        }
    }
    
    if (!has_files) {
        tab_add_file(&g_state.tabs[g_state.active_tab], NULL);
    }
    
    DEBUG_INFO("Editor initialized, entering main loop");
    
    /* Run the editor */
    editor_run();
    
    DEBUG_INFO("Editor loop exited");
    
    /* Cleanup */
    terminal_cleanup();
    editor_cleanup();
    
    if (g_state.log_fp) {
        log_message("=== EDE v%s shutdown ===", EDE_VERSION);
        fclose(g_state.log_fp);
    }
    
    if (debug_mode) {
        debug_disable();
    }
    
    return 0;
}
