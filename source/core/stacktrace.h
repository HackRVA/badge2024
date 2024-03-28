#ifndef STACKTRACE_H__
#ifdef TARGET_SIMULATOR

void stacktrace(char *msg);

#else

#define stacktrace(x)

#endif
#endif
