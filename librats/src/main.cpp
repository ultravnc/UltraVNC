#include "librats.h"
#include "network_utils.h"
#include "version.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>

// Main module logging macros
#define LOG_MAIN_DEBUG(message) LOG_DEBUG("rats", message)
#define LOG_MAIN_INFO(message)  LOG_INFO("rats", message)
#define LOG_MAIN_WARN(message)  LOG_WARN("rats", message)
#define LOG_MAIN_ERROR(message) LOG_ERROR("rats", message)

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [listen_port] [peer_host] [peer_port]\n";
    std::cout << "  listen_port: Port to listen on for incoming connections (default: 8080)\n";
    std::cout << "  peer_host:   Optional hostname/IP to connect to as peer\n";
    std::cout << "  peer_port:   Optional port of peer to connect to\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << program_name << "                     # Listen on default port 8080\n";
    std::cout << "  " << program_name << " 8080              # Listen on port 8080\n";
    std::cout << "  " << program_name << " 8081 localhost 8080  # Listen on 8081 and connect to 8080\n";
}

void print_help() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help              - Show this help message" << std::endl;
    std::cout << "  version           - Show version information" << std::endl;
    std::cout << "  peers             - Show number of connected peers" << std::endl;
    std::cout << "  list              - List all connected peers with their hash IDs" << std::endl;
    std::cout << "  broadcast <msg>   - Send message to all connected peers" << std::endl;
    std::cout << "  send <hash> <msg> - Send message to specific peer by hash ID" << std::endl;
    std::cout << "  connect <host> <port> - Connect to a peer" << std::endl;
    std::cout << "  connect6 <host> <port> - Connect to a peer using IPv6" << std::endl;
    std::cout << "  connect_dual <host> <port> - Connect using dual stack (IPv6 first, then IPv4)" << std::endl;
    std::cout << "  dht_start         - Start DHT peer discovery" << std::endl;
    std::cout << "  dht_stop          - Stop DHT peer discovery" << std::endl;
    std::cout << "  dht_status        - Show DHT status" << std::endl;
    std::cout << "  dht_find <hash>   - Find peers for content hash" << std::endl;
    std::cout << "  dht_announce <hash> [port] - Announce as peer for content hash" << std::endl;
    std::cout << "  dht_discovery_status - Show automatic rats peer discovery status" << std::endl;
    std::cout << "  netutils [hostname] - Test network utilities" << std::endl;
    std::cout << "  netutils6 [hostname] - Test IPv6 network utilities" << std::endl;
    std::cout << "  dht_test <ip> <port> - Test DHT protocol with specific peer" << std::endl;
    std::cout << "  test_ipv6 <host> <port> - Test IPv6 connectivity" << std::endl;
    std::cout << "\nFile Transfer Commands:" << std::endl;
    std::cout << "  file_send <peer_hash> <file_path> [remote_name] - Send file to peer" << std::endl;
    std::cout << "  dir_send <peer_hash> <dir_path> [remote_name] [recursive] - Send directory to peer" << std::endl;
    std::cout << "  file_request <peer_hash> <remote_path> <local_path> - Request file from peer" << std::endl;
    std::cout << "  dir_request <peer_hash> <remote_path> <local_path> [recursive] - Request directory from peer" << std::endl;
    std::cout << "  transfer_list     - List active file transfers" << std::endl;
    std::cout << "  transfer_status <transfer_id> - Show transfer progress" << std::endl;
    std::cout << "  transfer_pause <transfer_id> - Pause a transfer" << std::endl;
    std::cout << "  transfer_resume <transfer_id> - Resume a transfer" << std::endl;
    std::cout << "  transfer_cancel <transfer_id> - Cancel a transfer" << std::endl;
    std::cout << "  transfer_stats    - Show transfer statistics" << std::endl;
    std::cout << "  quit              - Exit the program" << std::endl;
}

int main(int argc, char* argv[]) {
    // Display ASCII header with version information
    librats::version::rats_print_header();
    
    // Enable debug level logging
    librats::Logger::getInstance().set_log_level(librats::LogLevel::DEBUG);
    
    int listen_port = 8080; // Default port
    
    if (argc >= 2) {
        listen_port = std::stoi(argv[1]);
    } else {
        LOG_MAIN_INFO("No port specified, using default port " << listen_port);
    }
    std::string peer_host = "";
    int peer_port = 0;
    
    if (argc >= 4) {
        peer_host = argv[2];
        peer_port = std::stoi(argv[3]);
    }
    
    LOG_MAIN_DEBUG("Debug logging enabled");
    LOG_MAIN_INFO("Starting RatsClient on port " << listen_port);
    
    // Create and configure the RatsClient
    librats::RatsClient client(listen_port);
    
    // Store connected peers for listing
    std::vector<std::pair<socket_t, std::string>> connected_peers;
    std::mutex peers_list_mutex;
    
    // Set up callbacks
    client.set_connection_callback([&](socket_t socket, const std::string& peer_hash_id) {
        LOG_MAIN_INFO("New peer connected: " << peer_hash_id << " (socket: " << socket << ")");
        
        // Add to connected peers list
        {
            std::lock_guard<std::mutex> lock(peers_list_mutex);
            connected_peers.push_back({socket, peer_hash_id});
        }
    });
    
    client.set_string_data_callback([](socket_t socket, const std::string& peer_hash_id, const std::string& data) {
        LOG_MAIN_INFO("Message from peer " << peer_hash_id << ": " << data);
    });
    
    client.set_disconnect_callback([&](socket_t socket, const std::string& peer_hash_id) {
        LOG_MAIN_INFO("Peer disconnected: " << peer_hash_id << " (socket: " << socket << ")");
        
        // Remove from connected peers list
        {
            std::lock_guard<std::mutex> lock(peers_list_mutex);
            connected_peers.erase(
                std::remove_if(connected_peers.begin(), connected_peers.end(),
                    [socket](const std::pair<socket_t, std::string>& peer) {
                        return peer.first == socket;
                    }),
                connected_peers.end()
            );
        }
    });

    // Set up file transfer callbacks
    if (client.is_file_transfer_available()) {
        // Progress callback for file transfers
        client.on_file_transfer_progress([](const librats::FileTransferProgress& progress) {
            LOG_MAIN_INFO("Transfer " << progress.transfer_id << ": " 
                         << progress.get_completion_percentage() << "% complete ("
                         << progress.bytes_transferred << "/" << progress.total_bytes << " bytes)"
                         << " - Rate: " << (progress.transfer_rate_bps / 1024.0) << " KB/s");
        });
        
        // Completion callback for file transfers
        client.on_file_transfer_completed([](const std::string& transfer_id, bool success, const std::string& error_message) {
            if (success) {
                LOG_MAIN_INFO("Transfer " << transfer_id << " completed successfully!");
            } else {
                LOG_MAIN_ERROR("Transfer " << transfer_id << " failed: " << error_message);
            }
        });
        
        // Incoming file transfer request callback
        client.on_file_transfer_request([](const std::string& peer_id, const librats::FileMetadata& metadata, const std::string& transfer_id) {
            LOG_MAIN_INFO("=== Incoming File Transfer Request ===");
            LOG_MAIN_INFO("From peer: " << peer_id);
            LOG_MAIN_INFO("File: " << metadata.filename);
            LOG_MAIN_INFO("Size: " << metadata.file_size << " bytes");
            LOG_MAIN_INFO("Transfer ID: " << transfer_id);
            LOG_MAIN_INFO("Auto-accepting file transfer...");
            return true; // Auto-accept for now - could be made interactive
        });
        
        // Directory transfer progress callback
        client.on_directory_transfer_progress([](const std::string& transfer_id, const std::string& current_file, 
                                                uint64_t files_completed, uint64_t total_files, 
                                                uint64_t bytes_completed, uint64_t total_bytes) {
            double file_progress = total_files > 0 ? (double(files_completed) / total_files) * 100.0 : 0.0;
            double byte_progress = total_bytes > 0 ? (double(bytes_completed) / total_bytes) * 100.0 : 0.0;
            LOG_MAIN_INFO("Directory transfer " << transfer_id << ": " << files_completed << "/" << total_files 
                         << " files (" << file_progress << "%), " << bytes_completed << "/" << total_bytes 
                         << " bytes (" << byte_progress << "%) - Current: " << current_file);
        });
        
        // File request callback (when receiving file requests)
        client.on_file_request([](const std::string& peer_id, const std::string& file_path, const std::string& transfer_id) {
            LOG_MAIN_INFO("=== Incoming File Request ===");
            LOG_MAIN_INFO("From peer: " << peer_id);
            LOG_MAIN_INFO("Requested file: " << file_path);
            LOG_MAIN_INFO("Transfer ID: " << transfer_id);
            LOG_MAIN_INFO("Auto-accepting file request...");
            return true; // Auto-accept for now - could be made interactive
        });
        
        // Directory request callback (when receiving directory requests)
        client.on_directory_request([](const std::string& peer_id, const std::string& directory_path, 
                                      bool recursive, const std::string& transfer_id) {
            LOG_MAIN_INFO("=== Incoming Directory Request ===");
            LOG_MAIN_INFO("From peer: " << peer_id);
            LOG_MAIN_INFO("Requested directory: " << directory_path);
            LOG_MAIN_INFO("Recursive: " << (recursive ? "yes" : "no"));
            LOG_MAIN_INFO("Transfer ID: " << transfer_id);
            LOG_MAIN_INFO("Auto-accepting directory request...");
            return true; // Auto-accept for now - could be made interactive
        });
        
        LOG_MAIN_INFO("File transfer callbacks configured");
    } else {
        LOG_MAIN_WARN("File transfer manager not available");
    }

    // Start the client
    if (!client.start()) {
        LOG_MAIN_ERROR("Failed to start RatsClient on port " << listen_port);
        return 1;
    }
    
    // Start DHT discovery
    LOG_MAIN_INFO("Starting DHT peer discovery...");
    if (client.start_dht_discovery()) {
        LOG_MAIN_INFO("DHT peer discovery started successfully");
    } else {
        LOG_MAIN_WARN("Failed to start DHT peer discovery, but continuing...");
    }
    
    // Connect to peer if specified
    if (!peer_host.empty() && peer_port > 0) {
        LOG_MAIN_INFO("Connecting to peer " << peer_host << ":" << peer_port << "...");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (client.connect_to_peer(peer_host, peer_port)) {
            LOG_MAIN_INFO("Successfully connected to peer!");
        } else {
            LOG_MAIN_WARN("Failed to connect to peer, but continuing...");
        }
    }
    
    LOG_MAIN_INFO("RatsClient is running. Current peers: " << client.get_peer_count());
    if (client.is_dht_running()) {
        LOG_MAIN_INFO("DHT peer discovery is active. Routing table size: " << client.get_dht_routing_table_size() << " nodes");
        if (client.is_automatic_discovery_running()) {
            LOG_MAIN_INFO("Automatic rats peer discovery is active using hash: " << librats::RatsClient::get_rats_peer_discovery_hash());
            LOG_MAIN_INFO("This will automatically find and connect to other rats peers!");
        }
    } else {
        LOG_MAIN_INFO("DHT peer discovery is inactive. Use 'dht_start' to enable it.");
    }
    print_help();
    
    // Add initial prompt
    std::cout << "\nrats> ";
    std::cout.flush();
    
    // Main command loop
    std::string input;
    while (client.is_running()) {
        std::getline(std::cin, input);
        
        if (input.empty()) {
            std::cout << "rats> ";
            std::cout.flush();
            continue;
        }
        
        std::istringstream iss(input);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            LOG_MAIN_INFO("Shutting down...");
            break;
        }
        else if (command == "help") {
            print_help();
        }
        else if (command == "version") {
            librats::version::rats_print_version_info();
        }
        else if (command == "peers") {
            LOG_MAIN_INFO("Connected peers: " << client.get_peer_count());
        }
        else if (command == "list") {
            std::lock_guard<std::mutex> lock(peers_list_mutex);
            if (connected_peers.empty()) {
                std::cout << "No peers connected." << std::endl;
            } else {
                std::cout << "Connected peers:" << std::endl;
                for (const auto& peer : connected_peers) {
                    std::cout << "  Socket: " << peer.first << " | Hash ID: " << peer.second << std::endl;
                }
            }
        }
        else if (command == "broadcast") {
            std::string message;
            std::getline(iss, message);
            if (!message.empty()) {
                message = message.substr(1); // Remove leading space
                int sent = client.broadcast_string_to_peers(message);
                LOG_MAIN_INFO("Broadcasted message to " << sent << " peers");
            } else {
                std::cout << "Usage: broadcast <message>" << std::endl;
            }
        }
        else if (command == "send") {
            std::string hash_id, message;
            iss >> hash_id;
            std::getline(iss, message);
            if (!hash_id.empty() && !message.empty()) {
                message = message.substr(1); // Remove leading space
                if (client.send_string_to_peer_id(hash_id, message)) {
                    LOG_MAIN_INFO("Sent message to peer " << hash_id);
                } else {
                    LOG_MAIN_ERROR("Failed to send message to peer " << hash_id);
                }
            } else {
                std::cout << "Usage: send <hash_id> <message>" << std::endl;
            }
        }
        else if (command == "connect") {
            std::string host;
            int port;
            iss >> host >> port;
            if (!host.empty() && port > 0) {
                LOG_MAIN_INFO("Connecting to " << host << ":" << port << "...");
                if (client.connect_to_peer(host, port)) {
                    LOG_MAIN_INFO("Successfully connected!");
                } else {
                    LOG_MAIN_ERROR("Failed to connect to peer");
                }
            } else {
                std::cout << "Usage: connect <host> <port>" << std::endl;
            }
        }
        else if (command == "dht_start") {
            if (client.is_dht_running()) {
                std::cout << "DHT is already running." << std::endl;
            } else {
                LOG_MAIN_INFO("Starting DHT peer discovery...");
                if (client.start_dht_discovery()) {
                    LOG_MAIN_INFO("DHT peer discovery started successfully");
                } else {
                    LOG_MAIN_ERROR("Failed to start DHT peer discovery");
                }
            }
        }
        else if (command == "dht_stop") {
            if (!client.is_dht_running()) {
                std::cout << "DHT is not running." << std::endl;
            } else {
                LOG_MAIN_INFO("Stopping DHT peer discovery...");
                client.stop_dht_discovery();
                LOG_MAIN_INFO("DHT peer discovery stopped");
            }
        }
        else if (command == "dht_status") {
            if (client.is_dht_running()) {
                size_t routing_table_size = client.get_dht_routing_table_size();
                LOG_MAIN_INFO("DHT Status: RUNNING | Routing table size: " << routing_table_size << " nodes");
            } else {
                LOG_MAIN_INFO("DHT Status: STOPPED");
            }
        }
        else if (command == "dht_find") {
            std::string content_hash;
            iss >> content_hash;
            if (!content_hash.empty()) {
                if (!client.is_dht_running()) {
                    std::cout << "DHT is not running. Start it first with 'dht_start'" << std::endl;
                } else {
                    LOG_MAIN_INFO("Finding peers for content hash: " << content_hash);
                    bool success = client.find_peers_by_hash(content_hash, 
                        [content_hash](const std::vector<std::string>& peers) {
                            LOG_MAIN_INFO("Found " << peers.size() << " peers for hash " << content_hash);
                            for (const auto& peer : peers) {
                                LOG_MAIN_INFO("  Peer: " << peer);
                            }
                        });
                    if (success) {
                        LOG_MAIN_INFO("DHT peer search initiated");
                    } else {
                        LOG_MAIN_ERROR("Failed to initiate DHT peer search");
                    }
                }
            } else {
                std::cout << "Usage: dht_find <content_hash>" << std::endl;
            }
        }
        else if (command == "dht_announce") {
            std::string content_hash;
            int port = 0;
            iss >> content_hash >> port;
            if (!content_hash.empty()) {
                if (!client.is_dht_running()) {
                    std::cout << "DHT is not running. Start it first with 'dht_start'" << std::endl;
                } else {
                    LOG_MAIN_INFO("Announcing as peer for content hash: " << content_hash 
                                  << " (port: " << (port > 0 ? port : listen_port) << ")");
                    if (client.announce_for_hash(content_hash, port)) {
                        LOG_MAIN_INFO("DHT peer announcement initiated");
                    } else {
                        LOG_MAIN_ERROR("Failed to initiate DHT peer announcement");
                    }
                }
            } else {
                std::cout << "Usage: dht_announce <content_hash> [port]" << std::endl;
            }
        }
        else if (command == "dht_discovery_status") {
            LOG_MAIN_INFO("=== Automatic Rats Peer Discovery Status ===");
            if (client.is_automatic_discovery_running()) {
                LOG_MAIN_INFO("Automatic discovery: RUNNING");
                LOG_MAIN_INFO("Discovery hash: " << librats::RatsClient::get_rats_peer_discovery_hash());
                LOG_MAIN_INFO("Discovery works by:");
                LOG_MAIN_INFO("  - Announcing our presence for the rats discovery hash every 10 minutes");
                LOG_MAIN_INFO("  - Searching for other rats peers every 5 minutes");
                LOG_MAIN_INFO("  - Automatically connecting to discovered rats peers");
            } else {
                LOG_MAIN_INFO("Automatic discovery: STOPPED");
            }
            if (client.is_dht_running()) {
                LOG_MAIN_INFO("DHT Status: RUNNING | Routing table size: " << client.get_dht_routing_table_size() << " nodes");
            } else {
                LOG_MAIN_INFO("DHT Status: STOPPED");
            }
        }
        else if (command == "file_send") {
            std::string peer_hash, file_path, remote_name;
            iss >> peer_hash >> file_path;
            std::getline(iss, remote_name);
            if (!remote_name.empty()) {
                remote_name = remote_name.substr(1); // Remove leading space
            }
            
            if (!peer_hash.empty() && !file_path.empty()) {
                if (!client.is_file_transfer_available()) {
                    LOG_MAIN_ERROR("File transfer not available");
                } else {
                    LOG_MAIN_INFO("Sending file '" << file_path << "' to peer " << peer_hash 
                                 << (remote_name.empty() ? "" : " as '" + remote_name + "'"));
                    std::string transfer_id = client.send_file(peer_hash, file_path, remote_name);
                    if (!transfer_id.empty()) {
                        LOG_MAIN_INFO("File transfer initiated with ID: " << transfer_id);
                    } else {
                        LOG_MAIN_ERROR("Failed to initiate file transfer");
                    }
                }
            } else {
                std::cout << "Usage: file_send <peer_hash> <file_path> [remote_name]" << std::endl;
            }
        }
        else if (command == "dir_send") {
            std::string peer_hash, dir_path, remote_name, recursive_str;
            iss >> peer_hash >> dir_path >> remote_name >> recursive_str;
            
            if (!peer_hash.empty() && !dir_path.empty()) {
                if (!client.is_file_transfer_available()) {
                    LOG_MAIN_ERROR("File transfer not available");
                } else {
                    bool recursive = (recursive_str.empty() || recursive_str == "true" || recursive_str == "1");
                    LOG_MAIN_INFO("Sending directory '" << dir_path << "' to peer " << peer_hash 
                                 << (remote_name.empty() ? "" : " as '" + remote_name + "'")
                                 << " (recursive: " << (recursive ? "yes" : "no") << ")");
                    std::string transfer_id = client.send_directory(peer_hash, dir_path, remote_name, recursive);
                    if (!transfer_id.empty()) {
                        LOG_MAIN_INFO("Directory transfer initiated with ID: " << transfer_id);
                    } else {
                        LOG_MAIN_ERROR("Failed to initiate directory transfer");
                    }
                }
            } else {
                std::cout << "Usage: dir_send <peer_hash> <dir_path> [remote_name] [recursive]" << std::endl;
            }
        }
        else if (command == "file_request") {
            std::string peer_hash, remote_path, local_path;
            iss >> peer_hash >> remote_path >> local_path;
            
            if (!peer_hash.empty() && !remote_path.empty() && !local_path.empty()) {
                if (!client.is_file_transfer_available()) {
                    LOG_MAIN_ERROR("File transfer not available");
                } else {
                    LOG_MAIN_INFO("Requesting file '" << remote_path << "' from peer " << peer_hash 
                                 << " to save as '" << local_path << "'");
                    std::string transfer_id = client.request_file(peer_hash, remote_path, local_path);
                    if (!transfer_id.empty()) {
                        LOG_MAIN_INFO("File request initiated with ID: " << transfer_id);
                    } else {
                        LOG_MAIN_ERROR("Failed to initiate file request");
                    }
                }
            } else {
                std::cout << "Usage: file_request <peer_hash> <remote_path> <local_path>" << std::endl;
            }
        }
        else if (command == "dir_request") {
            std::string peer_hash, remote_path, local_path, recursive_str;
            iss >> peer_hash >> remote_path >> local_path >> recursive_str;
            
            if (!peer_hash.empty() && !remote_path.empty() && !local_path.empty()) {
                if (!client.is_file_transfer_available()) {
                    LOG_MAIN_ERROR("File transfer not available");
                } else {
                    bool recursive = (recursive_str.empty() || recursive_str == "true" || recursive_str == "1");
                    LOG_MAIN_INFO("Requesting directory '" << remote_path << "' from peer " << peer_hash 
                                 << " to save as '" << local_path << "' (recursive: " << (recursive ? "yes" : "no") << ")");
                    std::string transfer_id = client.request_directory(peer_hash, remote_path, local_path, recursive);
                    if (!transfer_id.empty()) {
                        LOG_MAIN_INFO("Directory request initiated with ID: " << transfer_id);
                    } else {
                        LOG_MAIN_ERROR("Failed to initiate directory request");
                    }
                }
            } else {
                std::cout << "Usage: dir_request <peer_hash> <remote_path> <local_path> [recursive]" << std::endl;
            }
        }
        else if (command == "transfer_list") {
            if (!client.is_file_transfer_available()) {
                LOG_MAIN_ERROR("File transfer not available");
            } else {
                auto active_transfers = client.get_active_file_transfers();
                if (active_transfers.empty()) {
                    std::cout << "No active file transfers." << std::endl;
                } else {
                    std::cout << "Active file transfers:" << std::endl;
                    for (const auto& transfer : active_transfers) {
                        std::cout << "  ID: " << transfer->transfer_id << std::endl;
                        std::cout << "    Peer: " << transfer->peer_id << std::endl;
                        std::cout << "    File: " << transfer->filename << std::endl;
                        std::cout << "    Direction: " << (transfer->direction == librats::FileTransferDirection::SENDING ? "SENDING" : "RECEIVING") << std::endl;
                        std::cout << "    Status: ";
                        switch (transfer->status) {
                            case librats::FileTransferStatus::PENDING: std::cout << "PENDING"; break;
                            case librats::FileTransferStatus::STARTING: std::cout << "STARTING"; break;
                            case librats::FileTransferStatus::IN_PROGRESS: std::cout << "IN_PROGRESS"; break;
                            case librats::FileTransferStatus::PAUSED: std::cout << "PAUSED"; break;
                            case librats::FileTransferStatus::COMPLETED: std::cout << "COMPLETED"; break;
                            case librats::FileTransferStatus::FAILED: std::cout << "FAILED"; break;
                            case librats::FileTransferStatus::CANCELLED: std::cout << "CANCELLED"; break;
                            case librats::FileTransferStatus::RESUMING: std::cout << "RESUMING"; break;
                        }
                        std::cout << std::endl;
                        std::cout << "    Progress: " << transfer->get_completion_percentage() << "% ("
                                 << transfer->bytes_transferred << "/" << transfer->total_bytes << " bytes)" << std::endl;
                        std::cout << "    Rate: " << (transfer->transfer_rate_bps / 1024.0) << " KB/s" << std::endl;
                        std::cout << std::endl;
                    }
                }
            }
        }
        else if (command == "transfer_status") {
            std::string transfer_id;
            iss >> transfer_id;
            if (!transfer_id.empty()) {
                if (!client.is_file_transfer_available()) {
                    LOG_MAIN_ERROR("File transfer not available");
                } else {
                    auto progress = client.get_file_transfer_progress(transfer_id);
                    if (progress) {
                        std::cout << "Transfer " << transfer_id << " status:" << std::endl;
                        std::cout << "  Peer: " << progress->peer_id << std::endl;
                        std::cout << "  File: " << progress->filename << std::endl;
                        std::cout << "  Direction: " << (progress->direction == librats::FileTransferDirection::SENDING ? "SENDING" : "RECEIVING") << std::endl;
                        std::cout << "  Status: ";
                        switch (progress->status) {
                            case librats::FileTransferStatus::PENDING: std::cout << "PENDING"; break;
                            case librats::FileTransferStatus::STARTING: std::cout << "STARTING"; break;
                            case librats::FileTransferStatus::IN_PROGRESS: std::cout << "IN_PROGRESS"; break;
                            case librats::FileTransferStatus::PAUSED: std::cout << "PAUSED"; break;
                            case librats::FileTransferStatus::COMPLETED: std::cout << "COMPLETED"; break;
                            case librats::FileTransferStatus::FAILED: std::cout << "FAILED"; break;
                            case librats::FileTransferStatus::CANCELLED: std::cout << "CANCELLED"; break;
                            case librats::FileTransferStatus::RESUMING: std::cout << "RESUMING"; break;
                        }
                        std::cout << std::endl;
                        std::cout << "  Progress: " << progress->get_completion_percentage() << "%" << std::endl;
                        std::cout << "  Bytes: " << progress->bytes_transferred << "/" << progress->total_bytes << std::endl;
                        std::cout << "  Chunks: " << progress->chunks_completed << "/" << progress->total_chunks << std::endl;
                        std::cout << "  Rate: " << (progress->transfer_rate_bps / 1024.0) << " KB/s" << std::endl;
                        std::cout << "  Average Rate: " << (progress->average_rate_bps / 1024.0) << " KB/s" << std::endl;
                        std::cout << "  Elapsed: " << progress->get_elapsed_time().count() << " ms" << std::endl;
                        if (progress->estimated_time_remaining.count() > 0) {
                            std::cout << "  ETA: " << progress->estimated_time_remaining.count() << " ms" << std::endl;
                        }
                        if (!progress->error_message.empty()) {
                            std::cout << "  Error: " << progress->error_message << std::endl;
                        }
                    } else {
                        LOG_MAIN_ERROR("Transfer " << transfer_id << " not found");
                    }
                }
            } else {
                std::cout << "Usage: transfer_status <transfer_id>" << std::endl;
            }
        }
        else if (command == "transfer_pause") {
            std::string transfer_id;
            iss >> transfer_id;
            if (!transfer_id.empty()) {
                if (!client.is_file_transfer_available()) {
                    LOG_MAIN_ERROR("File transfer not available");
                } else {
                    if (client.pause_file_transfer(transfer_id)) {
                        LOG_MAIN_INFO("Transfer " << transfer_id << " paused");
                    } else {
                        LOG_MAIN_ERROR("Failed to pause transfer " << transfer_id);
                    }
                }
            } else {
                std::cout << "Usage: transfer_pause <transfer_id>" << std::endl;
            }
        }
        else if (command == "transfer_resume") {
            std::string transfer_id;
            iss >> transfer_id;
            if (!transfer_id.empty()) {
                if (!client.is_file_transfer_available()) {
                    LOG_MAIN_ERROR("File transfer not available");
                } else {
                    if (client.resume_file_transfer(transfer_id)) {
                        LOG_MAIN_INFO("Transfer " << transfer_id << " resumed");
                    } else {
                        LOG_MAIN_ERROR("Failed to resume transfer " << transfer_id);
                    }
                }
            } else {
                std::cout << "Usage: transfer_resume <transfer_id>" << std::endl;
            }
        }
        else if (command == "transfer_cancel") {
            std::string transfer_id;
            iss >> transfer_id;
            if (!transfer_id.empty()) {
                if (!client.is_file_transfer_available()) {
                    LOG_MAIN_ERROR("File transfer not available");
                } else {
                    if (client.cancel_file_transfer(transfer_id)) {
                        LOG_MAIN_INFO("Transfer " << transfer_id << " cancelled");
                    } else {
                        LOG_MAIN_ERROR("Failed to cancel transfer " << transfer_id);
                    }
                }
            } else {
                std::cout << "Usage: transfer_cancel <transfer_id>" << std::endl;
            }
        }
        else if (command == "transfer_stats") {
            if (!client.is_file_transfer_available()) {
                LOG_MAIN_ERROR("File transfer not available");
            } else {
                auto stats = client.get_file_transfer_statistics();
                std::cout << "File Transfer Statistics:" << std::endl;
                std::cout << stats.dump(2) << std::endl;
            }
        }
        else {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Type 'help' for available commands." << std::endl;
        }
        
        // Always show prompt after each command
        std::cout << "rats> ";
        std::cout.flush();
    }
    
    // Clean shutdown
    LOG_MAIN_INFO("Stopping DHT peer discovery...");
    client.stop_dht_discovery();
    
    client.stop();
    LOG_MAIN_INFO("RatsClient stopped. Goodbye!");
    
    return 0;
} 