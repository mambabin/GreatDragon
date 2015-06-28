#ifndef _SIGNAL_HANDLER_HPP_
#define _SIGNAL_HANDLER_HPP_

void SignalHandler_NormalExit(int sigNum);

void SignalHandler_ExitAndLogCallStack(int sigNum);

bool SignalHandler_IsOver();

#endif
