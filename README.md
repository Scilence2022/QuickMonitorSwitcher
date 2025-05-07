# QuickMonitorSwitcher

A simple macOS utility that allows you to move your cursor between multiple displays using keyboard shortcuts.

## Features

- Quickly move your cursor to the same relative position on adjacent monitors
- Uses Command+Left Arrow to move cursor to the left monitor
- Uses Command+Right Arrow to move cursor to the right monitor
- Preserves the relative position of the cursor when switching displays

## Requirements

- macOS
- Xcode Command Line Tools or full Xcode installation

## Installation

### Building from source

1. Clone this repository
   ```
   git clone https://github.com/yourusername/QuickMonitorSwitcher.git
   cd QuickMonitorSwitcher
   ```

2. Build the project
   ```
   make
   ```

3. (Optional) Install system-wide
   ```
   sudo make install
   ```

### Running

1. For first-time use, you need to grant accessibility permissions:
   - Go to System Settings → Privacy & Security → Accessibility
   - Add and enable the `monitor_switcher` application

2. Run the application:
   ```
   ./monitor_switcher
   ```
   
   If installed system-wide:
   ```
   monitor_switcher
   ```

3. The application will run in the foreground. To stop it, press Ctrl+C.

## Usage

- **Command + Left Arrow**: Move cursor to the left display
- **Command + Right Arrow**: Move cursor to the right display

## License

This project is available under the GNU General Public License v3.0. See LICENSE file for details.