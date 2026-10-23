#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <SimpleIni.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <set>
#include <thread>

using namespace std::literals;
namespace logger = SKSE::log;
