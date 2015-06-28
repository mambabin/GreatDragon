#ifndef _NETAGENT_HPP_
#define _NETAGENT_HPP_

#include <zmq.hpp>

void NetAgent_Init(zmq::context_t *context);

void NetAgent_ProcessAlways();

#endif
