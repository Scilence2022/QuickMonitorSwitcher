#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>  // For UINT16_MAX & PATH_MAX (or MAXPATHLEN)
#include <mach-o/dyld.h> // For _NSGetExecutablePath
#include <libgen.h>      // For dirname
#include <unistd.h>      // For readlink (optional, for resolving symlinks)

#define LEFT_ARROW_KEYCODE  0x7B
#define RIGHT_ARROW_KEYCODE 0x7C

// Global hotkey settings (modifiable via config.ini)
static CGEventFlags gSwitchModifiersRequired   = kCGEventFlagMaskControl;
static CGEventFlags gSwitchModifiersForbidden = kCGEventFlagMaskShift | kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand;
static CGKeyCode    gSwitchKeyCode            = 0x31;  // space
static CGEventFlags gExitModifiersRequired     = kCGEventFlagMaskControl | kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand;
static CGKeyCode    gExitKeyCode               = 0x0C;  // Q

// Map single character to CGKeyCode (ANSI US layout)
static CGKeyCode keycodeForChar(char c) {
    switch (c) {
        case 'A': return 0x00;  case 'S': return 0x01;
        case 'D': return 0x02;  case 'F': return 0x03;
        case 'H': return 0x04;  case 'G': return 0x05;
        case 'Z': return 0x06;  case 'X': return 0x07;
        case 'C': return 0x08;  case 'V': return 0x09;
        case 'B': return 0x0B;  case 'Q': return 0x0C;
        case 'W': return 0x0D;  case 'E': return 0x0E;
        case 'R': return 0x0F;  case 'Y': return 0x10;
        case 'T': return 0x11;  case '1': return 0x12;
        case '2': return 0x13;  case '3': return 0x14;
        case '4': return 0x15;  case '6': return 0x16;
        case '5': return 0x17;  case '=': return 0x18;
        case '9': return 0x19;  case '7': return 0x1A;
        case '-': return 0x1B;  case '8': return 0x1C;
        case '0': return 0x1D;  case 'O': return 0x1F;
        case 'U': return 0x20;  case 'I': return 0x22;
        case 'P': return 0x23;  case 'L': return 0x25;
        case 'J': return 0x26;  case 'K': return 0x28;
        case 'N': return 0x2D;  case 'M': return 0x2E;
        case ',': return 0x2B;  case '.': return 0x2F;
        case '/': return 0x2C;  case ';': return 0x29;
        case '\'': return 0x27;  case '\\': return 0x2A;
        case '`': return 0x32;  case ' ': return 0x31;
        default: return (CGKeyCode)UINT16_MAX;
    }
}

// Function to display a notification with cursor position
void notifyCursorPosition() {
    CGEventRef tempEvent = CGEventCreate(NULL);
    if (!tempEvent) return;
    CGPoint cursorPos = CGEventGetLocation(tempEvent);
    CFRelease(tempEvent);

    char notificationCmd[256];
    // Use snprintf to prevent buffer overflows
    snprintf(notificationCmd, sizeof(notificationCmd),
             "osascript -e 'display notification \"Cursor at X: %.0f, Y: %.0f\" with title \"QuickMonitorSwitcher\"' &> /dev/null",
             cursorPos.x, cursorPos.y);
    system(notificationCmd);
}

// Parse hotkey string of form "Modifier+Key" into modifiers mask and keycode
void parse_hotkey(const char *str, CGEventFlags *modifiers, CGKeyCode *keycode) {
    char buf[256];
    strncpy(buf, str, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    // Remove newline
    char *newline = strchr(buf, '\n');
    if (newline) *newline = '\0';
    // Trim leading and trailing whitespace
    char *p = buf;
    while (isspace((unsigned char)*p)) p++;
    char *endp = p + strlen(p) - 1;
    while (endp > p && isspace((unsigned char)*endp)) *endp-- = '\0';
    *modifiers = 0;
    *keycode = 0;
    // Tokenize on '+'
    char *token = strtok(p, "+");
    char *last = NULL;
    while (token) {
        // Trim whitespace around token
        char *t = token;
        while (isspace((unsigned char)*t)) t++;
        char *te = t + strlen(t) - 1;
        while (te > t && isspace((unsigned char)*te)) *te-- = '\0';
        last = t;
        if (strcasecmp(t, "Control") == 0) *modifiers |= kCGEventFlagMaskControl;
        else if (strcasecmp(t, "Shift") == 0) *modifiers |= kCGEventFlagMaskShift;
        else if (strcasecmp(t, "Option") == 0 || strcasecmp(t, "Alt") == 0) *modifiers |= kCGEventFlagMaskAlternate;
        else if (strcasecmp(t, "Command") == 0 || strcasecmp(t, "Cmd") == 0) *modifiers |= kCGEventFlagMaskCommand;
        token = strtok(NULL, "+");
    }
    if (last) {
        if (strcasecmp(last, "Space") == 0) {
            *keycode = keycodeForChar(' ');
        } else if (strcasecmp(last, "Left") == 0) {
            *keycode = LEFT_ARROW_KEYCODE;
        } else if (strcasecmp(last, "Right") == 0) {
            *keycode = RIGHT_ARROW_KEYCODE;
        } else if (strlen(last) == 1) {
            char c = toupper(last[0]);
            *keycode = keycodeForChar(c);
        }
    }
}

// Load configuration from config.ini; defaults used if missing
void loadConfig() {
    char exe_path[PATH_MAX];
    uint32_t len = sizeof(exe_path);
    char config_file_path[PATH_MAX];

    // Default to current directory
    strncpy(config_file_path, "config.ini", PATH_MAX -1);
    config_file_path[PATH_MAX-1] = '\0';

    if (_NSGetExecutablePath(exe_path, &len) == 0) {
        // Make a copy for dirname as it might modify the string
        char exe_path_copy_for_macos_dir[PATH_MAX];
        strncpy(exe_path_copy_for_macos_dir, exe_path, PATH_MAX - 1);
        exe_path_copy_for_macos_dir[PATH_MAX-1] = '\0';
        
        char *mac_os_dir = dirname(exe_path_copy_for_macos_dir); // .../AppName.app/Contents/MacOS

        // Make another copy for the next dirname
        char exe_path_copy_for_contents_dir[PATH_MAX];
        strncpy(exe_path_copy_for_contents_dir, mac_os_dir, PATH_MAX - 1);
        exe_path_copy_for_contents_dir[PATH_MAX-1] = '\0';
        
        char *contents_dir = dirname(exe_path_copy_for_contents_dir); // .../AppName.app/Contents

        if (strcmp(basename(mac_os_dir), "MacOS") == 0 && strcmp(basename(dirname(contents_dir)), "Contents") == 0) {
            // Likely running from an app bundle
            snprintf(config_file_path, sizeof(config_file_path), "%s/Resources/config.ini", contents_dir);
        } else {
             // Not in a typical App bundle structure, or _NSGetExecutablePath gave an unexpected path.
             // Keep config_file_path as "config.ini" (current directory)
             fprintf(stderr, "Not running in a standard app bundle, or path unexpected. Looking for config.ini in current directory.\n");
        }
    } else {
        fprintf(stderr, "Could not get executable path using _NSGetExecutablePath. Looking for config.ini in current directory.\n");
        // Fallback: config_file_path is already "config.ini"
    }
    
    fprintf(stdout, "Attempting to load config from: %s\n", config_file_path);
    FILE *f = fopen(config_file_path, "r");
    if (!f) {
        fprintf(stderr, "Failed to open config file: %s. Using default settings.\n", config_file_path);
        return; // Defaults are already set in global variables
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        while (*key == ' ' || *key == '\t') key++;
        char *end = key + strlen(key) - 1;
        while (end > key && (*end == ' ' || *end == '\t')) *end-- = '\0';
        while (*val == ' ' || *val == '\t') val++;
        char *vn = strchr(val, '\n'); if (vn) *vn = '\0';
        if (strcasecmp(key, "switch_hotkey") == 0) {
            parse_hotkey(val, &gSwitchModifiersRequired, &gSwitchKeyCode);
        } else if (strcasecmp(key, "exit_hotkey") == 0) {
            parse_hotkey(val, &gExitModifiersRequired, &gExitKeyCode);
        }
    }
    fclose(f);
}

// Switch cursor position by one display in the given direction (-1 = left, +1 = right)
void switchDisplay(int direction) {
    // Get list of active displays
    uint32_t maxDisplays = 16;
    uint32_t displayCount = 0;
    CGDirectDisplayID displays[16];
    CGError err = CGGetActiveDisplayList(maxDisplays, displays, &displayCount);
    if (err != kCGErrorSuccess || displayCount < 2) {
        return;
    }

    // Get current cursor position
    CGEventRef mouseEvent = CGEventCreate(NULL);
    CGPoint currentPos = CGEventGetLocation(mouseEvent);
    CFRelease(mouseEvent);

    // Find which display the cursor is currently on
    int currentIndex = 0;
    CGRect currentBounds = CGDisplayBounds(displays[0]);
    for (uint32_t i = 0; i < displayCount; ++i) {
        CGRect bounds = CGDisplayBounds(displays[i]);
        if (CGRectContainsPoint(bounds, currentPos)) {
            currentIndex = i;
            currentBounds = bounds;
            break;
        }
    }

    // Calculate the next display index
    int newIndex = (currentIndex + direction + displayCount) % displayCount;
    CGRect newBounds = CGDisplayBounds(displays[newIndex]);

    // Compute proportional position within the current display
    double fracX = 0.0, fracY = 0.0;
    if (currentBounds.size.width > 0)
        fracX = (currentPos.x - currentBounds.origin.x) / currentBounds.size.width;
    if (currentBounds.size.height > 0)
        fracY = (currentPos.y - currentBounds.origin.y) / currentBounds.size.height;

    // Compute new absolute position proportionally on the target display
    double newX = newBounds.origin.x + fracX * newBounds.size.width;
    double newY = newBounds.origin.y + fracY * newBounds.size.height;

    // Clamp within the target display
    if (newX < newBounds.origin.x) newX = newBounds.origin.x;
    if (newX > newBounds.origin.x + newBounds.size.width) newX = newBounds.origin.x + newBounds.size.width;
    if (newY < newBounds.origin.y) newY = newBounds.origin.y;
    if (newY > newBounds.origin.y + newBounds.size.height) newY = newBounds.origin.y + newBounds.size.height;

    // Warp the cursor
    CGWarpMouseCursorPosition(CGPointMake(newX, newY));
    CGAssociateMouseAndMouseCursorPosition(true);

    notifyCursorPosition(); // Notify cursor position after switching
}

// Callback for keyboard events
CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo) {
    if (type != kCGEventKeyDown) {
        return event;
    }

    CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    CGEventFlags flags = CGEventGetFlags(event);

    // Handle Control+Space for next display
    if ((flags & gSwitchModifiersRequired) == gSwitchModifiersRequired &&
        !(flags & gSwitchModifiersForbidden) &&
        keyCode == gSwitchKeyCode) {
        switchDisplay(1);
        return NULL; // consume the event
    }

    // Handle Control+Option+Command+Q to quit the application
    if ((flags & gExitModifiersRequired) == gExitModifiersRequired && keyCode == gExitKeyCode) {
        CFRunLoopStop(CFRunLoopGetCurrent());
        return NULL;
    }

    // Existing Command+Left/Right handling
    if ((flags & kCGEventFlagMaskCommand) && !(flags & (kCGEventFlagMaskShift | kCGEventFlagMaskAlternate | kCGEventFlagMaskControl))) {
        if (keyCode == LEFT_ARROW_KEYCODE) {
            switchDisplay(-1);
            return NULL; // consume the event
        } else if (keyCode == RIGHT_ARROW_KEYCODE) {
            switchDisplay(1);
            return NULL; // consume the event
        }
    }

    return event;
}

int main(void) {
    loadConfig();
    // Create an event tap to capture keydown events
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown);
    CFMachPortRef eventTap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        eventTapCallback,
        NULL
    );

    if (!eventTap) {
        fprintf(stderr, "Failed to create event tap.\n");
        return EXIT_FAILURE;
    }

    // Create a run loop source and add to the current run loop
    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);

    // Run the loop
    CFRunLoopRun();

    // Cleanup (unreachable in this simple program)
    CFRelease(runLoopSource);
    CFRelease(eventTap);

    return EXIT_SUCCESS;
}