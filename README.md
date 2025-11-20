# Terminal Tetris

Tiny curses playground for evolving a Tetris clone. Runs with `ncurses` on macOS/Linux and `pdcurses` on Windows.

## Build & Run

**macOS / Linux**
```
make
./build/terminal_tetris
make test   # logic tests
```

**Windows (MinGW + PDCurses)**
```
set CC=x86_64-w64-mingw32-gcc
mingw32-make LDFLAGS=-lpdcurses
build\terminal_tetris.exe
mingw32-make test
```

`make clean` (or `mingw32-make clean`) removes `build/`.

## Controls

- Arrow keys or A/D: move piece
- Up or W: rotate
- Down or S: soft drop
- Q: quit
```
