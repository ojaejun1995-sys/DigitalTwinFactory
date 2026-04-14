# DigitalTwinFactory

Unreal Engine 5 digital twin that mirrors factory sensor data in real time via MQTT.

## Architecture

```
[Python sensors] --MQTT/1883--> [Mosquitto] --MQTT/1883--> [ws_bridge.js] --WS/9001 (JSON)--> [UE5 MQTTSubsystem] --delegate--> [DigitalTwinManager] --> [mapped Actors]
```

UE5's built-in `IWebSocket` client speaks plain WebSocket, not MQTT-over-WebSocket framing. Instead of pointing it at Mosquitto's WS listener, a small Node bridge (`Scripts/ws_bridge.js`) subscribes to Mosquitto over native MQTT and rebroadcasts each message as a JSON text frame on port 9001. Mosquitto's WebSocket listener is therefore disabled — the bridge owns 9001.

### Runtime components
- `UMQTTSubsystem` (`Source/DigitalTwinFactory/MQTTSubsystem.{h,cpp}`) — `UGameInstanceSubsystem`. Opens a WebSocket to `ws://localhost:9001` (the bridge), parses incoming JSON, extracts `device_id`, broadcasts `FOnSensorDataReceived(DeviceId, Topic, Payload)`. Auto-reconnects with a 5s delay up to 10 attempts.
- `ADigitalTwinManager` (`Source/DigitalTwinFactory/DigitalTwinManager.{h,cpp}`) — place one in the level. Fill `DeviceActorMap` (`device_id -> AActor*`) in the editor. Dispatches per `type`:
  - `temperature` — lerps material color blue→red between `TempMin`..`TempMax`.
  - `vibration` — lerps material color green→yellow scaled by `VibrationMax`.
  - `robot_arm` — sets actor rotation from `pitch/yaw/roll`.
- `SetActorColor` creates and caches a `UMaterialInstanceDynamic` on the actor's first `UPrimitiveComponent`. The base material must expose a `Color` or `BaseColor` vector parameter.

### Tooling (`Scripts/`)
- `mosquitto.conf` — plain MQTT listener on 1883 only (no WS).
- `ws_bridge.js` + `package.json` — Node bridge: MQTT subscriber on 1883, WS server on 9001. Injects the source `topic` into each JSON payload before forwarding.
- `sensor_simulator.py` — publishes `temp_001`, `vibration_001`, `robot_arm_001` to `factory/line1/<device>` every 1s via paho-mqtt.

## Running locally

Run each step in its own terminal, from `Scripts/`:

```bash
# 1. Broker
mosquitto -c mosquitto.conf -v

# 2. Bridge (first time: npm install)
npm install
node ws_bridge.js

# 3. Simulator (first time: pip install paho-mqtt)
python sensor_simulator.py

# 4. Unreal — Play In Editor with DigitalTwinManager placed and DeviceActorMap populated.
#    UE5 connects to ws://localhost:9001 (the bridge, not Mosquitto).
```

Order matters: the bridge needs the broker up to subscribe, and UE5 needs the bridge up to connect. The simulator and UE5 can start in either order after that.

## Module dependencies
`DigitalTwinFactory.Build.cs` requires `WebSockets`, `Json`, `JsonUtilities` (private) alongside the default UE modules.

## Notes
- All logs use `[<ClassName>]` prefix (e.g. `[MQTTSubsystem]`, `[DigitalTwinManager]`) under dedicated log categories `LogMQTTSubsystem` / `LogDigitalTwinManager`. The bridge logs with a `[ws_bridge]` prefix.
- To move the bridge off 9001 (e.g. to keep Mosquitto's WS listener), set `WS_PORT=9002 node ws_bridge.js` and update `ADigitalTwinManager::BrokerUrl` accordingly.
