#ifndef _GC_MSG_PROCESSOR_HPP_
#define _GC_MSG_PROCESSOR_HPP_

#include <zmq.hpp>

void GCMsgProcessor_ProcessNetMsg(zmq::message_t *msg);

void GCMsgProcessor_ProcessDCMsg(zmq::message_t *msg);

#endif
