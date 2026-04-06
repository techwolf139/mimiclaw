#!/bin/bash
# Deploy UI Redesign to ESP32-S3
# Usage: ./deploy_ui.sh [PORT]
#   PORT: Serial port (e.g., /dev/cu.usbmodem11401)

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== MimiClaw UI Redesign Deployment ===${NC}"
echo "Date: $(date)"
echo "Project: $(pwd)"

# Check if we're in the project directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "main" ]; then
    echo -e "${RED}Error: Must run from project root directory${NC}"
    exit 1
fi

# Check ESP-IDF environment
if [ -z "$IDF_PATH" ]; then
    if [ -f "/Users/mac/.espressif/esp-idf-v5.5.2/export.sh" ]; then
        echo -e "${YELLOW}ESP-IDF environment not set. Sourcing export.sh...${NC}"
        source "/Users/mac/.espressif/esp-idf-v5.5.2/export.sh" > /dev/null 2>&1
        echo "ESP-IDF version: $(idf.py --version | head -1)"
    else
        echo -e "${RED}Error: ESP-IDF not found. Run:${NC}"
        echo "  ./scripts/setup_idf_macos.sh"
        echo "  source ~/.espressif/esp-idf-v5.5.2/export.sh"
        exit 1
    fi
fi

# Set target if not already set
if ! idf.py --list-targets 2>/dev/null | grep -q esp32s3; then
    echo -e "${YELLOW}Setting target to esp32s3...${NC}"
    idf.py set-target esp32s3
fi

# Check build status
if [ ! -f "build/mimiclaw.bin" ]; then
    echo -e "${YELLOW}Firmware not built. Building...${NC}"
    echo "This may take several minutes..."
    idf.py build
else
    echo -e "${GREEN}Firmware already built: build/mimiclaw.bin${NC}"
    ls -lh build/mimiclaw.bin
fi

# Check binary size
BIN_SIZE=$(stat -f%z build/mimiclaw.bin 2>/dev/null || wc -c < build/mimiclaw.bin)
echo "Binary size: $((BIN_SIZE/1024)) KB"

# Determine serial port
PORT="$1"
if [ -z "$PORT" ]; then
    echo -e "${YELLOW}No port specified. Searching for ESP32-S3...${NC}"
    
    # Try common macOS ports
    for p in /dev/cu.usbmodem* /dev/cu.usbserial* /dev/tty.usbmodem* /dev/tty.usbserial*; do
        if [ -c "$p" ]; then
            PORT="$p"
            echo "Found possible port: $PORT"
            break
        fi
    done
    
    if [ -z "$PORT" ]; then
        echo -e "${RED}No serial ports found. Please connect ESP32-S3 and specify port:${NC}"
        echo "  ./deploy_ui.sh /dev/cu.usbmodem11401"
        echo ""
        echo -e "${YELLOW}Check available ports:${NC}"
        ls /dev/cu.* 2>/dev/null || echo "No serial devices"
        exit 1
    fi
fi

# Verify port exists
if [ ! -c "$PORT" ]; then
    echo -e "${RED}Error: Port '$PORT' not found${NC}"
    echo "Available ports:"
    ls /dev/cu.* 2>/dev/null | grep -v Bluetooth || echo "None found"
    exit 1
fi

# Get user confirmation
echo ""
echo -e "${GREEN}=== Deployment Summary ==="
echo "Port:          $PORT"
echo "Firmware:      build/mimiclaw.bin ($((BIN_SIZE/1024)) KB)"
echo "Target:        esp32s3"
echo "UI Version:    Redesign with screen manager & 5 screens"
echo "==============${NC}"
echo ""
read -p "Proceed with flash? (y/N): " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}Deployment cancelled${NC}"
    exit 0
fi

# Flash the firmware
echo -e "${GREEN}Flashing to $PORT...${NC}"
echo "This will take 30-60 seconds..."
idf.py -p "$PORT" flash monitor

echo -e "${GREEN}=== Deployment Complete ==="
echo "UI Redesign deployed successfully!"
echo "Connect via serial monitor to test:"
echo "  idf.py -p $PORT monitor"
echo "Or use CLI commands:"
echo "  mimi> config_show"
echo "  mimi> wifi_status${NC}"