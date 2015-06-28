#ifndef _DCAGENT_HPP_
#define _DCAGENT_HPP_

#include "Config.hpp"
#include "Debug.hpp"
#include <zmq.hpp>
#include <sys/types.h>

void DCAgent_Init(zmq::context_t *context);

void DCAgent_ProcessAlways();
void DCAgent_Process(int32_t timeout);

// Don't use this function unless you know the format of msg.
// Use DCAgent_SendProto* instead of it.
void DCAgent_SendMsgToGCAgent(zmq::message_t *msg, bool noblock);

template<typename TProto>
void DCAgent_SendProtoToGCAgent(const TProto *proto, bool noblock = true) {
	if (proto == NULL)
		return;

	u_int8_t groupID = (u_int8_t)TProto::GROUPID;
	u_int8_t unitID = (u_int8_t)TProto::UNITID;

	zmq::message_t message(2 + proto->ByteSize());

	*((u_int8_t *)message.data()) = groupID;
	*((u_int8_t *)message.data() + 1) = unitID;
	if (!proto->SerializeToArray((int8_t *)message.data() + 2, message.size() - 2))
		return;

	DCAgent_SendMsgToGCAgent(&message, noblock);
}

#endif
