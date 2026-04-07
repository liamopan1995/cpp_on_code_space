#!/bin/bash

# Script to install OpenSpec CLI

echo "Installing OpenSpec..."

npm install -g @fission-ai/openspec

if [ $? -eq 0 ]; then
    echo "OpenSpec installed successfully."
else
    echo "Installation failed."
fi