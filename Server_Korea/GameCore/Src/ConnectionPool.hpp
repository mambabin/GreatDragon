#ifndef _CONNECTION_POOL_HPP_
#define _CONNECTION_POOL_HPP_

#include <sys/types.h>

void ConnectionPool_Init();
void ConnectionPool_Prepare();

void ConnectionPool_Add(int32_t id);
void ConnectionPool_Del(int32_t id);

int ConnectionPool_TotalCount();

void ConnectionPool_DoHeartbeat(int32_t id);

#endif
