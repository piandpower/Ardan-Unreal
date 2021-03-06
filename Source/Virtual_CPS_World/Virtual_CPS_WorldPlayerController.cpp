// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Virtual_CPS_World.h"
#include "Virtual_CPS_WorldPlayerController.h"
#include "Virtual_CPS_WorldCharacter.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Sensor.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

#if WITH_EDITOR
 #include "UnrealEd.h"
 #endif


AVirtual_CPS_WorldPlayerController::AVirtual_CPS_WorldPlayerController()
{
  /* Allows camera movement when world is paused */
  SetTickableWhenPaused(true);
  bShouldPerformFullTickWhenPaused = true;

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	UBlueprint* blueprint = Cast<UBlueprint>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("Blueprint'/Game/SensorNode.SensorNode'")));
	sensorBlueprint = (UClass*)(blueprint->GeneratedClass);
	if (sensorBlueprint != NULL) {
		UE_LOG(LogNet, Log, TEXT("BName: %s"), *(sensorBlueprint->GetClass()->GetName()));
	} else {
		UE_LOG(LogNet, Log, TEXT("I got nothing"));
	}

  /* Networking setup */
	sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = sockSubSystem->CreateSocket(NAME_DGram, TEXT("UDPCONN2"), true);
	if (socket) UE_LOG(LogNet, Log, TEXT("Created Socket"));
	socket->SetReceiveBufferSize(RecvSize, RecvSize);
	socket->SetSendBufferSize(SendSize, SendSize);

	/* Set up destination address:port to send Cooja messages to */
	FIPv4Address::Parse(address, ip);
	addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(ip.GetValue());
	addr->SetPort(port);

}

void AVirtual_CPS_WorldPlayerController::BeginPlay() {
  conns = FRunnableConnection::create(5000, &packetQ);
  if (conns) {UE_LOG(LogNet, Log, TEXT("conns created"));}
  else { UE_LOG(LogNet, Log, TEXT("conns failed"));}
  // conns->Init();
  // conns->Run();

	TSubclassOf<ACameraActor> ClassToFind;
	cameras.Empty();
	currentCam = 0;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ClassToFind, cameras);
	for(TActorIterator<AActor> It(GetWorld(), ACameraActor::StaticClass()); It; ++It)
	{
		ACameraActor* Actor = (ACameraActor*)*It;
		if(!Actor->IsPendingKill())
		{
			cameras.Add(Actor);
      Actor->SetTickableWhenPaused(true);
		}
	}
	UE_LOG(LogNet, Log, TEXT("Found %d cameras"), cameras.Num());
	//if (cameras.Num()) NextCamera();

  /* Find all sensors in the world to handle connections to Cooja */
 	for (TActorIterator<ASensor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		if (ActorItr->GetClass() == ASensor::StaticClass()) {
			//UE_LOG(LogNet, Log, TEXT("Name: %s"), *(ActorItr->GetName()));
			//UE_LOG(LogNet, Log, TEXT("Class: %s"),
					//*(ActorItr->GetClass()->GetDesc()));
			//UE_LOG(LogNet, Log, TEXT("Location: %s"),
				//	*(ActorItr->GetActorLocation().ToString()));
			sensors.Add(ActorItr.operator *());
			sensorTable.Add(ActorItr->ID, ActorItr.operator *());

		}
	}
  UE_LOG(LogNet, Log, TEXT("--START--"));
}

void AVirtual_CPS_WorldPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (conns) {
    conns->Stop();
    conns->Shutdown();
  }

}


void AVirtual_CPS_WorldPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
  /* Working out FPS */
  elapsed += DeltaTime;
  tickCount += 1;
  if (elapsed >= 1) {
    UE_LOG(LogNet, Log, TEXT("TSTAMP: %f %f %f %d "), DeltaTime, 1/DeltaTime,
          elapsed/tickCount,
          tickCount);
    elapsed = 0;
    tickCount = 0;
  }
	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}
	if (bSelectItem) {
		selectItem();
		bSelectItem = false;
	}
  update_sensors();
}


void AVirtual_CPS_WorldPlayerController::update_sensors() {
  uint8* pkt;
  int count = 0;
  while (count++ < 5 && !packetQ.IsEmpty()){
    packetQ.Dequeue(pkt);
    if (pkt[0] == LED_PKT && pkt[1] < sensors.Num()) {
   			//UE_LOG(LogNet, Log, TEXT("LEDPKT"));
   			ASensor **s = sensorTable.Find(pkt[1]);
  			if (s == NULL) return;
        //UE_LOG(LogNet, Log, TEXT("FOUND SENSOR"));
  			(*s)->SetLed(pkt[2], pkt[3], pkt[4]);
 		}
    else if (pkt[0] == RADIO_PKT) {
  		//UE_LOG(LogNet, Log, TEXT("RMPKT %d (%d)"), pkt[1], pkt[2]);
  		/* Display radio event */
  		ASensor **s = sensorTable.Find(pkt[1]);
  		if (s == NULL) return;
      FVector sourceLoc = (*s)->GetSensorLocation();
  		DrawDebugCircle(GetWorld(), sourceLoc, 50.0, 360, colours[pkt[1] % 12], false, 0.5 , 0, 5, FVector(1.f,0.f,0.f), FVector(0.f,1.f,0.f), false);
  		for (int i = 0; i < pkt[2]; i++) {
		    ASensor **ss = sensorTable.Find(pkt[3 + i]);
        if (ss == NULL) continue;
  			DrawDebugDirectionalArrow(GetWorld(), sourceLoc,
                                  (*ss)->GetSensorLocation(),
                    							500.0, colours[pkt[1] % 12],
                    			 				false, 0.5, 0, 5.0);
  		}
 		}
		free(pkt);
  }
}

void AVirtual_CPS_WorldPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &AVirtual_CPS_WorldPlayerController::OnSetDestinationReleased);

	// support touch devices
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AVirtual_CPS_WorldPlayerController::MoveToTouchLocation);

	InputComponent->BindAction("SelectItem", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::OnSelectItemPressed);

	InputComponent->BindAction("ScrollUp", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::ScrollUp);
	InputComponent->BindAction("ScrollDown", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::ScrollDown);

	InputComponent->BindAction("Pause", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::Pause).bExecuteWhenPaused = true;
	InputComponent->BindAction("NextCamera", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::NextCamera);

  InputComponent->BindAction("Slow", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::speedSlow);
  InputComponent->BindAction("Normal", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::speedNormal);
  InputComponent->BindAction("Fast", IE_Pressed, this, &AVirtual_CPS_WorldPlayerController::SpeedFast);
}

void AVirtual_CPS_WorldPlayerController::NextCamera() {
	if (cameras.Num() == 0) return;
	this->SetViewTargetWithBlend((ACameraActor *)cameras[currentCam], SmoothBlendTime);
	currentCam = (currentCam + 1 ) % cameras.Num();
}

void AVirtual_CPS_WorldPlayerController::Pause() {
  UE_LOG(LogNet, Log, TEXT("Paused"));
	UGameplayStatics::SetGamePaused(GetWorld(), !UGameplayStatics::IsGamePaused(GetWorld()));

  fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);

	//msg.add_id(ID);
	msg.add_type(UGameplayStatics::IsGamePaused(GetWorld()) ?
                                              UnrealCoojaMsg::MsgType_PAUSE :
                                              UnrealCoojaMsg::MsgType_RESUME);
	//msg.add_pir()
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);

}

void AVirtual_CPS_WorldPlayerController::speedSlow() {
  UE_LOG(LogNet, Log, TEXT("Speed at 0.1"));
  UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.1);

  fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);
	msg.add_type(slow ? UnrealCoojaMsg::MsgType_SPEED_NORM :
                      UnrealCoojaMsg::MsgType_SPEED_SLOW);
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);
  slow = !slow;
}

void AVirtual_CPS_WorldPlayerController::speedNormal() {
  UE_LOG(LogNet, Log, TEXT("Speed at 1.0"));
  UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0);

  fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);
	msg.add_type(UnrealCoojaMsg::MsgType_SPEED_NORM);
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);
}

void AVirtual_CPS_WorldPlayerController::speedFast() {
  UE_LOG(LogNet, Log, TEXT("Speed at 2.0"));
  UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 2.0);

  fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);
	msg.add_type(UnrealCoojaMsg::MsgType_SPEED_FAST);
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);
}

void AVirtual_CPS_WorldPlayerController::ScrollUp() {
	AVirtual_CPS_WorldCharacter* const Pawn = (AVirtual_CPS_WorldCharacter *) GetPawn();

		if (Pawn){
			FRotator rot = Pawn->GetCameraBoom()->RelativeRotation;
			rot.Add(5, 0, 0);
			Pawn->GetCameraBoom()->SetWorldRotation(rot);

		}
}

void AVirtual_CPS_WorldPlayerController::ScrollDown() {
	AVirtual_CPS_WorldCharacter* const Pawn = (AVirtual_CPS_WorldCharacter *) GetPawn();

		if (Pawn){
			FRotator rot = Pawn->GetCameraBoom()->RelativeRotation;
			rot.Add(-5, 0, 0);
			Pawn->GetCameraBoom()->SetWorldRotation(rot);
		}
}

void AVirtual_CPS_WorldPlayerController::MoveToMouseCursor()
{
	// Trace to see what is under the mouse cursor
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	if (Hit.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(Hit.ImpactPoint);
	}
}

void AVirtual_CPS_WorldPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void AVirtual_CPS_WorldPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const Pawn = GetPawn();
	if (Pawn)
	{
		UNavigationSystem* const NavSys = GetWorld()->GetNavigationSystem();
		float const Distance = FVector::Dist(DestLocation, Pawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if (NavSys && (Distance > 120.0f))
		{
			NavSys->SimpleMoveToLocation(this, DestLocation);
		}
	}
}

void AVirtual_CPS_WorldPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

void AVirtual_CPS_WorldPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}

void AVirtual_CPS_WorldPlayerController::OnSelectItemPressed()
{
	bSelectItem = true;

}

void AVirtual_CPS_WorldPlayerController::selectItem() {
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	AActor *item = Hit.GetActor();
	if (item != NULL) {
		if (item->GetClass()->GetName().Compare(sensorBlueprint->GetName()) == 0){
			UE_LOG(LogNet, Log, TEXT("Clicked %s"), *(item->GetName()));
			ASensor *sensorActor = (ASensor *)item->GetAttachParentActor();
			DrawDebugCircle(GetWorld(), item->GetActorLocation(), 150.0, 360, FColor(0, 255, 0), false, 0.3, 0, 5, FVector(0.f,1.f,0.f), FVector(0.f,0.f,1.f), false);
		}
	}
}
