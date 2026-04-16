@echo off
REM Downloads Pixel Streaming Infrastructure (signalling server) for UE 5.7
REM Run once before start_signalling.bat

pushd "%~dp0"

if exist "SignallingWebServer\platform_scripts\cmd\Start.bat" (
    echo [setup_signalling] Already downloaded. Delete SignallingWebServer\ to re-download.
    goto :EOF
)

echo [setup_signalling] Downloading Pixel Streaming Infrastructure for UE 5.7...
call "%~dp0..\get_ps_servers.bat"
if not exist "get_ps_servers.bat" (
    copy "C:\Program Files\Epic Games\UE_5.7\Engine\Plugins\Media\PixelStreaming2\Resources\WebServers\get_ps_servers.bat" "%~dp0get_ps_servers.bat" >nul
    call "%~dp0get_ps_servers.bat" /v 5.7
)

if exist "SignallingWebServer\platform_scripts\cmd\Start.bat" (
    echo [setup_signalling] Download complete.
) else (
    echo [setup_signalling] ERROR: SignallingWebServer not found after download.
    echo [setup_signalling] Manual download: https://github.com/EpicGamesExt/PixelStreamingInfrastructure
)

popd
