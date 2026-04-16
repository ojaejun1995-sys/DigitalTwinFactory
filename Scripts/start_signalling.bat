@echo off
REM Starts the Pixel Streaming 2 signalling server
REM  HTTP player page : port 80
REM  Streamer WS      : port 8888
REM  Player WS        : port 80
REM  SFU WS           : port 8889
REM
REM These ports do NOT conflict with:
REM  - Mosquitto MQTT  : 1883
REM  - ws_bridge WS    : 9001

pushd "%~dp0"

if not exist "SignallingWebServer\platform_scripts\cmd\Start.bat" (
    echo [start_signalling] SignallingWebServer not found. Running setup first...
    call "%~dp0setup_signalling.bat"
)

if not exist "SignallingWebServer\platform_scripts\cmd\Start.bat" (
    echo [start_signalling] ERROR: Could not find SignallingWebServer. See setup_signalling.bat.
    popd
    exit /b 1
)

echo [start_signalling] Starting signalling server (config.json: player=80, streamer=8888, sfu=8889)...
cd SignallingWebServer\platform_scripts\cmd
call start.bat

popd
