// MQTT <-> WebSocket bridge for DigitalTwinFactory.
// Subscribes to factory/# on the local Mosquitto broker and forwards every
// message as a JSON text frame to all connected UE5 WebSocket clients.
//
// Usage:
//   npm install
//   node ws_bridge.js

const mqtt = require('mqtt');
const { WebSocketServer } = require('ws');

const MQTT_URL = process.env.MQTT_URL || 'mqtt://localhost:1883';
const MQTT_TOPIC = process.env.MQTT_TOPIC || 'factory/#';
const WS_PORT = Number(process.env.WS_PORT || 9001);

const wss = new WebSocketServer({ port: WS_PORT });
console.log(`[ws_bridge] WebSocket server listening on ws://localhost:${WS_PORT}`);

wss.on('connection', (socket, req) => {
  const remote = req.socket.remoteAddress;
  console.log(`[ws_bridge] Client connected: ${remote} (total=${wss.clients.size})`);
  socket.on('close', () => {
    console.log(`[ws_bridge] Client disconnected: ${remote} (total=${wss.clients.size})`);
  });
  socket.on('error', (err) => {
    console.error(`[ws_bridge] Client error: ${err.message}`);
  });
});

const client = mqtt.connect(MQTT_URL, { clientId: `ws_bridge_${Date.now()}` });

client.on('connect', () => {
  console.log(`[ws_bridge] Connected to MQTT broker at ${MQTT_URL}`);
  client.subscribe(MQTT_TOPIC, (err) => {
    if (err) {
      console.error(`[ws_bridge] Subscribe failed: ${err.message}`);
      process.exit(1);
    }
    console.log(`[ws_bridge] Subscribed to ${MQTT_TOPIC}`);
  });
});

client.on('reconnect', () => console.log('[ws_bridge] MQTT reconnecting...'));
client.on('error', (err) => console.error(`[ws_bridge] MQTT error: ${err.message}`));

client.on('message', (topic, payload) => {
  const text = payload.toString('utf8');

  let frame;
  try {
    const parsed = JSON.parse(text);
    if (typeof parsed === 'object' && parsed !== null && !parsed.topic) {
      parsed.topic = topic;
    }
    frame = JSON.stringify(parsed);
  } catch {
    frame = JSON.stringify({ topic, payload: text });
  }

  for (const socket of wss.clients) {
    if (socket.readyState === socket.OPEN) {
      socket.send(frame);
    }
  }
});

function shutdown(signal) {
  console.log(`[ws_bridge] Received ${signal}, shutting down`);
  client.end(true, () => {
    wss.close(() => process.exit(0));
  });
}

process.on('SIGINT', () => shutdown('SIGINT'));
process.on('SIGTERM', () => shutdown('SIGTERM'));
