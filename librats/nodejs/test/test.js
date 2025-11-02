const { RatsClient, getVersionString, getVersion, ConnectionStrategy, ErrorCodes } = require('../lib/index');
const assert = require('assert');

/**
 * Basic test suite for Node.js librats bindings
 */
describe('LibRats Node.js Bindings', function() {
  this.timeout(10000); // 10 second timeout for tests

  describe('Version Information', function() {
    it('should return version string', function() {
      const version = getVersionString();
      assert(typeof version === 'string');
      assert(version.length > 0);
      console.log(`Version string: ${version}`);
    });

    it('should return version components', function() {
      const version = getVersion();
      assert(typeof version === 'object');
      assert(typeof version.major === 'number');
      assert(typeof version.minor === 'number');
      assert(typeof version.patch === 'number');
      assert(typeof version.build === 'number');
      console.log(`Version: ${version.major}.${version.minor}.${version.patch}.${version.build}`);
    });
  });

  describe('Constants', function() {
    it('should have connection strategies defined', function() {
      assert(typeof ConnectionStrategy.DIRECT_ONLY === 'number');
      assert(typeof ConnectionStrategy.STUN_ASSISTED === 'number');
      assert(typeof ConnectionStrategy.ICE_FULL === 'number');
      assert(typeof ConnectionStrategy.TURN_RELAY === 'number');
      assert(typeof ConnectionStrategy.AUTO_ADAPTIVE === 'number');
    });

    it('should have error codes defined', function() {
      assert(typeof ErrorCodes.SUCCESS === 'number');
      assert(typeof ErrorCodes.INVALID_HANDLE === 'number');
      assert(typeof ErrorCodes.INVALID_PARAMETER === 'number');
    });
  });

  describe('RatsClient', function() {
    let client1, client2;

    beforeEach(function() {
      // Create test clients on different ports
      client1 = new RatsClient(18080);
      client2 = new RatsClient(18081);
    });

    afterEach(function() {
      // Clean up clients
      if (client1) {
        client1.stop();
      }
      if (client2) {
        client2.stop();
      }
    });

    it('should create a client instance', function() {
      assert(client1 instanceof RatsClient);
      assert(client2 instanceof RatsClient);
    });

    it('should start and stop successfully', function() {
      const started = client1.start();
      assert(started === true, 'Client should start successfully');
      
      // Should be able to get our peer ID after starting
      const peerId = client1.getOurPeerId();
      assert(typeof peerId === 'string');
      assert(peerId.length > 0);
      console.log(`Client 1 peer ID: ${peerId}`);
      
      client1.stop();
    });

    it('should handle peer count correctly', function() {
      client1.start();
      
      const initialCount = client1.getPeerCount();
      assert(typeof initialCount === 'number');
      assert(initialCount >= 0);
      console.log(`Initial peer count: ${initialCount}`);
    });

    it('should handle max peers setting', function() {
      client1.start();
      
      const result = client1.setMaxPeers(50);
      assert(result === true, 'Should set max peers successfully');
      
      const maxPeers = client1.getMaxPeers();
      assert(maxPeers === 50, 'Max peers should be 50');
      
      const limitReached = client1.isPeerLimitReached();
      assert(typeof limitReached === 'boolean');
    });

    it('should handle encryption settings', function() {
      client1.start();
      
      // Enable encryption
      const enableResult = client1.setEncryptionEnabled(true);
      assert(enableResult === true, 'Should enable encryption successfully');
      
      const isEnabled = client1.isEncryptionEnabled();
      assert(isEnabled === true, 'Encryption should be enabled');
      
      // Generate key
      const key = client1.generateEncryptionKey();
      assert(typeof key === 'string');
      assert(key.length > 0);
      console.log(`Generated encryption key: ${key.substring(0, 16)}...`);
      
      // Set key
      const setKeyResult = client1.setEncryptionKey(key);
      assert(setKeyResult === true, 'Should set encryption key successfully');
      
      // Get key
      const retrievedKey = client1.getEncryptionKey();
      assert(retrievedKey === key, 'Retrieved key should match set key');
    });

    it('should handle GossipSub availability', function() {
      client1.start();
      
      const available = client1.isGossipsubAvailable();
      console.log(`GossipSub available: ${available}`);
      
      if (available) {
        const running = client1.isGossipsubRunning();
        console.log(`GossipSub running: ${running}`);
        
        // Test topic subscription
        const subscribed = client1.subscribeToTopic('test-topic');
        console.log(`Subscribed to test-topic: ${subscribed}`);
        
        if (subscribed) {
          const isSubscribed = client1.isSubscribedToTopic('test-topic');
          assert(isSubscribed === true, 'Should be subscribed to test-topic');
          
          const topics = client1.getSubscribedTopics();
          assert(Array.isArray(topics));
          assert(topics.includes('test-topic'));
          
          const unsubscribed = client1.unsubscribeFromTopic('test-topic');
          assert(unsubscribed === true, 'Should unsubscribe successfully');
        }
      }
    });

    it('should handle DHT operations', function() {
      client1.start();
      
      // Try to start DHT
      const dhtStarted = client1.startDhtDiscovery(18082);
      console.log(`DHT started: ${dhtStarted}`);
      
      if (dhtStarted) {
        const isRunning = client1.isDhtRunning();
        assert(isRunning === true, 'DHT should be running');
        
        const tableSize = client1.getDhtRoutingTableSize();
        assert(typeof tableSize === 'number');
        console.log(`DHT routing table size: ${tableSize}`);
        
        client1.stopDhtDiscovery();
        
        // Check if stopped
        const isStillRunning = client1.isDhtRunning();
        console.log(`DHT still running after stop: ${isStillRunning}`);
      }
    });

    it('should handle mDNS operations', function() {
      client1.start();
      
      // Try to start mDNS
      const mdnsStarted = client1.startMdnsDiscovery('test-service');
      console.log(`mDNS started: ${mdnsStarted}`);
      
      if (mdnsStarted) {
        const isRunning = client1.isMdnsRunning();
        assert(isRunning === true, 'mDNS should be running');
        
        client1.stopMdnsDiscovery();
        
        // Check if stopped
        const isStillRunning = client1.isMdnsRunning();
        console.log(`mDNS still running after stop: ${isStillRunning}`);
      }
    });

    it('should handle configuration persistence', function() {
      client1.start();
      
      // Set data directory
      const dataDir = './test-data';
      const setDirResult = client1.setDataDirectory(dataDir);
      console.log(`Set data directory result: ${setDirResult}`);
      
      const retrievedDir = client1.getDataDirectory();
      console.log(`Retrieved data directory: ${retrievedDir}`);
      
      // Try to save configuration
      const saveResult = client1.saveConfiguration();
      console.log(`Save configuration result: ${saveResult}`);
      
      // Try to load configuration
      const loadResult = client1.loadConfiguration();
      console.log(`Load configuration result: ${loadResult}`);
    });

    it('should handle connection attempts', function(done) {
      client1.start();
      client2.start();
      
      // Set up connection callback for client2
      client2.onConnection((peerId) => {
        console.log(`Client2 received connection from: ${peerId}`);
        
        // Verify peer is in the list
        const peers = client2.getPeerIds();
        assert(Array.isArray(peers));
        console.log(`Client2 peer list: ${peers.join(', ')}`);
        
        done();
      });
      
      // Try to connect client1 to client2
      setTimeout(() => {
        const connected = client1.connect('127.0.0.1', 18081);
        console.log(`Connection attempt result: ${connected}`);
      }, 100);
    });

    it('should handle message callbacks', function(done) {
      client1.start();
      client2.start();
      
      let messagesReceived = 0;
      const expectedMessages = 3;
      
      // Set up message callbacks for client2
      client2.onString((peerId, message) => {
        console.log(`Client2 received string: ${message} from ${peerId}`);
        messagesReceived++;
        if (messagesReceived === expectedMessages) done();
      });
      
      client2.onBinary((peerId, data) => {
        console.log(`Client2 received binary: ${data.length} bytes from ${peerId}`);
        messagesReceived++;
        if (messagesReceived === expectedMessages) done();
      });
      
      client2.onJson((peerId, jsonStr) => {
        console.log(`Client2 received JSON: ${jsonStr} from ${peerId}`);
        messagesReceived++;
        if (messagesReceived === expectedMessages) done();
      });
      
      // Set up connection callback for client2 to send messages
      client2.onConnection((peerId) => {
        console.log(`Client2 connected to: ${peerId}`);
        
        // Send test messages after connection
        setTimeout(() => {
          const stringResult = client2.sendString(peerId, 'Hello from client2!');
          console.log(`String send result: ${stringResult}`);
          
          const binaryData = Buffer.from('Binary test data', 'utf8');
          const binaryResult = client2.sendBinary(peerId, binaryData);
          console.log(`Binary send result: ${binaryResult}`);
          
          const jsonData = { message: 'JSON test', timestamp: Date.now() };
          const jsonResult = client2.sendJson(peerId, JSON.stringify(jsonData));
          console.log(`JSON send result: ${jsonResult}`);
        }, 100);
      });
      
      // Connect client1 to client2
      setTimeout(() => {
        client1.connect('127.0.0.1', 18081);
      }, 100);
    });
  });

  describe('Error Handling', function() {
    it('should handle invalid parameters gracefully', function() {
      assert.throws(() => {
        new RatsClient(-1); // Invalid port
      });
      
      assert.throws(() => {
        new RatsClient('invalid'); // Invalid port type
      });
    });
  });

  describe('Statistics', function() {
    let client;

    beforeEach(function() {
      client = new RatsClient(18083);
      client.start();
    });

    afterEach(function() {
      client.stop();
    });

    it('should provide connection statistics', function() {
      const stats = client.getConnectionStatistics();
      if (stats) {
        console.log('Connection statistics available');
        const parsed = JSON.parse(stats);
        assert(typeof parsed === 'object');
      } else {
        console.log('No connection statistics available');
      }
    });

    it('should provide file transfer statistics', function() {
      const stats = client.getFileTransferStatistics();
      if (stats) {
        console.log('File transfer statistics available');
        const parsed = JSON.parse(stats);
        assert(typeof parsed === 'object');
      } else {
        console.log('No file transfer statistics available');
      }
    });
  });
});

// Run tests if this file is executed directly
if (require.main === module) {
  console.log('Running librats Node.js binding tests...');
  
  // Simple test runner
  const tests = [
    () => {
      console.log('Testing version info...');
      const version = getVersionString();
      console.log(`✅ Version: ${version}`);
    },
    
    () => {
      console.log('Testing client creation...');
      const client = new RatsClient(18090);
      console.log('✅ Client created');
      
      console.log('Testing client start...');
      const started = client.start();
      if (started) {
        console.log('✅ Client started');
        console.log(`✅ Peer ID: ${client.getOurPeerId()}`);
        client.stop();
        console.log('✅ Client stopped');
      } else {
        console.log('❌ Client failed to start');
      }
    }
  ];
  
  try {
    tests.forEach((test, index) => {
      console.log(`\n--- Test ${index + 1} ---`);
      test();
    });
    console.log('\n✅ All tests completed');
  } catch (error) {
    console.error('\n❌ Test failed:', error.message);
    process.exit(1);
  }
}
