#include <llapi/LoggerAPI.h>
#include "version.h"
#include <memory/hook.h>
extern Logger logger;
void PluginInit()
{
    logger.info("Hello, world!");
}
