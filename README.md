# GNU ede v1.0
**Enhanced Development Editor** - A powerful nano-like terminal text editor with advanced features

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)]()
[![License](https://img.shields.io/badge/license-GPL%20v3-green)]()

## Features

### Core Capabilities
- ✅ **Bare metal terminal control** - No ncurses, windows.h, or conio.h dependencies
- ✅ **Cross-platform** - Windows, Linux, macOS, and Android support
- ✅ **GNU GPL v3** licensed
- ✅ **Syntax highlighting** - C/C++, Python, JavaScript
- ✅ **Arrow key navigation** with Home/End/PageUp/PageDown support

### Advanced Module System
- 🚀 **Hyper-advanced scripting language** with tokenizer & interpreter
- 🔌 **30+ built-in commands** for editor automation
- 📦 **Module compilation** - `.esrc` → `.emod` format
- 🎯 **Module API** with 15+ functions for editor control
- ⚡ **Dynamic loading** - Load/unload modules at runtime

### Editing Features
- 📝 **Search & Replace** - Case-sensitive, whole-word matching
- 📋 **Clipboard** - Copy, cut, paste operations (Ctrl-U, Ctrl-K, Ctrl-V)
- 🔄 **Undo/Redo** - Full history with Ctrl-Z/Ctrl-Y
- 🎯 **Autocomplete** - Smart word completion (Ctrl-T)
- 🎬 **Macro recording** - Record and playback key sequences
- 👆 **Multi-cursor editing** - Edit multiple locations (Ctrl-D)
- 📑 **Bookmarks** - Quick navigation markers (Ctrl-B)
- 📂 **Code folding** - Collapse/expand code blocks (Ctrl-G)
- 🔀 **Split views** - Horizontal/vertical splits (Ctrl-W)
- 💡 **Smart indentation** - Auto-indent with brace detection

### Vim Mode
Enable with **Ctrl-C → Ctrl-M**:
- `:q`, `:q!` - Quit commands
- `:w`, `:wq`, `:x` - Save commands  
- `:e filename` - Open files
- `:/search`, `:?search` - Search
- `:s/old/new/` - Find & replace
- `:42` - Jump to line number
- `:set nu` - Toggle line numbers
- `:help` - Show help

### Developer Tools
- 🔍 **Git integration** - Blame, diff, status
- 📊 **Diff viewer** - Side-by-side comparison
- 🔌 **LSP hooks** - Language server protocol support
- 💻 **Terminal emulator** - Built-in terminal
- 💾 **Session management** - Save/restore editor state
- 📁 **File browser** - Navigate directories

## Installation

### Compile from source:
```bash
git clone https://github.com/srhs1723/ede.git
cd ede
gcc ede.c -o ede
```

### Windows:
```powershell
gcc ede.c -o ede.exe
```

## Usage

```bash
# Open a file
./ede filename.txt

# Show help
./ede --help

# Show version
./ede --version

# Compile a module
./ede module.esrc -o module.emod

# Load a module
./ede -m module.emod filename.txt
```

## Keybindings

| Key | Action |
|-----|--------|
| **Ctrl-Q** | Quit (prompts to save) |
| **Ctrl-S** | Save file |
| **Ctrl-F** | Find text |
| **Ctrl-R** | Replace text |
| **Ctrl-N/P** | Next/previous search result |
| **Ctrl-Z/Y** | Undo/Redo |
| **Ctrl-K** | Cut line |
| **Ctrl-U** | Copy line |
| **Ctrl-V** | Paste |
| **Ctrl-T** | Autocomplete |
| **Ctrl-W** | Toggle split view focus |
| **Ctrl-B** | Add bookmark |
| **Ctrl-G** | Toggle code folding |
| **Ctrl-D** | Add multi-cursor |
| **Ctrl-C → Ctrl-M** | Toggle vim mode |
| **Arrow keys** | Navigate |
| **Home/End** | Line start/end |
| **PageUp/Down** | Scroll |

## Module System

### Example Module (`hello.esrc`)
```esrc
// Print messages
print("Hello from ede module!");

// Get editor info
editor linecount;
editor filename;

// Search and replace
search("test");
replace("old", "new");

// Execute shell commands
exec("echo Success!");
```

### Compile & Load
```bash
./ede hello.esrc -o hello.emod
./ede -m hello.emod myfile.txt
```

### Module API Functions
- `print(text)` - Display message
- `editor linecount` - Get line count
- `editor filename` - Get filename  
- `search(pattern)` - Find matches
- `replace(old, new)` - Replace all
- `exec(command)` - Run shell command
- `set variable = value` - Variables
- Control flow: `if`, `else`, `while`, `for`

## Building

Requires only standard C compiler (gcc, clang, msvc):
- C99 or later
- No external dependencies
- ~3500 lines of code
- Cross-platform compatible

## Architecture

- **Bare metal terminal control** - Manual OS structure definitions
- **Custom getline** - Windows compatibility layer
- **VT sequence parsing** - Full escape sequence support
- **Zero dependencies** - Standalone binary

## License

GNU General Public License v3.0

Copyright (C) 2025 Free Software Foundation, Inc.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Credits

Developed as a modern, feature-rich alternative to nano with vim-like capabilities and a powerful module system.

---

**GNU ede v1.0** - Enhanced Development Editor
