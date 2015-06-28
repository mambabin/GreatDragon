#ifndef _STATUS_HPP_
#define _STATUS_HPP_

#include "Component.hpp"
#include "StatusInfo.pb.h"
#include "StatusEntity.hpp"

struct Status;

void Status_Init();

struct Status * Status_Create(struct Component *component);
bool Status_IsValid(struct Status *status);
void Status_Destroy(struct Status *status);
void Status_Finalize(struct Status *status);

struct Component * Status_Component(struct Status *status);

int Status_GetType(struct Status *status, StatusInfo::StatusType type, struct StatusEntity **statuses, size_t size);

int Status_Add(struct Status *status, struct StatusEntity *entity);

// Only delete status within status component.
void Status_Del(struct Status *status, struct StatusEntity *entity);

// Delete status within status component and destroy it.
void Status_DelType(struct Status *status, StatusInfo::StatusType type);
void Status_DelTypes(struct Status *status, StatusInfo::StatusType types[], size_t size);

// Delete all statuses within status component and destroy it.
void Status_Clear(struct Status *status);

bool Status_HasType(struct Status *status, StatusInfo::StatusType type);
bool Status_HasTypes(struct Status *status, StatusInfo::StatusType types[], size_t size);

bool Status_IsControlOver(struct Status *status);

void Status_Update(struct Status *status);

#endif
