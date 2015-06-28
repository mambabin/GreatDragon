#ifndef _NET_ID_HPP_
#define _NET_ID_HPP_

#include <zmq.hpp>
#include <sys/types.h>
#include <string>
#include <vector>

#define IP_BUF_LEN 16

void NetID_Init();

int32_t NetID_AddFD(int fd, const char *ip);

void NetID_ProtectID(int32_t id);
void NetID_CollectID(int32_t id);

int NetID_ExchangeID(int32_t prev, int32_t cur);

int32_t NetID_ID(int fd);
int NetID_FD(int32_t id);
const char * NetID_IP(int id);

void NetID_DelFD(int fd);
void NetID_DelID(int32_t id);

void * NetID_RecvBuf(int32_t id, int *len);
void NetID_AddDataLen(int32_t id, int len);
void NetID_ClearRecvBuf(int32_t id);

// -2: msg is wrong
// -100: overload
int NetID_ExtraMsg(int32_t id, int cur, zmq::message_t *msg);

void NetID_SendDataFD(std::vector<int> *fd);
const std::string * NetID_SendData(int fd);
void NetID_PopSendData(int fd);
void NetID_PushSendData(int32_t fd, const void *data, size_t size);

void NetID_AllFD(std::vector<int> *fds);
void NetID_AllID(std::vector<int32_t> *ids);

#endif
