const { RatsClient } = require('../lib/index');

/**
 * GossipSub chat example demonstrating topic-based messaging
 */
class GossipSubChatExample {
  constructor(port = 8080, username = 'Anonymous') {
    this.client = new RatsClient(port);
    this.username = username;
    this.currentTopic = null;
    this.setupCallbacks();
  }

  setupCallbacks() {
    // Connection callbacks
    this.client.onConnection((peerId) => {
      console.log(`✅ Peer connected: ${peerId}`);
      
      // Send a welcome message if we're in a topic
      if (this.currentTopic) {
        this.sendChatMessage(`${this.username} welcomes ${peerId} to the chat!`);
      }
    });

    this.client.onDisconnect((peerId) => {
      console.log(`❌ Peer disconnected: ${peerId}`);
    });

    // Regular string messages for direct communication
    this.client.onString((peerId, message) => {
      console.log(`📝 Direct message from ${peerId}: ${message}`);
    });

    // JSON messages for structured data
    this.client.onJson((peerId, jsonStr) => {
      try {
        const data = JSON.parse(jsonStr);
        if (data.type === 'user_info') {
          console.log(`👤 User info from ${peerId}: ${data.username || 'Unknown'}`);
        } else {
          console.log(`🔧 JSON message from ${peerId}:`, data);
        }
      } catch (e) {
        console.log(`❌ Invalid JSON from ${peerId}: ${jsonStr}`);
      }
    });
  }

  // Set up topic-specific callbacks
  setupTopicCallbacks(topic) {
    // Note: In the actual implementation, you would set topic-specific callbacks
    // For now, we'll use the general message callbacks and filter by topic
    console.log(`🔧 Setting up callbacks for topic: ${topic}`);
  }

  async start() {
    console.log('🚀 Starting GossipSub Chat Client...');
    
    if (!this.client.start()) {
      throw new Error('Failed to start client');
    }

    console.log(`✅ Client started successfully`);
    console.log(`📋 Our peer ID: ${this.client.getOurPeerId()}`);
    console.log(`👤 Username: ${this.username}`);
    
    // Check if GossipSub is available
    if (!this.client.isGossipsubAvailable()) {
      console.log('❌ GossipSub is not available on this client');
      return;
    }
    
    console.log('✅ GossipSub is available');
    
    // Enable encryption for secure communication
    this.client.setEncryptionEnabled(true);
    const encKey = this.client.generateEncryptionKey();
    console.log(`🔐 Generated encryption key: ${encKey}`);
  }

  connectToPeer(host, port) {
    console.log(`🔗 Connecting to ${host}:${port}`);
    
    if (this.client.connect(host, port)) {
      console.log(`✅ Connection initiated successfully`);
    } else {
      console.log(`❌ Failed to initiate connection`);
    }
  }

  joinTopic(topic) {
    if (this.currentTopic) {
      console.log(`🚪 Leaving current topic: ${this.currentTopic}`);
      this.leaveTopic();
    }

    console.log(`🔗 Joining topic: ${topic}`);
    
    if (this.client.subscribeToTopic(topic)) {
      this.currentTopic = topic;
      this.setupTopicCallbacks(topic);
      console.log(`✅ Successfully joined topic: ${topic}`);
      
      // Send a join message
      this.sendChatMessage(`${this.username} joined the chat!`);
      
      // Send user info
      this.sendUserInfo();
      
      return true;
    } else {
      console.log(`❌ Failed to join topic: ${topic}`);
      return false;
    }
  }

  leaveTopic() {
    if (!this.currentTopic) {
      console.log('❌ Not currently in any topic');
      return;
    }

    // Send a leave message before leaving
    this.sendChatMessage(`${this.username} left the chat.`);

    console.log(`🚪 Leaving topic: ${this.currentTopic}`);
    
    if (this.client.unsubscribeFromTopic(this.currentTopic)) {
      console.log(`✅ Successfully left topic: ${this.currentTopic}`);
      this.currentTopic = null;
    } else {
      console.log(`❌ Failed to leave topic: ${this.currentTopic}`);
    }
  }

  sendChatMessage(message) {
    if (!this.currentTopic) {
      console.log('❌ Not in any topic. Join a topic first.');
      return false;
    }

    const chatMessage = {
      type: 'chat',
      username: this.username,
      peerId: this.client.getOurPeerId(),
      message: message,
      timestamp: Date.now()
    };

    if (this.client.publishJsonToTopic(this.currentTopic, JSON.stringify(chatMessage))) {
      console.log(`[${this.currentTopic}] ${this.username}: ${message}`);
      return true;
    } else {
      console.log(`❌ Failed to send message to topic: ${this.currentTopic}`);
      return false;
    }
  }

  sendUserInfo() {
    if (!this.currentTopic) {
      return;
    }

    const userInfo = {
      type: 'user_info',
      username: this.username,
      peerId: this.client.getOurPeerId(),
      timestamp: Date.now(),
      version: 'Node.js librats example'
    };

    this.client.publishJsonToTopic(this.currentTopic, JSON.stringify(userInfo));
  }

  sendTypingIndicator() {
    if (!this.currentTopic) {
      return;
    }

    const typingMessage = {
      type: 'typing',
      username: this.username,
      peerId: this.client.getOurPeerId(),
      timestamp: Date.now()
    };

    this.client.publishJsonToTopic(this.currentTopic, JSON.stringify(typingMessage));
  }

  listTopics() {
    const topics = this.client.getSubscribedTopics();
    console.log('\n📋 Subscribed Topics:');
    if (topics.length === 0) {
      console.log('   No topics subscribed');
    } else {
      topics.forEach(topic => {
        const peerCount = this.client.getTopicPeers(topic).length;
        const current = topic === this.currentTopic ? ' (current)' : '';
        console.log(`   ${topic}: ${peerCount} peers${current}`);
      });
    }
    console.log('');
  }

  listTopicPeers(topic) {
    const peers = this.client.getTopicPeers(topic || this.currentTopic);
    const topicName = topic || this.currentTopic || 'unknown';
    
    console.log(`\n👥 Peers in topic "${topicName}":`);
    if (peers.length === 0) {
      console.log('   No peers in this topic');
    } else {
      peers.forEach(peerId => {
        console.log(`   ${peerId}`);
      });
    }
    console.log('');
  }

  printGossipSubStats() {
    const statsJson = this.client.getGossipsubStatistics();
    if (statsJson) {
      try {
        const stats = JSON.parse(statsJson);
        console.log('\n📊 GossipSub Statistics:');
        console.log(JSON.stringify(stats, null, 2));
      } catch (e) {
        console.log('❌ Failed to parse GossipSub statistics');
      }
    } else {
      console.log('❌ No GossipSub statistics available');
    }
  }

  printStatus() {
    console.log('\n📊 Chat Client Status:');
    console.log(`   Username: ${this.username}`);
    console.log(`   Our Peer ID: ${this.client.getOurPeerId()}`);
    console.log(`   Connected Peers: ${this.client.getPeerCount()}`);
    console.log(`   Current Topic: ${this.currentTopic || 'None'}`);
    console.log(`   GossipSub Running: ${this.client.isGossipsubRunning()}`);
    
    if (this.currentTopic) {
      const topicPeers = this.client.getTopicPeers(this.currentTopic);
      console.log(`   Topic Peers: ${topicPeers.length}`);
    }
    console.log('');
  }

  stop() {
    if (this.currentTopic) {
      this.leaveTopic();
    }
    console.log('🛑 Stopping chat client...');
    this.client.stop();
  }
}

// Interactive Chat CLI
function setupChatCLI(client) {
  const readline = require('readline');
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    prompt: 'chat> '
  });

  console.log('\n💬 Interactive GossipSub Chat CLI');
  console.log('Commands:');
  console.log('  connect <host> <port>     - Connect to a peer');
  console.log('  join <topic>              - Join a chat topic');
  console.log('  leave                     - Leave current topic');
  console.log('  say <message>             - Send message to current topic');
  console.log('  topics                    - List subscribed topics');
  console.log('  peers [topic]             - List peers in topic');
  console.log('  stats                     - Show GossipSub statistics');
  console.log('  status                    - Show client status');
  console.log('  username <name>           - Change username');
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

        case 'join':
          if (args.length >= 2) {
            const topic = args.slice(1).join(' ');
            client.joinTopic(topic);
          } else {
            console.log('Usage: join <topic>');
          }
          break;

        case 'leave':
          client.leaveTopic();
          break;

        case 'say':
          if (args.length >= 2) {
            const message = args.slice(1).join(' ');
            client.sendChatMessage(message);
          } else {
            console.log('Usage: say <message>');
          }
          break;

        case 'topics':
          client.listTopics();
          break;

        case 'peers':
          if (args.length >= 2) {
            client.listTopicPeers(args[1]);
          } else {
            client.listTopicPeers();
          }
          break;

        case 'stats':
          client.printGossipSubStats();
          break;

        case 'status':
          client.printStatus();
          break;

        case 'username':
          if (args.length >= 2) {
            client.username = args.slice(1).join(' ');
            console.log(`👤 Username changed to: ${client.username}`);
          } else {
            console.log('Usage: username <name>');
          }
          break;

        case 'quit':
        case 'exit':
          console.log('Goodbye!');
          client.stop();
          process.exit(0);
          break;

        default:
          if (command) {
            // If not a command, treat as a chat message
            if (client.currentTopic) {
              client.sendChatMessage(line.trim());
            } else {
              console.log(`Unknown command: ${command}. Type a message to chat, or 'join <topic>' to join a topic.`);
            }
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

  // Handle typing indicator (simplified)
  rl.on('line', () => {
    // Could send typing indicator here
  });
}

// Example usage
async function main() {
  const args = process.argv.slice(2);
  const port = args[0] ? parseInt(args[0]) : 8080;
  const username = args[1] || `User_${Math.random().toString(36).substr(2, 6)}`;
  
  const client = new GossipSubChatExample(port, username);
  
  try {
    await client.start();
    
    // If host and port are provided, connect to a peer
    if (args.length >= 3) {
      const host = args[2];
      const peerPort = parseInt(args[3]) || 8081;
      
      setTimeout(() => {
        client.connectToPeer(host, peerPort);
      }, 1000);
    }
    
    // Auto-join a default topic if specified
    if (args.length >= 5) {
      const defaultTopic = args[4];
      setTimeout(() => {
        client.joinTopic(defaultTopic);
      }, 2000);
    }
    
    // Print status every 30 seconds
    const statusInterval = setInterval(() => {
      if (client.client.getPeerCount() > 0) {
        client.printStatus();
      }
    }, 30000);
    
    // Start interactive CLI
    setupChatCLI(client);
    
    // Handle graceful shutdown
    process.on('SIGINT', () => {
      console.log('\n🛑 Received SIGINT, shutting down gracefully...');
      clearInterval(statusInterval);
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

module.exports = GossipSubChatExample;
