#ifndef _DEBUG_HPP_
#define _DEBUG_HPP_

void Debug_Init();

// thread safety
void Debug_Log(const char *file, int line, const char *format, ...);
// thread safety
void Debug_LogError(const char *file, int line, const char *format, ...);
// thread safety
void Debug_LogRecord(const char *file, int line, const char *format, ...);

#define DEBUG_LOG(format, ...) Debug_Log(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define DEBUG_LOGERROR(format, ...) Debug_LogError(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define DEBUG_LOGRECORD(format, ...) Debug_LogRecord(__FILE__, __LINE__, format, ##__VA_ARGS__)

void Debug_LogCallStack();

#endif
