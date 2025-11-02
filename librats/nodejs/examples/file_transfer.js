const { RatsClient } = require('../lib/index');
const fs = require('fs');
const path = require('path');

/**
 * File transfer example demonstrating sending and receiving files
 */
class FileTransferExample {
  constructor(port = 8080) {
    this.client = new RatsClient(port);
    this.transfers = new Map(); // Track active transfers
    this.setupCallbacks();
  }

  setupCallbacks() {
    // Connection callbacks
    this.client.onConnection((peerId) => {
      console.log(`✅ Peer connected: ${peerId}`);
    });

    this.client.onDisconnect((peerId) => {
      console.log(`❌ Peer disconnected: ${peerId}`);
    });

    // File transfer progress callback
    this.client.onFileProgress((transferId, progressPercent, status) => {
      console.log(`📊 Transfer ${transferId}: ${progressPercent}% - ${status}`);
      
      if (status === 'COMPLETED') {
        console.log(`✅ Transfer ${transferId} completed successfully!`);
        this.transfers.delete(transferId);
      } else if (status === 'FAILED' || status === 'CANCELLED') {
        console.log(`❌ Transfer ${transferId} ${status.toLowerCase()}`);
        this.transfers.delete(transferId);
      }
    });

    // String message callback for simple commands
    this.client.onString((peerId, message) => {
      console.log(`📝 Message from ${peerId}: ${message}`);
      
      // Handle simple file transfer commands
      if (message.startsWith('send:')) {
        const filePath = message.substring(5).trim();
        this.sendFileToPool(filePath);
      } else if (message.startsWith('request:')) {
        const filePath = message.substring(8).trim();
        this.requestFileFromPeer(peerId, filePath);
      }
    });
  }

  async start() {
    console.log('🚀 Starting File Transfer Client...');
    
    if (!this.client.start()) {
      throw new Error('Failed to start client');
    }

    console.log(`✅ Client started successfully`);
    console.log(`📋 Our peer ID: ${this.client.getOurPeerId()}`);
    
    // Enable encryption for secure file transfers
    this.client.setEncryptionEnabled(true);
    const encKey = this.client.generateEncryptionKey();
    console.log(`🔐 Generated encryption key: ${encKey}`);
    
    // Set up a data directory for transfers
    const dataDir = path.join(__dirname, 'transfers');
    if (!fs.existsSync(dataDir)) {
      fs.mkdirSync(dataDir, { recursive: true });
    }
    this.client.setDataDirectory(dataDir);
    console.log(`📁 Data directory: ${dataDir}`);
  }

  connectToPeer(host, port) {
    console.log(`🔗 Connecting to ${host}:${port}`);
    
    if (this.client.connect(host, port)) {
      console.log(`✅ Connection initiated successfully`);
    } else {
      console.log(`❌ Failed to initiate connection`);
    }
  }

  sendFileToPool(filePath) {
    const peerIds = this.client.getPeerIds();
    
    if (peerIds.length === 0) {
      console.log('❌ No peers connected to send file to');
      return;
    }

    if (!fs.existsSync(filePath)) {
      console.log(`❌ File not found: ${filePath}`);
      return;
    }

    console.log(`📤 Sending file "${filePath}" to ${peerIds.length} peer(s)...`);
    
    peerIds.forEach(peerId => {
      this.sendFileToPeer(peerId, filePath);
    });
  }

  sendFileToPeer(peerId, filePath, remoteFilename = null) {
    if (!fs.existsSync(filePath)) {
      console.log(`❌ File not found: ${filePath}`);
      return null;
    }

    const filename = remoteFilename || path.basename(filePath);
    console.log(`📤 Sending file "${filePath}" to peer ${peerId} as "${filename}"`);
    
    const transferId = this.client.sendFile(peerId, filePath, filename);
    
    if (transferId) {
      this.transfers.set(transferId, {
        type: 'send',
        peerId,
        filePath,
        remoteFilename: filename,
        startTime: Date.now()
      });
      console.log(`✅ File transfer initiated with ID: ${transferId}`);
      return transferId;
    } else {
      console.log(`❌ Failed to initiate file transfer`);
      return null;
    }
  }

  sendDirectoryToPeer(peerId, dirPath, remoteDirName = null, recursive = true) {
    if (!fs.existsSync(dirPath)) {
      console.log(`❌ Directory not found: ${dirPath}`);
      return null;
    }

    const dirName = remoteDirName || path.basename(dirPath);
    console.log(`📂 Sending directory "${dirPath}" to peer ${peerId} as "${dirName}" (recursive: ${recursive})`);
    
    const transferId = this.client.sendDirectory(peerId, dirPath, dirName, recursive);
    
    if (transferId) {
      this.transfers.set(transferId, {
        type: 'send_dir',
        peerId,
        dirPath,
        remoteDirName: dirName,
        recursive,
        startTime: Date.now()
      });
      console.log(`✅ Directory transfer initiated with ID: ${transferId}`);
      return transferId;
    } else {
      console.log(`❌ Failed to initiate directory transfer`);
      return null;
    }
  }

  requestFileFromPeer(peerId, remoteFilePath, localPath = null) {
    const filename = path.basename(remoteFilePath);
    const downloadPath = localPath || path.join(__dirname, 'transfers', 'downloads', filename);
    
    // Ensure download directory exists
    const downloadDir = path.dirname(downloadPath);
    if (!fs.existsSync(downloadDir)) {
      fs.mkdirSync(downloadDir, { recursive: true });
    }

    console.log(`📥 Requesting file "${remoteFilePath}" from peer ${peerId}`);
    console.log(`📁 Will save to: ${downloadPath}`);
    
    const transferId = this.client.requestFile(peerId, remoteFilePath, downloadPath);
    
    if (transferId) {
      this.transfers.set(transferId, {
        type: 'request',
        peerId,
        remoteFilePath,
        localPath: downloadPath,
        startTime: Date.now()
      });
      console.log(`✅ File request initiated with ID: ${transferId}`);
      return transferId;
    } else {
      console.log(`❌ Failed to initiate file request`);
      return null;
    }
  }

  requestDirectoryFromPeer(peerId, remoteDirPath, localDirPath = null, recursive = true) {
    const dirName = path.basename(remoteDirPath);
    const downloadPath = localDirPath || path.join(__dirname, 'transfers', 'downloads', dirName);
    
    // Ensure download directory exists
    if (!fs.existsSync(downloadPath)) {
      fs.mkdirSync(downloadPath, { recursive: true });
    }

    console.log(`📂 Requesting directory "${remoteDirPath}" from peer ${peerId} (recursive: ${recursive})`);
    console.log(`📁 Will save to: ${downloadPath}`);
    
    const transferId = this.client.requestDirectory(peerId, remoteDirPath, downloadPath, recursive);
    
    if (transferId) {
      this.transfers.set(transferId, {
        type: 'request_dir',
        peerId,
        remoteDirPath,
        localDirPath: downloadPath,
        recursive,
        startTime: Date.now()
      });
      console.log(`✅ Directory request initiated with ID: ${transferId}`);
      return transferId;
    } else {
      console.log(`❌ Failed to initiate directory request`);
      return null;
    }
  }

  pauseTransfer(transferId) {
    if (this.client.pauseFileTransfer(transferId)) {
      console.log(`⏸️ Transfer ${transferId} paused`);
    } else {
      console.log(`❌ Failed to pause transfer ${transferId}`);
    }
  }

  resumeTransfer(transferId) {
    if (this.client.resumeFileTransfer(transferId)) {
      console.log(`▶️ Transfer ${transferId} resumed`);
    } else {
      console.log(`❌ Failed to resume transfer ${transferId}`);
    }
  }

  cancelTransfer(transferId) {
    if (this.client.cancelFileTransfer(transferId)) {
      console.log(`❌ Transfer ${transferId} cancelled`);
      this.transfers.delete(transferId);
    } else {
      console.log(`❌ Failed to cancel transfer ${transferId}`);
    }
  }

  getTransferProgress(transferId) {
    const progressJson = this.client.getFileTransferProgress(transferId);
    if (progressJson) {
      try {
        const progress = JSON.parse(progressJson);
        console.log(`📊 Transfer ${transferId} progress:`, progress);
        return progress;
      } catch (e) {
        console.log(`❌ Failed to parse progress for transfer ${transferId}`);
      }
    } else {
      console.log(`❌ No progress information for transfer ${transferId}`);
    }
    return null;
  }

  printTransferStatistics() {
    const statsJson = this.client.getFileTransferStatistics();
    if (statsJson) {
      try {
        const stats = JSON.parse(statsJson);
        console.log('\n📊 File Transfer Statistics:');
        console.log(JSON.stringify(stats, null, 2));
      } catch (e) {
        console.log('❌ Failed to parse transfer statistics');
      }
    } else {
      console.log('❌ No transfer statistics available');
    }
  }

  printActiveTransfers() {
    console.log('\n🔄 Active Transfers:');
    if (this.transfers.size === 0) {
      console.log('   No active transfers');
    } else {
      this.transfers.forEach((transfer, transferId) => {
        const elapsed = Math.round((Date.now() - transfer.startTime) / 1000);
        console.log(`   ${transferId}: ${transfer.type} - ${elapsed}s elapsed`);
        
        // Get current progress
        this.getTransferProgress(transferId);
      });
    }
    console.log('');
  }

  stop() {
    console.log('🛑 Stopping file transfer client...');
    this.client.stop();
  }
}

// Interactive CLI
function setupInteractiveCLI(client) {
  const readline = require('readline');
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    prompt: 'librats> '
  });

  console.log('\n💡 Interactive File Transfer CLI');
  console.log('Commands:');
  console.log('  connect <host> <port>     - Connect to a peer');
  console.log('  send <peer_id> <file>     - Send file to peer');
  console.log('  senddir <peer_id> <dir>   - Send directory to peer');
  console.log('  request <peer_id> <file>  - Request file from peer');
  console.log('  reqdir <peer_id> <dir>    - Request directory from peer');
  console.log('  pause <transfer_id>       - Pause transfer');
  console.log('  resume <transfer_id>      - Resume transfer');
  console.log('  cancel <transfer_id>      - Cancel transfer');
  console.log('  progress <transfer_id>    - Get transfer progress');
  console.log('  transfers                 - List active transfers');
  console.log('  stats                     - Show transfer statistics');
  console.log('  peers                     - List connected peers');
  console.log('  quit                      - Exit the program');
  console.log('');

  rl.prompt();

  rl.on('line', (line) => {
    const args = line.trim().split(' ');
    const command = args[0].toLowerCase();

    try {
      switch (command) {
        case 'connect':
          if (args.length >= 3) {
            client.connectToPeer(args[1], parseInt(args[2]));
          } else {
            console.log('Usage: connect <host> <port>');
          }
          break;

        case 'send':
          if (args.length >= 3) {
            client.sendFileToPeer(args[1], args[2]);
          } else {
            console.log('Usage: send <peer_id> <file_path>');
          }
          break;

        case 'senddir':
          if (args.length >= 3) {
            client.sendDirectoryToPeer(args[1], args[2]);
          } else {
            console.log('Usage: senddir <peer_id> <directory_path>');
          }
          break;

        case 'request':
          if (args.length >= 3) {
            client.requestFileFromPeer(args[1], args[2]);
          } else {
            console.log('Usage: request <peer_id> <remote_file_path>');
          }
          break;

        case 'reqdir':
          if (args.length >= 3) {
            client.requestDirectoryFromPeer(args[1], args[2]);
          } else {
            console.log('Usage: reqdir <peer_id> <remote_directory_path>');
          }
          break;

        case 'pause':
          if (args.length >= 2) {
            client.pauseTransfer(args[1]);
          } else {
            console.log('Usage: pause <transfer_id>');
          }
          break;

        case 'resume':
          if (args.length >= 2) {
            client.resumeTransfer(args[1]);
          } else {
            console.log('Usage: resume <transfer_id>');
          }
          break;

        case 'cancel':
          if (args.length >= 2) {
            client.cancelTransfer(args[1]);
          } else {
            console.log('Usage: cancel <transfer_id>');
          }
          break;

        case 'progress':
          if (args.length >= 2) {
            client.getTransferProgress(args[1]);
          } else {
            console.log('Usage: progress <transfer_id>');
          }
          break;

        case 'transfers':
          client.printActiveTransfers();
          break;

        case 'stats':
          client.printTransferStatistics();
          break;

        case 'peers':
          const peerIds = client.client.getPeerIds();
          console.log(`Connected peers: ${peerIds.join(', ') || 'None'}`);
          break;

        case 'quit':
        case 'exit':
          console.log('Goodbye!');
          client.stop();
          process.exit(0);
          break;

        default:
          if (command) {
            console.log(`Unknown command: ${command}`);
          }
          break;
      }
    } catch (error) {
      console.error('Error:', error.message);
    }

    rl.prompt();
  });

  rl.on('close', () => {
    console.log('\nGoodbye!');
    client.stop();
    process.exit(0);
  });
}

// Example usage
async function main() {
  const args = process.argv.slice(2);
  const port = args[0] ? parseInt(args[0]) : 8080;
  
  const client = new FileTransferExample(port);
  
  try {
    await client.start();
    
    // If host and port are provided, connect to a peer
    if (args.length >= 2) {
      const host = args[1];
      const peerPort = parseInt(args[2]) || 8081;
      
      setTimeout(() => {
        client.connectToPeer(host, peerPort);
      }, 1000);
    }
    
    // Print active transfers every 30 seconds
    const transferInterval = setInterval(() => {
      if (client.transfers.size > 0) {
        client.printActiveTransfers();
      }
    }, 30000);
    
    // Start interactive CLI
    setupInteractiveCLI(client);
    
    // Handle graceful shutdown
    process.on('SIGINT', () => {
      console.log('\n🛑 Received SIGINT, shutting down gracefully...');
      clearInterval(transferInterval);
      client.stop();
      process.exit(0);
    });
    
  } catch (error) {
    console.error('❌ Error:', error.message);
    process.exit(1);
  }
}

if (require.main === module) {
  main();
}

module.exports = FileTransferExample;
