# QuickMonitorSwitcher

**GitHub Repository: [https://github.com/Scilence2022/QuickMonitorSwitcher.git](https://github.com/Scilence2022/QuickMonitorSwitcher.git)**

A simple macOS utility that allows you to move your cursor between multiple displays using keyboard shortcuts. This version supports configurable hotkeys via a `config.ini` file and can be packaged as a standard macOS `.app` application.

## Features

- Quickly move your cursor to a proportional position on adjacent monitors (handles different display sizes correctly).
- Default hotkeys:
    - **Command + Left Arrow**: Move cursor to the left display.
    - **Command + Right Arrow**: Move cursor to the right display.
- Configurable hotkeys via `config.ini` for:
    - Switching to the next display (e.g., `Control+D`).
    - Exiting the application (e.g., `Control+Option+Command+Q`).
- Displays a system notification with the cursor's new coordinates after each switch.
- Can be run as a command-line tool or packaged as a `.app` application.
- Supports a custom application icon (`AppIcon.icns`) when packaged as an `.app`.
- Runs in the background without a Dock icon when launched as an `.app`.

## Requirements

- macOS
- Xcode Command Line Tools (for building from source)

## Installation

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/Scilence2022/QuickMonitorSwitcher.git
    cd QuickMonitorSwitcher
    ```

### Option 1: Building and Running the Command-Line Tool

1.  **Build the executable:**
    ```bash
    make
    ```
2.  **(Optional) Install system-wide:**
    ```bash
    sudo make install
    ```
    This will copy the `monitor_switcher` executable to `/usr/local/bin/`.

### Option 2: Building and Running the .app Bundle

1.  **(Optional) Prepare a custom icon:**
    *   Create a 1024x1024 PNG icon.
    *   Convert it to `AppIcon.icns` using macOS's `iconutil` or an online converter.
    *   Place `AppIcon.icns` in the project's root directory. If not provided, a default system icon will be used.
2.  **Build the .app bundle:**
    ```bash
    make app
    ```
    This creates `QuickMonitorSwitcher.app` in the project directory.
3.  **(Optional) Install the .app to Applications folder:**
    ```bash
    make install-app
    ```
    This copies `QuickMonitorSwitcher.app` to your `/Applications` folder (may require `sudo`).

## Configuration (`config.ini`)

The application loads hotkey settings from a `config.ini` file located in the same directory as the executable (for command-line tool) or in `QuickMonitorSwitcher.app/Contents/Resources/` (for the .app bundle).

If `config.ini` is not found, default hotkeys are used.

**Example `config.ini`:**
```ini
; QuickMonitorSwitcher configuration
; Specify hotkey combinations as Modifier+Key, separated by '+'
; Available modifiers: Control, Shift, Option (or Alt), Command (or Cmd)
; Available keys: A-Z, 0-9, Space, Left, Right. (More can be added in keycodeForChar function in .c)

switch_hotkey=Control+D
exit_hotkey=Control+Option+Command+Q
```

-   `switch_hotkey`: Defines the hotkey to switch to the next display (cycles through available displays).
-   `exit_hotkey`: Defines the hotkey to quit the application.

## Running the Application

### Command-Line Tool

1.  **Grant Accessibility Permissions:**
    *   Go to System Settings → Privacy & Security → Accessibility.
    *   Add and enable the `monitor_switcher` executable (e.g., located in the project directory or `/usr/local/bin/` if installed).
2.  **Run from the project directory:**
    ```bash
    ./monitor_switcher
    ```
    Or, if installed system-wide:
    ```bash
    monitor_switcher
    ```
3.  The application runs in the foreground. Press `Ctrl+C` in the terminal to stop it, or use the configured `exit_hotkey`.

### .app Bundle

1.  **Grant Accessibility Permissions:**
    *   Go to System Settings → Privacy & Security → Accessibility.
    *   Add and enable `QuickMonitorSwitcher.app` (e.g., from the project directory or `/Applications`).
2.  **Launch the application:**
    *   Double-click `QuickMonitorSwitcher.app`.
3.  The application runs silently in the background. Use the configured `exit_hotkey` to quit.

## Usage

-   Use **Command + Left Arrow** or **Command + Right Arrow** to move the cursor to the previous or next display, respectively.
-   Use the hotkey defined by `switch_hotkey` in `config.ini` (default: `Control+D` if you used the example) to cycle to the next display.
-   Use the hotkey defined by `exit_hotkey` in `config.ini` (default: `Control+Option+Command+Q`) to quit the application.

## License

This project is available under the GNU General Public License v3.0. See `LICENSE` file for details.