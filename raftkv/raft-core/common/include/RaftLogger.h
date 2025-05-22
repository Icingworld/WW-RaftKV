#pragma once

#include <Logger.h>
#include <ConsoleSink.h>

namespace WW
{

#define DEBUG(x) Logger::getSyncLogger("RaftLogger").debug(x)
#define INFO(x) Logger::getSyncLogger("RaftLogger").info(x)
#define WARN(x) Logger::getSyncLogger("RaftLogger").warn(x)
#define ERROR(x) Logger::getSyncLogger("RaftLogger").error(x)

} // namespace WW
