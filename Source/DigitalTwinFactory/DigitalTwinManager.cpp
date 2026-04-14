// Copyright Epic Games, Inc. All Rights Reserved.

#include "DigitalTwinManager.h"
#include "MQTTSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogDigitalTwinManager, Log, All);

ADigitalTwinManager::ADigitalTwinManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADigitalTwinManager::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogDigitalTwinManager, Error, TEXT("[DigitalTwinManager] GameInstance not found"));
		return;
	}

	UMQTTSubsystem* MQTT = GameInstance->GetSubsystem<UMQTTSubsystem>();
	if (!MQTT)
	{
		UE_LOG(LogDigitalTwinManager, Error, TEXT("[DigitalTwinManager] MQTTSubsystem not found"));
		return;
	}

	MQTT->OnSensorDataReceived.AddDynamic(this, &ADigitalTwinManager::HandleSensorData);
	MQTT->ConnectToBroker(BrokerUrl);

	UE_LOG(LogDigitalTwinManager, Log, TEXT("[DigitalTwinManager] Bound to MQTTSubsystem, %d devices mapped"),
		DeviceActorMap.Num());
}

void ADigitalTwinManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMQTTSubsystem* MQTT = GameInstance->GetSubsystem<UMQTTSubsystem>())
		{
			MQTT->OnSensorDataReceived.RemoveDynamic(this, &ADigitalTwinManager::HandleSensorData);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ADigitalTwinManager::HandleSensorData(const FString& DeviceId, const FString& Topic, const FString& Payload)
{
	AActor** FoundActor = DeviceActorMap.Find(DeviceId);
	if (!FoundActor || !*FoundActor)
	{
		UE_LOG(LogDigitalTwinManager, Verbose, TEXT("[DigitalTwinManager] No actor mapped for device %s"), *DeviceId);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogDigitalTwinManager, Warning, TEXT("[DigitalTwinManager] Failed to parse payload for %s"), *DeviceId);
		return;
	}

	FString DeviceType;
	if (!JsonObject->TryGetStringField(TEXT("type"), DeviceType))
	{
		UE_LOG(LogDigitalTwinManager, Warning, TEXT("[DigitalTwinManager] Payload missing 'type' field for %s"), *DeviceId);
		return;
	}

	if (DeviceType.Equals(TEXT("temperature"), ESearchCase::IgnoreCase))
	{
		HandleTemperature(*FoundActor, JsonObject);
	}
	else if (DeviceType.Equals(TEXT("vibration"), ESearchCase::IgnoreCase))
	{
		HandleVibration(*FoundActor, JsonObject);
	}
	else if (DeviceType.Equals(TEXT("robot_arm"), ESearchCase::IgnoreCase))
	{
		HandleRobotArm(*FoundActor, JsonObject);
	}
	else
	{
		UE_LOG(LogDigitalTwinManager, Warning, TEXT("[DigitalTwinManager] Unknown device type '%s' for %s"),
			*DeviceType, *DeviceId);
	}
}

void ADigitalTwinManager::HandleTemperature(AActor* TargetActor, const TSharedPtr<FJsonObject>& JsonObject)
{
	double Temperature = 0.0;
	if (!JsonObject->TryGetNumberField(TEXT("value"), Temperature))
	{
		return;
	}

	const float Range = FMath::Max(TempMax - TempMin, KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(static_cast<float>((Temperature - TempMin) / Range), 0.0f, 1.0f);
	const FLinearColor Color = FMath::Lerp(FLinearColor::Blue, FLinearColor::Red, Alpha);

	SetActorColor(TargetActor, Color);
}

void ADigitalTwinManager::HandleVibration(AActor* TargetActor, const TSharedPtr<FJsonObject>& JsonObject)
{
	double Vibration = 0.0;
	if (!JsonObject->TryGetNumberField(TEXT("value"), Vibration))
	{
		return;
	}

	const float Range = FMath::Max(VibrationMax, KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(static_cast<float>(Vibration / Range), 0.0f, 1.0f);
	const FLinearColor Color = FMath::Lerp(FLinearColor::Green, FLinearColor::Yellow, Alpha);

	SetActorColor(TargetActor, Color);
}

void ADigitalTwinManager::HandleRobotArm(AActor* TargetActor, const TSharedPtr<FJsonObject>& JsonObject)
{
	double Pitch = 0.0;
	double Yaw = 0.0;
	double Roll = 0.0;

	JsonObject->TryGetNumberField(TEXT("pitch"), Pitch);
	JsonObject->TryGetNumberField(TEXT("yaw"), Yaw);
	JsonObject->TryGetNumberField(TEXT("roll"), Roll);

	const FRotator NewRotation(static_cast<float>(Pitch), static_cast<float>(Yaw), static_cast<float>(Roll));
	TargetActor->SetActorRotation(NewRotation);
}

void ADigitalTwinManager::SetActorColor(AActor* TargetActor, const FLinearColor& Color)
{
	if (!TargetActor)
	{
		return;
	}

	UMaterialInstanceDynamic** CachedMID = DynamicMaterialCache.Find(TargetActor);
	UMaterialInstanceDynamic* MID = CachedMID ? *CachedMID : nullptr;

	if (!MID)
	{
		UPrimitiveComponent* PrimComp = TargetActor->FindComponentByClass<UPrimitiveComponent>();
		if (!PrimComp)
		{
			UE_LOG(LogDigitalTwinManager, Warning, TEXT("[DigitalTwinManager] Actor %s has no PrimitiveComponent"),
				*TargetActor->GetName());
			return;
		}

		UMaterialInterface* BaseMaterial = PrimComp->GetMaterial(0);
		if (!BaseMaterial)
		{
			UE_LOG(LogDigitalTwinManager, Warning, TEXT("[DigitalTwinManager] Actor %s has no base material on slot 0"),
				*TargetActor->GetName());
			return;
		}

		MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		PrimComp->SetMaterial(0, MID);
		DynamicMaterialCache.Add(TargetActor, MID);
	}

	MID->SetVectorParameterValue(TEXT("Color"), Color);
	MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
}
