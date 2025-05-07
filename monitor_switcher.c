#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <stdio.h>
#include <stdlib.h>

#define LEFT_ARROW_KEYCODE  0x7B
#define RIGHT_ARROW_KEYCODE 0x7C

// Define new hotkeys: Control+Space for next display, Q for exit
#define HOTKEY_SWITCH_NEXT_MODIFIERS_REQUIRED kCGEventFlagMaskControl
#define HOTKEY_SWITCH_NEXT_MODIFIERS_FORBIDDEN (kCGEventFlagMaskShift | kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand)
#define HOTKEY_SWITCH_NEXT_KEYCODE   0x31  // space key
#define HOTKEY_EXIT_MODIFIERS        (kCGEventFlagMaskControl | kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand)
#define HOTKEY_EXIT_KEYCODE          0x0C  // 'Q' key

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
    if ((flags & HOTKEY_SWITCH_NEXT_MODIFIERS_REQUIRED) == HOTKEY_SWITCH_NEXT_MODIFIERS_REQUIRED &&
        !(flags & HOTKEY_SWITCH_NEXT_MODIFIERS_FORBIDDEN) &&
        keyCode == HOTKEY_SWITCH_NEXT_KEYCODE) {
        switchDisplay(1);
        return NULL; // consume the event
    }

    // Handle Control+Option+Command+Q to quit the application
    if ((flags & HOTKEY_EXIT_MODIFIERS) == HOTKEY_EXIT_MODIFIERS && keyCode == HOTKEY_EXIT_KEYCODE) {
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