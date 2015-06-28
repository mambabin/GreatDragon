#ifndef _DC_MSG_PROCESSOR_HPP_
#define _DC_MSG_PROCESSOR_HPP_

#include <zmq.hpp>

void DCMsgProcessor_ProcessDCMsg(zmq::message_t *msg);

#endif
