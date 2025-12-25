#!/bin/bash
# Simple HTTP server launcher for the WebUSB flasher
# This script starts a local web server to serve the flasher interface

cd "$(dirname "$0")"

echo "🌊 Starting Nautilus WebUSB Flasher server..."
echo ""
echo "Server will be available at:"
echo "  http://localhost:8000"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Try python3 first, then python, then node
if command -v python3 &> /dev/null; then
    python3 -m http.server 8000
elif command -v python &> /dev/null; then
    python -m http.server 8000
elif command -v npx &> /dev/null; then
    npx http-server -p 8000
else
    echo "Error: No suitable web server found!"
    echo "Please install Python 3 or Node.js to run the server."
    exit 1
fi
