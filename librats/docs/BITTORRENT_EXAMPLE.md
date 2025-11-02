# BitTorrent Integration in LibRats

## Features

- Complete BitTorrent protocol implementation
- DHT integration for peer discovery
- Torrent file parsing and validation
- Piece verification and file management
- Download progress tracking
- Multi-file torrent support
- Peer connection management

## Basic Usage

### 1. Enable BitTorrent

```cpp
#include "librats.h"

librats::RatsClient client(8080);  // Create RatsClient on port 8080

// Start the RatsClient first
if (!client.start()) {
    std::cerr << "Failed to start RatsClient" << std::endl;
    return -1;
}

// Enable BitTorrent functionality (optional)
if (client.enable_bittorrent(6881)) {
    std::cout << "BitTorrent enabled on port 6881" << std::endl;
} else {
    std::cerr << "Failed to enable BitTorrent" << std::endl;
}
```

### 2. Add a Torrent

```cpp
// Add a torrent from file
auto torrent = client.add_torrent("example.torrent", "./downloads/");
if (torrent) {
    std::cout << "Added torrent: " << torrent->get_torrent_info().get_name() << std::endl;
    std::cout << "Total size: " << torrent->get_torrent_info().get_total_length() << " bytes" << std::endl;
    std::cout << "Number of pieces: " << torrent->get_torrent_info().get_num_pieces() << std::endl;
} else {
    std::cerr << "Failed to add torrent" << std::endl;
}
```

### 3. Monitor Download Progress

```cpp
// Set up progress callback
if (torrent) {
    torrent->set_progress_callback([](uint64_t downloaded, uint64_t total, double percentage) {
        std::cout << "Progress: " << percentage << "% (" 
                  << downloaded << "/" << total << " bytes)" << std::endl;
    });
    
    torrent->set_piece_complete_callback([](librats::PieceIndex piece_index) {
        std::cout << "Piece " << piece_index << " completed" << std::endl;
    });
    
    torrent->set_torrent_complete_callback([](const std::string& torrent_name) {
        std::cout << "Torrent completed: " << torrent_name << std::endl;
    });
}
```

### 4. Get Statistics

```cpp
// Get overall BitTorrent statistics
auto stats = client.get_bittorrent_stats();
std::cout << "Downloaded: " << stats.first << " bytes" << std::endl;
std::cout << "Uploaded: " << stats.second << " bytes" << std::endl;
std::cout << "Active torrents: " << client.get_active_torrents_count() << std::endl;

// Get specific torrent progress
if (torrent) {
    std::cout << "Torrent progress: " << torrent->get_progress_percentage() << "%" << std::endl;
    std::cout << "Completed pieces: " << torrent->get_completed_pieces() 
              << "/" << torrent->get_torrent_info().get_num_pieces() << std::endl;
}
```

### 5. DHT Integration

BitTorrent automatically integrates with librats' DHT functionality:

```cpp
// Start DHT first (for peer discovery)
if (client.start_dht_discovery(6881)) {
    std::cout << "DHT started for peer discovery" << std::endl;
}

// Enable BitTorrent (will automatically use DHT for peer discovery)
client.enable_bittorrent(6882);  // Use different port than DHT

// Add torrent (will automatically announce to DHT and discover peers)
auto torrent = client.add_torrent("example.torrent", "./downloads/");
```

### 6. Manage Multiple Torrents

```cpp
// Add multiple torrents
auto torrent1 = client.add_torrent("movie.torrent", "./downloads/movies/");
auto torrent2 = client.add_torrent("music.torrent", "./downloads/music/");
auto torrent3 = client.add_torrent("software.torrent", "./downloads/software/");

// List all active torrents
auto all_torrents = client.get_all_torrents();
std::cout << "Managing " << all_torrents.size() << " torrents:" << std::endl;

for (const auto& t : all_torrents) {
    std::cout << "- " << t->get_torrent_info().get_name() 
              << " (" << t->get_progress_percentage() << "%)" << std::endl;
}

// Remove a torrent
const auto& info_hash = torrent1->get_torrent_info().get_info_hash();
if (client.remove_torrent(info_hash)) {
    std::cout << "Torrent removed successfully" << std::endl;
}
```

## Complete Example

```cpp
#include "librats.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Create RatsClient
    librats::RatsClient client(8080);
    
    // Start the client
    if (!client.start()) {
        std::cerr << "Failed to start RatsClient" << std::endl;
        return -1;
    }
    
    // Start DHT for peer discovery
    if (client.start_dht_discovery(6881)) {
        std::cout << "DHT started for peer discovery" << std::endl;
    }
    
    // Enable BitTorrent functionality
    if (!client.enable_bittorrent(6882)) {
        std::cerr << "Failed to enable BitTorrent" << std::endl;
        return -1;
    }
    
    std::cout << "BitTorrent enabled successfully!" << std::endl;
    
    // Add a torrent (replace with actual torrent file)
    auto torrent = client.add_torrent("example.torrent", "./downloads/");
    if (!torrent) {
        std::cerr << "Failed to add torrent (make sure example.torrent exists)" << std::endl;
        return -1;
    }
    
    std::cout << "Added torrent: " << torrent->get_torrent_info().get_name() << std::endl;
    
    // Set up callbacks
    torrent->set_progress_callback([](uint64_t downloaded, uint64_t total, double percentage) {
        std::cout << "Download progress: " << std::fixed << std::setprecision(2) 
                  << percentage << "%" << std::endl;
    });
    
    torrent->set_torrent_complete_callback([](const std::string& name) {
        std::cout << "Download completed: " << name << std::endl;
    });
    
    // Monitor download for 60 seconds
    std::cout << "Monitoring download for 60 seconds..." << std::endl;
    
    for (int i = 0; i < 60; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (torrent->is_complete()) {
            std::cout << "Download completed!" << std::endl;
            break;
        }
        
        // Print stats every 10 seconds
        if (i % 10 == 0) {
            auto stats = client.get_bittorrent_stats();
            std::cout << "Stats - Downloaded: " << stats.first 
                      << " bytes, Peers: " << torrent->get_peer_count() << std::endl;
        }
    }
    
    // Clean up
    client.stop();
    std::cout << "Demo finished" << std::endl;
    
    return 0;
}
```
