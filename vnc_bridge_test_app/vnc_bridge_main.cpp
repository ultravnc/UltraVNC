#include "../common/vnc_bridge.h"
#include <iostream>
#include <string>

// Main function for standalone vnc_bridge.exe
int main(int argc, char* argv[]) {
    // Enable console logging for the standalone bridge app
    VncBridge::set_console_logging(true);
    
    if (argc < 3) {
        std::cout << "🌉 VNC BRIDGE - NAT Traversal for VNC (Fixed Ports)" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  Server mode: vnc_bridge server <discovery_code|auto> [vnc_server_ip] [vnc_port]" << std::endl;
        std::cout << "  Client mode: vnc_bridge client <discovery_code>" << std::endl;
        std::cout << std::endl;
        std::cout << "Fixed Ports:" << std::endl;
        std::cout << "  🔌 Bridge Server: Always listens on UDP port 50000" << std::endl;
        std::cout << "  🔌 Bridge Client: Always listens on TCP port 5901" << std::endl;
        std::cout << std::endl;
        std::cout << "Discovery Code:" << std::endl;
        std::cout << "  📝 Format: XXX-XXXX-XXX-XX (last 2 digits = checksum)" << std::endl;
        std::cout << "  🎲 Use 'auto' to generate unique code based on machine ID" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  vnc_bridge server auto 127.0.0.1 5900        # Auto-generate code" << std::endl;
        std::cout << "  vnc_bridge server \"123-4567-890-45\"         # Use specific code" << std::endl;
        std::cout << "  vnc_bridge client \"123-4567-890-45\"         # Connect with code" << std::endl;
        std::cout << std::endl;
        std::cout << "VNC Viewer Connection:" << std::endl;
        std::cout << "  Connect to: localhost:5901 (when using client mode)" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    std::string discovery_code = argv[2];
    
    try {
        if (mode == "server") {
            std::string vnc_server_ip = "127.0.0.1";  // Default
            int vnc_port = 5900;  // Default VNC port
            
            if (argc >= 4) vnc_server_ip = argv[3];
            if (argc >= 5) vnc_port = std::stoi(argv[4]);
            
            // Auto-generate code if "auto" is specified
            if (discovery_code == "auto") {
                discovery_code = "";  // Will trigger auto-generation
            }
            
            // Create bridge in server mode (fixed port 50000)
            VncBridge bridge("server", discovery_code, vnc_server_ip, vnc_port);
            
            std::cout << bridge.get_status() << std::endl;
            
            // Show the discovery code prominently
            std::cout << "\n🔑 DISCOVERY CODE: " << bridge.get_discovery_code() << std::endl;
            std::cout << "📋 Share this code with the client side!" << std::endl;
            std::cout << "💡 Client command: vnc_bridge client \"" << bridge.get_discovery_code() << "\"" << std::endl << std::endl;
            
            bridge.run_server_mode();
            
        } else if (mode == "client") {
            // Create bridge in client mode (fixed port 5901)
            VncBridge bridge("client", discovery_code);
            
            std::cout << bridge.get_status() << std::endl;
            bridge.run_client_mode();
            
        } else {
            std::cerr << "❌ Invalid mode. Use 'server' or 'client'" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
