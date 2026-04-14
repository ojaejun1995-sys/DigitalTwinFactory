// Copyright Epic Games, Inc. All Rights Reserved.

#include "MQTTSubsystem.h"
#include "WebSocketsModule.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMQTTSubsystem, Log, All);

void UMQTTSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebSockets")))
	{
		FModuleManager::Get().LoadModule(TEXT("WebSockets"));
	}

	UE_LOG(LogMQTTSubsystem, Log, TEXT("[MQTTSubsystem] Initialized"));
}

void UMQTTSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReconnectTimerHandle);
	}

	DisconnectFromBroker();
	UE_LOG(LogMQTTSubsystem, Log, TEXT("[MQTTSubsystem] Deinitialized"));

	Super::Deinitialize();
}

void UMQTTSubsystem::ConnectToBroker(const FString& ServerUrl)
{
	CurrentServerUrl = ServerUrl;

	if (WebSocket.IsValid() && WebSocket->IsConnected())
	{
		UE_LOG(LogMQTTSubsystem, Warning, TEXT("[MQTTSubsystem] Already connected to %s"), *CurrentServerUrl);
		return;
	}

	WebSocket = FWebSocketsModule::Get().CreateWebSocket(CurrentServerUrl, TEXT("mqtt"));

	WebSocket->OnConnected().AddUObject(this, &UMQTTSubsystem::HandleConnected);
	WebSocket->OnConnectionError().AddUObject(this, &UMQTTSubsystem::HandleConnectionError);
	WebSocket->OnClosed().AddUObject(this, &UMQTTSubsystem::HandleClosed);
	WebSocket->OnMessage().AddUObject(this, &UMQTTSubsystem::HandleMessage);

	UE_LOG(LogMQTTSubsystem, Log, TEXT("[MQTTSubsystem] Connecting to %s"), *CurrentServerUrl);
	WebSocket->Connect();
}

void UMQTTSubsystem::DisconnectFromBroker()
{
	if (WebSocket.IsValid())
	{
		if (WebSocket->IsConnected())
		{
			WebSocket->Close();
		}
		WebSocket.Reset();
	}
}

void UMQTTSubsystem::HandleConnected()
{
	ReconnectAttempts = 0;
	UE_LOG(LogMQTTSubsystem, Log, TEXT("[MQTTSubsystem] Connected to broker"));
}

void UMQTTSubsystem::HandleConnectionError(const FString& Error)
{
	UE_LOG(LogMQTTSubsystem, Error, TEXT("[MQTTSubsystem] Connection error: %s"), *Error);
	ScheduleReconnect();
}

void UMQTTSubsystem::HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(LogMQTTSubsystem, Warning, TEXT("[MQTTSubsystem] Connection closed (code=%d, clean=%d): %s"),
		StatusCode, bWasClean ? 1 : 0, *Reason);
	ScheduleReconnect();
}

void UMQTTSubsystem::HandleMessage(const FString& Message)
{
	UE_LOG(LogMQTTSubsystem, Verbose, TEXT("[MQTTSubsystem] Received message: %s"), *Message);

	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogMQTTSubsystem, Warning, TEXT("[MQTTSubsystem] Failed to parse JSON payload"));
		return;
	}

	FString DeviceId;
	if (!JsonObject->TryGetStringField(TEXT("device_id"), DeviceId))
	{
		UE_LOG(LogMQTTSubsystem, Warning, TEXT("[MQTTSubsystem] Message missing device_id field"));
		return;
	}

	FString Topic;
	JsonObject->TryGetStringField(TEXT("topic"), Topic);

	OnSensorDataReceived.Broadcast(DeviceId, Topic, Message);
}

void UMQTTSubsystem::ScheduleReconnect()
{
	if (ReconnectAttempts >= MaxReconnectAttempts)
	{
		UE_LOG(LogMQTTSubsystem, Error, TEXT("[MQTTSubsystem] Max reconnect attempts (%d) reached, giving up"), MaxReconnectAttempts);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	++ReconnectAttempts;
	UE_LOG(LogMQTTSubsystem, Log, TEXT("[MQTTSubsystem] Scheduling reconnect attempt %d/%d in %.1fs"),
		ReconnectAttempts, MaxReconnectAttempts, ReconnectDelaySeconds);

	World->GetTimerManager().SetTimer(ReconnectTimerHandle, [this]()
	{
		ConnectToBroker(CurrentServerUrl);
	}, ReconnectDelaySeconds, false);
}
