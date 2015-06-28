#ifndef _GCAGENT_HPP_
#define _GCAGENT_HPP_

#include "PlayerPool.hpp"
#include "MapPool.hpp"
#include "Config.hpp"
#include "Debug.hpp"
#include <zmq.hpp>
#include <sys/types.h>
#include <vector>

#define ORDER_CLOSE_CLIENT 0
#define ORDER_PROTECT_ID 1
#define ORDER_COLLECT_ID 2
#define ORDER_EXCHANGE_ID 3
#define ORDER_NEW_CLIENT 4

void GCAgent_Init();

void GCAgent_Process(int32_t timeout);

void GCAgent_WaitDCAgent(int count);

zmq::context_t * GCAgent_Context();

// 0, id --Close client
void GCAgent_SendOrderToNetAgent(u_int8_t order, int32_t arg1, int32_t arg2);

// Don't use these functions unless you know the format of msg.
// Use GCAgent_SendProto* instead of it.
void GCAgent_SendMsgToNetAgent(zmq::message_t *msg, bool noblock);
void GCAgent_SendMsgToDCAgent(zmq::message_t *msg, bool noblock);

template<typename TProto>
void GCAgent_SendProtoToDCAgent(const TProto *proto, bool noblock = true) {
	if (proto == NULL)
		return;

	u_int8_t groupID = (u_int8_t)TProto::GROUPID;
	u_int8_t unitID = (u_int8_t)TProto::UNITID;

	zmq::message_t message(2 + proto->ByteSize());

	*((u_int8_t *)message.data()) = groupID;
	*((u_int8_t *)message.data() + 1) = unitID;
	if (!proto->SerializeToArray((int8_t *)message.data() + 2, message.size() - 2))
		return;

	GCAgent_SendMsgToDCAgent(&message, noblock);
}

// count id(0) id(1) ... id(count - 1) data
template<typename TProto>
void GCAgent_SendProtoToClients(const int32_t *ids, size_t size, const TProto *proto, bool noblock = true) {
	if (ids == NULL || size == 0 || proto == NULL)
		return;
	if (size == (size_t)-1)
		return;

	u_int8_t groupID = (u_int8_t)TProto::GROUPID;
	u_int8_t unitID = (u_int8_t)TProto::UNITID;

	zmq::message_t message(4 * (size + 1) + 2 + proto->ByteSize());

	*(int32_t *)message.data() = size;
	for (size_t i = 0; i < size; i++) {
		*((int32_t *)message.data() + 1 + i) = ids[i];
	}
	*((u_int8_t *)message.data() + 4 * (size + 1)) = groupID;
	*((u_int8_t *)message.data() + 4 * (size + 1) + 1) = unitID;
	if (!proto->SerializeToArray((int8_t *)message.data() + 4 * (size + 1) + 2, message.size() - 4 * (size + 1) - 2))
		return;

	GCAgent_SendMsgToNetAgent(&message, noblock);
}

// -1 data
template<typename TProto>
void GCAgent_SendProtoToAllClients(const TProto *proto, bool noblock = true) {
	if (proto == NULL)
		return;

	u_int8_t groupID = (u_int8_t)TProto::GROUPID;
	u_int8_t unitID = (u_int8_t)TProto::UNITID;

	zmq::message_t message(6 + proto->ByteSize());

	*(int32_t *)message.data() = -1;
	*((u_int8_t *)message.data() + 4) = groupID;
	*((u_int8_t *)message.data() + 5) = unitID;
	if (!proto->SerializeToArray((int8_t *)message.data() + 6, message.size() - 6))
		return;

	GCAgent_SendMsgToNetAgent(&message, noblock);
}

// -2 id data
template<typename TProto>
void GCAgent_SendProtoToAllClientsExceptOne(int32_t id, const TProto *proto, bool noblock = true) {
	if (proto == NULL)
		return;

	u_int8_t groupID = (u_int8_t)TProto::GROUPID;
	u_int8_t unitID = (u_int8_t)TProto::UNITID;

	zmq::message_t message(10 + proto->ByteSize());

	*(int32_t *)message.data() = -2;
	*((int32_t *)message.data() + 1) = id;
	*((u_int8_t *)message.data() + 8) = groupID;
	*((u_int8_t *)message.data() + 9) = unitID;
	if (!proto->SerializeToArray((int8_t *)message.data() + 10, message.size() - 10))
		return;

	GCAgent_SendMsgToNetAgent(&message, noblock);
}

template<typename TProto>
void GCAgent_SendProtoToAllClientsInSameScene(int32_t map, struct PlayerEntity *player, const TProto *proto, bool noblock = true) {
	if (proto == NULL)
		return;

	static std::vector<struct PlayerEntity *> players;
	players.clear();
	int num = PlayerPool_Players(map, player, &players);
	if (num <= 0)
		return;

	int32_t ids[CONFIG_FIXEDARRAY];
	for (int i = 0; i < num; i++) {
		ids[i] = PlayerEntity_ID(players[i]);
	}

	GCAgent_SendProtoToClients(ids, num, proto, noblock);
}

template<typename TProto>
void GCAgent_SendProtoToAllClientsInSameScene(int32_t map, const TProto *proto, bool noblock = true) {
	if (proto == NULL)
		return;

	static std::vector<struct PlayerEntity *> players;
	players.clear();
	int num = PlayerPool_Players(map, &players);
	if (num <= 0)
		return;

	int32_t ids[CONFIG_FIXEDARRAY];
	for (int i = 0; i < num; i++) {
		ids[i] = PlayerEntity_ID(players[i]);
	}

	GCAgent_SendProtoToClients(ids, num, proto, noblock);
}

template<typename TProto>
void GCAgent_SendProtoToAllClientsExceptOneInSameScene(struct PlayerEntity *player, const TProto *proto, bool noblock = true) {
	if (player == NULL || proto == NULL)
		return;

	static std::vector<struct PlayerEntity *> players;
	players.clear();
	struct Component *component = PlayerEntity_Component(player);
	if (component == NULL)
		return;
	int num = PlayerPool_Players(component->roleAtt->movementAtt().mapID(), player, &players);
	if (num <= 0)
		return;

	int32_t ids[CONFIG_FIXEDARRAY];
	int count = 0;
	for (int i = 0; i < num; i++) {
		if (players[i] == player)
			continue;
		ids[count++] = PlayerEntity_ID(players[i]);
	}
	if (count <= 0)
		return;

	GCAgent_SendProtoToClients(ids, count, proto, noblock);
}

#endif
