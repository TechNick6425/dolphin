// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <memory>

#include "MemoryWatcher.h"

#ifdef _WIN32
#include "MemoryWatcherWin.h"
#elif defined(__unix__) || defined(__APPLE__)
#include "MemoryWatcherUnix.h"
#endif

#ifdef _WIN32
static std::unique_ptr<MemoryWatcherWin> instance;
#elif defined(__unix__) || defined(__APPLE__)
static std::unique_ptr<MemoryWatcherUnix> instance;
#endif

#include "Common/FileUtil.h"
#include "Common/Logging/Log.h"
#include "Common/Thread.h"
#include "Core/HW/Memmap.h"

MemoryWatcher::MemoryWatcher()
{
#if defined(__unix__) || defined(__APPLE__)
	instance = std::make_unique<MemoryWatcherUnix>();
#elif defined(_WIN32)
	instance = std::make_unique<MemoryWatcherWin>();
#endif

	instance->Create();
}

MemoryWatcher::~MemoryWatcher()
{
#if defined(_WIN32) || defined(__unix__)
	instance->Destroy();
#endif
}

u32 MemoryWatcher::ChasePointer(const std::string& line)
{
	u32 value = 0;
	for (u32 offset : m_addresses[line])
		value = Memory::Read_U32(value + offset);
	return value;
}

std::string MemoryWatcher::ComposeMessage(const std::string& line, u32 value)
{
	std::stringstream message_stream;
	message_stream << line << '\n' << std::hex << value;
	return message_stream.str();
}

bool MemoryWatcher::LoadAddresses(const std::string& path)
{
	std::ifstream locations(path);
	if (!locations)
		return false;

	std::string line;
	while (std::getline(locations, line))
		ParseLine(line);

	return m_values.size() > 0;
}

void MemoryWatcher::ParseLine(const std::string& line)
{
	m_values[line] = 0;
	m_addresses[line] = std::vector<u32>();

	std::stringstream offsets(line);
	offsets >> std::hex;
	u32 offset;
	while (offsets >> offset)
		m_addresses[line].push_back(offset);
}