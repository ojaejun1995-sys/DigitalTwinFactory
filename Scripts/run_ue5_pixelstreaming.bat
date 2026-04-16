@echo off
REM Launches UE5 editor with Pixel Streaming 2 enabled
REM Connects to signalling server at ws://localhost:8888
REM
REM Port map:
REM   Signalling HTTP (players) : 80
REM   Signalling WS (streamer)  : 8888
REM   MQTT broker               : 1883
REM   MQTT-WS bridge            : 9001

SET UE_EDITOR="C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
SET PROJECT="C:\Users\user\Documents\Unreal Projects\DigitalTwinFactory\DigitalTwinFactory.uproject"

echo [run_ue5] Starting UE5 Editor with Pixel Streaming 2...
start "" %UE_EDITOR% %PROJECT% ^
    -AudioMixer ^
    -PixelStreamingURL=ws://localhost:8888 ^
    -RenderOffScreen
