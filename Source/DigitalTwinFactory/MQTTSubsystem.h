// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IWebSocket.h"
#include "MQTTSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSensorDataReceived, const FString&, DeviceId, const FString&, Topic, const FString&, Payload);

UCLASS()
class DIGITALTWINFACTORY_API UMQTTSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "MQTT")
	void ConnectToBroker(const FString& ServerUrl = TEXT("ws://localhost:9001"));

	UFUNCTION(BlueprintCallable, Category = "MQTT")
	void DisconnectFromBroker();

	UPROPERTY(BlueprintAssignable, Category = "MQTT")
	FOnSensorDataReceived OnSensorDataReceived;

private:
	TSharedPtr<IWebSocket> WebSocket;

	FString CurrentServerUrl;
	int32 ReconnectAttempts = 0;
	static constexpr int32 MaxReconnectAttempts = 10;
	static constexpr float ReconnectDelaySeconds = 5.0f;

	FTimerHandle ReconnectTimerHandle;

	void HandleConnected();
	void HandleConnectionError(const FString& Error);
	void HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void HandleMessage(const FString& Message);

	void ScheduleReconnect();
};
