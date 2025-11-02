const { RatsClient, ConnectionStrategy } = require('../lib/index');

/**
 * Basic client example demonstrating connection and message exchange
 */
class BasicClientExample {
  constructor(port = 8080) {
    this.client = new RatsClient(port);
    this.setupCallbacks();
  }

  setupCallbacks() {
    // Set up event handlers
    this.client.onConnection((peerId) => {
      console.log(`✅ Peer connected: ${peerId}`);
      
      // Send a welcome message to the new peer
      this.client.sendString(peerId, "Hello! Welcome to the network.");
    });

    this.client.onString((peerId, message) => {
      console.log(`📝 String message from ${peerId}: ${message}`);
      
      // Echo the message back
      this.client.sendString(peerId, `Echo: ${message}`);
    });

    this.client.onBinary((peerId, data) => {
      console.log(`📦 Binary message from ${peerId}, size: ${data.length} bytes`);
      console.log(`📦 Data: ${data.toString('hex')}`);
    });

    this.client.onJson((peerId, jsonStr) => {
      try {
        const data = JSON.parse(jsonStr);
        console.log(`🔧 JSON message from ${peerId}:`, data);
      } catch (e) {
        console.log(`❌ Invalid JSON from ${peerId}: ${jsonStr}`);
      }
    });

    this.client.onDisconnect((peerId) => {
      console.log(`❌ Peer disconnected: ${peerId}`);
    });
  }

  async start() {
    console.log('🚀 Starting RatsClient...');
    
    if (!this.client.start()) {
      throw new Error('Failed to start client');
    }

    console.log(`✅ Client started successfully`);
    console.log(`📋 Our peer ID: ${this.client.getOurPeerId()}`);
    
    // Enable encryption for secure communication
    this.client.setEncryptionEnabled(true);
    const encKey = this.client.generateEncryptionKey();
    console.log(`🔐 Generated encryption key: ${encKey}`);
  }

  connectToPeer(host, port, strategy = ConnectionStrategy.AUTO_ADAPTIVE) {
    console.log(`🔗 Connecting to ${host}:${port} using strategy ${strategy}`);
    
    if (this.client.connectWithStrategy(host, port, strategy)) {
      console.log(`✅ Connection initiated successfully`);
    } else {
      console.log(`❌ Failed to initiate connection`);
    }
  }

  sendTestMessages() {
    const peerIds = this.client.getPeerIds();
    
    if (peerIds.length === 0) {
      console.log('No peers connected to send messages to');
      return;
    }

    console.log(`📤 Sending test messages to ${peerIds.length} peer(s)`);

    // Send different types of messages
    peerIds.forEach(peerId => {
      // String message
      this.client.sendString(peerId, `Hello from ${this.client.getOurPeerId()}!`);
      
      // Binary message
      const binaryData = Buffer.from('Hello World!', 'utf8');
      this.client.sendBinary(peerId, binaryData);
      
      // JSON message
      const jsonData = {
        type: 'greeting',
        from: this.client.getOurPeerId(),
        timestamp: Date.now(),
        message: 'Hello from Node.js!'
      };
      this.client.sendJson(peerId, JSON.stringify(jsonData));
    });
  }

  broadcastTestMessages() {
    console.log('📡 Broadcasting test messages...');
    
    // Broadcast string
    const stringCount = this.client.broadcastString('Broadcast message from Node.js!');
    console.log(`📤 String broadcast sent to ${stringCount} peers`);
    
    // Broadcast binary
    const binaryData = Buffer.from('Binary broadcast data', 'utf8');
    const binaryCount = this.client.broadcastBinary(binaryData);
    console.log(`📤 Binary broadcast sent to ${binaryCount} peers`);
    
    // Broadcast JSON
    const jsonData = {
      type: 'broadcast',
      from: this.client.getOurPeerId(),
      timestamp: Date.now(),
      message: 'This is a JSON broadcast'
    };
    const jsonCount = this.client.broadcastJson(JSON.stringify(jsonData));
    console.log(`📤 JSON broadcast sent to ${jsonCount} peers`);
  }

  printStatus() {
    console.log('\n📊 Client Status:');
    console.log(`   Peer Count: ${this.client.getPeerCount()}`);
    console.log(`   Our Peer ID: ${this.client.getOurPeerId()}`);
    console.log(`   Max Peers: ${this.client.getMaxPeers()}`);
    console.log(`   Peer Limit Reached: ${this.client.isPeerLimitReached()}`);
    console.log(`   Encryption Enabled: ${this.client.isEncryptionEnabled()}`);
    
    const stats = this.client.getConnectionStatistics();
    if (stats) {
      console.log(`   Connection Statistics: ${stats}`);
    }
    
    const peerIds = this.client.getPeerIds();
    if (peerIds.length > 0) {
      console.log(`   Connected Peers: ${peerIds.join(', ')}`);
    }
    console.log('');
  }

  stop() {
    console.log('🛑 Stopping client...');
    this.client.stop();
  }
}

// Example usage
async function main() {
  const args = process.argv.slice(2);
  const port = args[0] ? parseInt(args[0]) : 8080;
  
  const client = new BasicClientExample(port);
  
  try {
    await client.start();
    
    // Print status every 10 seconds
    const statusInterval = setInterval(() => {
      client.printStatus();
    }, 10000);
    
    // If host and port are provided, connect to a peer
    if (args.length >= 2) {
      const host = args[1];
      const peerPort = parseInt(args[2]) || 8081;
      
      setTimeout(() => {
        client.connectToPeer(host, peerPort);
      }, 1000);
    }
    
    // Send test messages every 15 seconds if we have peers
    const messageInterval = setInterval(() => {
      if (client.client.getPeerCount() > 0) {
        client.sendTestMessages();
      }
    }, 15000);
    
    // Broadcast test messages every 30 seconds if we have peers
    const broadcastInterval = setInterval(() => {
      if (client.client.getPeerCount() > 0) {
        client.broadcastTestMessages();
      }
    }, 30000);
    
    // Handle graceful shutdown
    process.on('SIGINT', () => {
      console.log('\n🛑 Received SIGINT, shutting down gracefully...');
      clearInterval(statusInterval);
      clearInterval(messageInterval);
      clearInterval(broadcastInterval);
      client.stop();
      process.exit(0);
    });
    
    console.log('✅ Client is running. Press Ctrl+C to stop.');
    console.log('💡 Usage: node basic_client.js [listen_port] [connect_host] [connect_port]');
    
  } catch (error) {
    console.error('❌ Error:', error.message);
    process.exit(1);
  }
}

if (require.main === module) {
  main();
}

module.exports = BasicClientExample;
