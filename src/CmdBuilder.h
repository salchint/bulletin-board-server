//Filename:  CmdBuilder.h

#pragma once

#include <optional>
#include <string>
#include "SessionResources.h"
#include "ThreadPool.h"

std::optional<ThreadPool::Commands_t> build_command(const std::string commandId, const char* line, SessionResources& resources, ConnectionQueue* qu = nullptr);

