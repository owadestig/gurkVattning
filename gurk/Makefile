CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Isrc -Imock
TEST_DIR = test
WIFI_TARGET = $(TEST_DIR)/test_wifi_manager
HTTP_TARGET = $(TEST_DIR)/test_http_handler
WATER_TARGET = $(TEST_DIR)/test_water_controller
INTEGRATION_TARGET = test_integration
WIFI_SOURCES = $(TEST_DIR)/test_wifi_manager.cpp
HTTP_SOURCES = $(TEST_DIR)/test_http_handler.cpp
WATER_SOURCES = $(TEST_DIR)/test_water_controller.cpp

# Arduino constants for compilation
DEFINES = -DWL_CONNECTED=3 -DWL_DISCONNECTED=6 -DWIFI_OFF=0 -DWIFI_STA=1 -DHTTP_CODE_OK=200 -DHTTP_CODE_MOVED_PERMANENTLY=301 -DHIGH=1 -DLOW=0 -DOUTPUT=1 -DINPUT_PULLUP=2

# Default target
all: test

# Build test executables
$(WIFI_TARGET): $(WIFI_SOURCES)
	@mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(DEFINES) -o $(WIFI_TARGET) $(WIFI_SOURCES)

$(HTTP_TARGET): $(HTTP_SOURCES)
	@mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(DEFINES) -o $(HTTP_TARGET) $(HTTP_SOURCES)

$(WATER_TARGET): $(WATER_SOURCES)
	@mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(DEFINES) -o $(WATER_TARGET) $(WATER_SOURCES)

$(INTEGRATION_TARGET): test/test_integration.cpp test/mock/ArduinoMock.cpp
	$(CXX) $(CXXFLAGS) -I./test -I./.pio/libdeps/modwifi/ArduinoJson/src \
	-o $(INTEGRATION_TARGET) test/test_integration.cpp test/mock/ArduinoMock.cpp

# Run all tests
test: test-wifi test-http test-water test-integration

# Run individual test suites
test-wifi: $(WIFI_TARGET)
	@echo "Running WiFiManager tests..."
	./$(WIFI_TARGET)

test-http: $(HTTP_TARGET)
	@echo "Running HTTPHandler tests..."
	./$(HTTP_TARGET)

test-water: $(WATER_TARGET)
	@echo "Running WaterController tests..."
	./$(WATER_TARGET)

# Integration test with Arduino mock
test-integration: $(INTEGRATION_TARGET)
	@echo "Running Integration Tests..."
	./$(INTEGRATION_TARGET)

# Clean up
clean:
	rm -f $(WIFI_TARGET) $(HTTP_TARGET) $(WATER_TARGET) $(INTEGRATION_TARGET)

# Build PlatformIO project (requires PlatformIO to be installed)
build:
	pio run

# Upload to device (requires PlatformIO and connected device)
upload:
	pio run --target upload

# Monitor serial output (requires PlatformIO)
monitor:
	pio device monitor

# Full cycle: test, build, upload
deploy: test build upload

# Help
help:
	@echo "Available targets:"
	@echo "  test            - Run all unit tests on development machine"
	@echo "  test-wifi       - Run only WiFiManager tests"
	@echo "  test-http       - Run only HTTPHandler tests"
	@echo "  test-water      - Run only WaterController tests"
	@echo "  test-integration - Run integration tests (JSON processing, logic)"
	@echo "  build           - Build the PlatformIO project"
	@echo "  upload          - Upload to ESP8266 device"
	@echo "  monitor         - Monitor serial output"
	@echo "  deploy          - Run tests, build, and upload"
	@echo "  clean           - Clean test artifacts"
	@echo "  help            - Show this help message"

.PHONY: all test test-wifi test-http test-water test-integration clean build upload monitor deploy help