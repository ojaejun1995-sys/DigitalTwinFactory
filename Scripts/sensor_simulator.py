"""Simulate three factory devices publishing sensor data over MQTT.

Run order (all from the Scripts/ directory):
    1. mosquitto -c mosquitto.conf -v       # broker on 1883
    2. npm install && node ws_bridge.js     # MQTT<->WS bridge on 9001
    3. pip install paho-mqtt
       python sensor_simulator.py           # this script
    4. Play In Editor in Unreal (UE5 connects to ws://localhost:9001)
"""

import json
import math
import random
import signal
import sys
import time

import paho.mqtt.client as mqtt

BROKER_HOST = "localhost"
BROKER_PORT = 1883
BASE_TOPIC = "factory/line1"
PUBLISH_INTERVAL_SEC = 1.0


DEVICES = [
    {"device_id": "temp_001", "type": "temperature"},
    {"device_id": "vibration_001", "type": "vibration"},
    {"device_id": "robot_arm_001", "type": "robot_arm"},
]


def build_payload(device: dict, tick: int) -> dict:
    payload = {
        "device_id": device["device_id"],
        "type": device["type"],
        "timestamp": time.time(),
    }

    if device["type"] == "temperature":
        payload["value"] = round(50.0 + 20.0 * math.sin(tick * 0.1) + random.uniform(-2, 2), 2)
        payload["unit"] = "C"
    elif device["type"] == "vibration":
        payload["value"] = round(abs(5.0 * math.sin(tick * 0.3)) + random.uniform(0, 1), 2)
        payload["unit"] = "mm/s"
    elif device["type"] == "robot_arm":
        payload["pitch"] = round(30.0 * math.sin(tick * 0.2), 2)
        payload["yaw"] = round((tick * 5) % 360, 2)
        payload["roll"] = round(15.0 * math.cos(tick * 0.2), 2)

    return payload


def on_connect(client, userdata, flags, rc):
    print(f"[simulator] Connected to broker rc={rc}")


def main() -> int:
    client = mqtt.Client(client_id="sensor_simulator")
    client.on_connect = on_connect

    try:
        client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
    except OSError as exc:
        print(f"[simulator] Failed to connect: {exc}", file=sys.stderr)
        return 1

    client.loop_start()

    running = True

    def stop(_signum, _frame):
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    tick = 0
    try:
        while running:
            for device in DEVICES:
                topic = f"{BASE_TOPIC}/{device['device_id']}"
                payload = build_payload(device, tick)
                payload["topic"] = topic
                client.publish(topic, json.dumps(payload), qos=0)
                print(f"[simulator] {topic} -> {payload}")
            tick += 1
            time.sleep(PUBLISH_INTERVAL_SEC)
    finally:
        client.loop_stop()
        client.disconnect()
        print("[simulator] Stopped")

    return 0


if __name__ == "__main__":
    sys.exit(main())
