CC = clang
CFLAGS = -Wall -Wextra -O2
FRAMEWORKS = -framework ApplicationServices -framework Carbon
TARGET = monitor_switcher
PREFIX = /usr/local
APP_NAME = QuickMonitorSwitcher
APP_BUNDLE = $(APP_NAME).app
DMG_NAME = $(APP_NAME)-1.0.dmg

# Define directories within the .app bundle
APP_CONTENTS_DIR = $(APP_BUNDLE)/Contents
APP_MACOS_DIR = $(APP_CONTENTS_DIR)/MacOS
APP_RESOURCES_DIR = $(APP_CONTENTS_DIR)/Resources

all: $(TARGET)

$(TARGET): monitor_switcher.c
	$(CC) $(CFLAGS) $(FRAMEWORKS) $< -o $@

clean:
	rm -f $(TARGET)
	rm -rf $(APP_BUNDLE)
	rm -f $(DMG_NAME)

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	cp $(TARGET) $(PREFIX)/bin/
	chmod 755 $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
	rm -rf /Applications/$(APP_BUNDLE)

# App target now depends on the executable, Info.plist, and config.ini
app: $(APP_MACOS_DIR)/$(TARGET) $(APP_CONTENTS_DIR)/Info.plist $(APP_RESOURCES_DIR)/config.ini

# Rule to create directories and copy the executable
$(APP_MACOS_DIR)/$(TARGET): $(TARGET)
	mkdir -p $(APP_MACOS_DIR)
	cp $(TARGET) $(APP_MACOS_DIR)/

# Rule to create directories and copy Info.plist
$(APP_CONTENTS_DIR)/Info.plist: Info.plist
	mkdir -p $(APP_CONTENTS_DIR)
	cp Info.plist $(APP_CONTENTS_DIR)/

# Rule to create directories and copy config.ini
$(APP_RESOURCES_DIR)/config.ini: config.ini
	mkdir -p $(APP_RESOURCES_DIR)
	cp config.ini $(APP_RESOURCES_DIR)/

install-app: app
	# Check if /Applications directory exists and is writable, or use sudo
	# For simplicity, this example still uses sudo. Consider user-specific install for non-admin.
	# Also, it might be better to remove an existing app first or use rsync.
	@echo "Installing $(APP_BUNDLE) to /Applications/..."
	# The cp command might fail if the app is already running.
	# Consider asking the user to quit the app first if it exists.
	if [ -d "/Applications/$(APP_BUNDLE)" ]; then \
		rm -rf "/Applications/$(APP_BUNDLE)"; \
	fi
	cp -R $(APP_BUNDLE) /Applications/
	@echo "Installation complete. You might need to grant Accessibility permissions again for the new app location."

dmg: app
	# Create temporary directory for DMG contents
	mkdir -p dmg_tmp
	cp -R $(APP_BUNDLE) dmg_tmp/
	cp README.md dmg_tmp/
	cp LICENSE dmg_tmp/
	
	# Create the DMG
	hdiutil create -volname "$(APP_NAME)" -srcfolder dmg_tmp -ov -format UDZO $(DMG_NAME)
	
	# Clean up
	rm -rf dmg_tmp

sign: app
	@echo "To sign the application, uncomment and modify the line below:"
	@echo "# codesign --force --deep --sign 'Developer ID Application: Your Name' $(APP_BUNDLE)"

launch: app
	open $(APP_BUNDLE)

.PHONY: all clean install uninstall app install-app dmg sign launch 