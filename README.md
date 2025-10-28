# GNU ede - Advanced Modular Text Editor

![Version](https://img.shields.io/badge/version-1.0-blue)
![License](https://img.shields.io/badge/license-GPL--3.0-green)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20Termux-lightgrey)

**ede** (pronounced "eddy") is an advanced, modular terminal-based text editor with plugin support, Git integration, and a custom scripting language. Think **nano meets vim with a plugin system** - all in a single 4700-line C file!

```
 _____ ____  _____ 
| ____|  _ \| ____|
|  _| | | | |  _|  
| |___| |_| | |___ 
|_____|____/|_____|
                   
GNU ede v1.0
```

## 🚀 Features

### Core Editing
- ✅ **Multi-file, multi-tab editing** - Work on multiple files simultaneously
- ✅ **Gap buffer** - Efficient text manipulation
- ✅ **Undo/Redo** - 100 levels of history
- ✅ **Visual mode** - Vim-like selection (character, line, block)
- ✅ **Syntax highlighting** - C, C++, Python, Java, JavaScript, HTML, CSS, JSON, XML, Markdown
- ✅ **Line numbers** - Optional line number display
- ✅ **Auto-indent** - Smart indentation

### Advanced Features
- 🔌 **Plugin/Module system** - Load DLLs to extend functionality
- 📜 **ESRC language** - Custom scripting language that compiles to modules
- 🔍 **Search & Replace** - With case-sensitive and regex support
- 📑 **Bookmarks** - Mark and jump to lines
- 🎬 **Macro recording** - Record and replay key sequences
- 💡 **Auto-complete** - Context-aware code completion
- 📝 **Snippets** - Predefined code templates
- 📂 **File browser** - Navigate directories visually
- 🎨 **Themes** - Multiple color schemes

### Git Integration
- 🔀 **Git status** - View repository status
- 📊 **Git diff** - See file changes
- 💾 **Git commit** - Commit from within editor
- 🌿 **Branch display** - Shows current branch in status bar

### Developer Tools
- 🔨 **Build system** - Execute build commands
- 🔍 **Diff tool** - Compare two files
- 🏷️ **CTags integration** - Symbol navigation
- 📦 **Project templates** - Quick project scaffolding
- 🗂️ **Workspace management** - Save and restore sessions
- 📈 **Performance monitoring** - Track editor statistics

## 📦 Installation

### Linux

```bash
git clone https://github.com/srhs1723/ede.git
cd ede
make
sudo make install
```

### Windows

```powershell
git clone https://github.com/srhs1723/ede.git
cd ede
mingw32-make -f Makefile.win
```

### Termux (Android)

```bash
pkg install git clang make
git clone https://github.com/srhs1723/ede.git
cd ede
make
```

## 🎮 Usage

```bash
# Open a file
ede filename.txt

# Open multiple files
ede file1.c file2.c file3.c

# Load a module
ede -m mymodule.emod file.txt

# Compile ESRC module
ede module.esrc -o module.emod
```

### Keyboard Shortcuts

- `Ctrl+N` - New file
- `Ctrl+O` - Open file
- `Ctrl+S` - Save file
- `Ctrl+Q` - Quit
- `Ctrl+Z` - Undo
- `Ctrl+Y` - Redo
- `Ctrl+F` - Find
- `Ctrl+T` - New tab
- `v` - Visual mode

### Commands

```
:save, :w          Save
:quit, :q          Quit
:wq                Save and quit
:help              Show help
:git status        Git status
:build CMD         Run build
```

## 🔌 Module System

ede supports loadable modules with full API access:

```c
// example.esrc
function hello {
    insert_text("Hello from ESRC!\n");
}
```

Compile: `ede example.esrc -o example.emod`

## ⚙️ Configuration

Create `~/.ederc`:

```ini
show_line_numbers=true
syntax_highlighting=true
tab_width=4
```

## 📄 License

GNU General Public License v3.0

Copyright (C) 2025 Free Software Foundation, Inc.

## 🤝 Contributing

Contributions welcome! Open an issue or PR.

---

**Made with 🔥 and ☕**
