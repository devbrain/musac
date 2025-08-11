/**
 * Musac ImGui Player - Refactored Version
 * 
 * This is a refactored version of the original imgui_player example,
 * demonstrating better code organization and maintainability through:
 * 
 * 1. Separation of Concerns - Each class has a single responsibility
 * 2. Dependency Injection - Components are loosely coupled
 * 3. Clear Abstractions - SDL/Backend details are isolated
 * 4. Modular Design - Easy to extend and maintain
 * 
 * Components:
 * - AudioDeviceManager: Handles device enumeration and switching
 * - StreamManager: Manages audio stream lifecycle
 * - WaveformVisualizer: Handles audio visualization
 * - PlayerUI: Manages all UI rendering
 * - ImGuiPlayer: Coordinates all components
 */

#include "imgui_player.hh"
#include <iostream>

int main(int argc, char** argv) {
    try {
        musac::player::ImGuiPlayer player;
        
        if (!player.init()) {
            std::cerr << "Failed to initialize player\n";
            return 1;
        }
        
        player.run();
        player.shutdown();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}