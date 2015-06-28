#ifndef _MOVEMENT_HPP_
#define _MOVEMENT_HPP_

#include "Component.hpp"
#include "Math.hpp"
#include "MovementInfo.hpp"
#include <sys/types.h>

struct Movement;

void Movement_Init();

struct Movement * Movement_Create(MovementAtt *att, struct Component *component);
bool Movement_IsValid(struct Movement *movement);
void Movement_Destroy(struct Movement *movement);
void Movement_Finalize(struct Movement *movement);

const MovementAtt * Movement_Att(struct Movement *movement);

struct Component * Movement_Component(struct Movement *movement);

int32_t Movement_NextMap(struct Movement *movement);

int Movement_Line(struct Movement *movement);
void Movement_SetLine(struct Movement *movement, int line);

bool Movement_CanEnterRoom(struct Movement *movement, int32_t room, bool real, bool noPower);

int Movement_BeginWaitRoom(struct Movement *movement, int32_t room);
void Movement_EndWaitRoom(struct Movement *movement);
int Movement_RoomWaitCount(int32_t room);

RoomInfo * Movement_SearchRoomInfo(int32_t id);
int Movement_MultiRoom(struct Movement *movement);
void Movement_CreateRoom(struct Movement *movement, int32_t room, bool noPower);
void Movement_JoinRoom(struct Movement *movement, int32_t id, bool noPower);
void Movement_LeaveRoom(struct Movement *movement);
void Movement_DestroyRoom(struct Movement *movement, bool notice = true);
void Movement_EvictRole(struct Movement *movement, int pos);
void Movement_RoomList(struct Movement *movement);
void Movement_BeginMultiRoom(struct Movement *movement);

int32_t Movement_SerialNum(struct Movement *movement);
void Movement_AddSerialNum(struct Movement *movement, int32_t delta);

void Movement_Stop(struct Movement *movement);

int Movement_SetPosition(struct Movement *movement, const Vector3f *position);

void Movement_BeginChangeScene(struct Movement *movement, int32_t nextMap, const Vector2i *nextCoord, int curJump = -1, bool noPower = false);
void Movement_EndChangeScene(struct Movement *movement);
void Movement_EndLoadModel(struct Movement *movement);

bool Movement_InSingle(struct Movement *movement);

void Movement_SetSpeedFactor(struct Movement *movement, float factor);
float Movement_SpeedFactor(struct Movement *movement);

void Movement_SetCantMove(struct Movement *movement, bool cantMove);
bool Movement_CantMove(struct Movement *movement);

bool Movement_SameScene(struct Movement *a, struct Movement *b);

void Movement_SetOriginalOffsetPos(struct Movement *movement, const Vector3f *pos);
const Vector3f * Movement_OriginalOffsetPos(struct Movement *movement);

// 0: Succeed.
// -1: Failure.
int Movement_MoveStraightly(struct Movement *movement, const Vector2i *dest);

// 0: Succeed.
// -1: Failed to find path.
int Movement_MoveByAStar(struct Movement *movement, const Vector2i *dest);

int Movement_MoveByPathNode(struct Movement *movement, int dest);

int Movement_Follow(struct Movement *movement, struct Movement *target, float dist, int followDelta);

void Movement_Update(struct Movement *movement);

#endif
