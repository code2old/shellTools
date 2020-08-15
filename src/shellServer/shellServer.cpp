#include "shellServer.h"

int initShellServer(void);
int regShellCmd(const char *shellCmdName, func callback, void *data);