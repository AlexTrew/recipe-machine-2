# Simple GTK File Browser in C

This project is a minimal GTK-based file browser written in C. It displays a window with a file list and allows basic navigation (open folders, select files).

## Build Instructions

1. Ensure you have GTK 3 development libraries installed:
   ```sh
   sudo apt-get install libgtk-3-dev
   ```
2. Build the project:
   ```sh
   gcc -o gtk_file_browser main.c `pkg-config --cflags --libs gtk+-3.0`
   ```
3. Run the application:
   ```sh
   ./gtk_file_browser
   ```

## Features
- Browse directories
- Select files

---
