#ifndef _AI_HPP_
#define _AI_HPP_

#include "Component.hpp"
#include "AIInfo.hpp"

struct AI;

void AI_Init();

struct AI * AI_Create(AIAtt *att, struct Component *component);
bool AI_IsValid(struct AI *ai);
void AI_Destroy(struct AI *ai);

void AI_Prepare(struct AI *ai);

const AIAtt * AI_Att(struct AI *ai);

struct Component * AI_Component(struct AI *ai);

void AI_Idle(struct AI *ai);

void AI_BeAttacked(struct AI *ai, struct Fight *attacker);

void AI_Update(struct AI *ai);

#endif
