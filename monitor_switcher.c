#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <stdio.h>
#include <stdlib.h>

#define LEFT_ARROW_KEYCODE  0x7B
#define RIGHT_ARROW_KEYCODE 0x7C

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

    // Compute relative position within the current display
    double relX = currentPos.x - currentBounds.origin.x;
    double relY = currentPos.y - currentBounds.origin.y;

    // Compute new absolute position on the target display
    double newX = newBounds.origin.x + relX;
    double newY = newBounds.origin.y + relY;

    // Clamp within the target display
    if (newX < newBounds.origin.x) newX = newBounds.origin.x;
    if (newX > newBounds.origin.x + newBounds.size.width) newX = newBounds.origin.x + newBounds.size.width;
    if (newY < newBounds.origin.y) newY = newBounds.origin.y;
    if (newY > newBounds.origin.y + newBounds.size.height) newY = newBounds.origin.y + newBounds.size.height;

    // Warp the cursor
    CGWarpMouseCursorPosition(CGPointMake(newX, newY));
    CGAssociateMouseAndMouseCursorPosition(true);
}

// Callback for keyboard events
CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo) {
    if (type != kCGEventKeyDown) {
        return event;
    }

    // Get key code and flags
    CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    CGEventFlags flags = CGEventGetFlags(event);

    // Check for Command + Left or Command + Right without other modifiers
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