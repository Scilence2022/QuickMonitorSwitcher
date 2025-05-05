CC = clang
CFLAGS = -Wall -Wextra -O2
FRAMEWORKS = -framework ApplicationServices -framework Carbon
TARGET = monitor_switcher
PREFIX = /usr/local
APP_NAME = QuickMonitorSwitcher
APP_BUNDLE = $(APP_NAME).app
DMG_NAME = $(APP_NAME)-1.0.dmg

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

app: $(TARGET)
	mkdir -p $(APP_BUNDLE)/Contents/MacOS
	mkdir -p $(APP_BUNDLE)/Contents/Resources
	cp $(TARGET) $(APP_BUNDLE)/Contents/MacOS/
	
	# Create Info.plist
	echo '<?xml version="1.0" encoding="UTF-8"?>' > $(APP_BUNDLE)/Contents/Info.plist
	echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '<plist version="1.0">' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '<dict>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <key>CFBundleExecutable</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <string>monitor_switcher</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <key>CFBundleIdentifier</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <string>com.example.$(APP_NAME)</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <key>CFBundleName</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <string>$(APP_NAME)</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <key>CFBundleVersion</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <string>1.0</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <key>CFBundleShortVersionString</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <string>1.0</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <key>NSHumanReadableCopyright</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <string>Copyright © 2023 $(APP_NAME). All rights reserved.</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <key>LSBackgroundOnly</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '  <true/>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '</dict>' >> $(APP_BUNDLE)/Contents/Info.plist
	echo '</plist>' >> $(APP_BUNDLE)/Contents/Info.plist

install-app: app
	cp -R $(APP_BUNDLE) /Applications/

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