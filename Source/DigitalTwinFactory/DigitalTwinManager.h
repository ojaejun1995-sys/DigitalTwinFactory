// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DigitalTwinManager.generated.h"

UCLASS()
class DIGITALTWINFACTORY_API ADigitalTwinManager : public AActor
{
	GENERATED_BODY()

public:
	ADigitalTwinManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Digital Twin")
	TMap<FString, AActor*> DeviceActorMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Digital Twin")
	FString BrokerUrl = TEXT("ws://localhost:9001");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Digital Twin|Temperature")
	float TempMin = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Digital Twin|Temperature")
	float TempMax = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Digital Twin|Vibration")
	float VibrationMax = 10.0f;

private:
	UFUNCTION()
	void HandleSensorData(const FString& DeviceId, const FString& Topic, const FString& Payload);

	void HandleTemperature(AActor* TargetActor, const TSharedPtr<FJsonObject>& JsonObject);
	void HandleVibration(AActor* TargetActor, const TSharedPtr<FJsonObject>& JsonObject);
	void HandleRobotArm(AActor* TargetActor, const TSharedPtr<FJsonObject>& JsonObject);

	void SetActorColor(AActor* TargetActor, const FLinearColor& Color);

	UPROPERTY()
	TMap<AActor*, UMaterialInstanceDynamic*> DynamicMaterialCache;
};
