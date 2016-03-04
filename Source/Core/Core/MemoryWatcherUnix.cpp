// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "Common/FileUtil.h"
#include "Common/Logging/Log.h"
#include "Common/Thread.h"
#include "Core/MemoryWatcherUnix.h"
#include "Core/HW/Memmap.h"

// We don't want to kill the cpu, so sleep for this long after polling.
static const int SLEEP_DURATION = 2; // ms

static int m_fd;
static sockaddr_un m_addr;

void MemoryWatcherUnix::Create()
{
	if (!LoadAddresses(File::GetUserPath(F_MEMORYWATCHERLOCATIONS_IDX)))
		return;
	if (!OpenSocket(File::GetUserPath(F_MEMORYWATCHERSOCKET_IDX)))
		return;
	m_running = true;
	m_watcher_thread = std::thread(&MemoryWatcherUnix::WatcherThread, this);
}

void MemoryWatcherUnix::Destroy()
{
	if (!m_running)
		return;

	m_running = false;
	m_watcher_thread.join();

	close(m_fd);
}

bool MemoryWatcherUnix::OpenSocket(const std::string& path)
{
	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sun_family = AF_UNIX;
	strncpy(m_addr.sun_path, path.c_str(), sizeof(m_addr.sun_path) - 1);

	m_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	return m_fd >= 0;
}

void MemoryWatcherUnix::WatcherThread()
{
	while (m_running)
	{
		for (auto& entry : m_values)
		{
			std::string address = entry.first;
			u32& current_value = entry.second;

			u32 new_value = ChasePointer(address);
			if (new_value != current_value)
			{
				// Update the value
				current_value = new_value;
				std::string message = ComposeMessage(address, new_value);
				sendto(
					m_fd,
					message.c_str(),
					message.size() + 1,
					0,
					reinterpret_cast<sockaddr*>(&m_addr),
					sizeof(m_addr));
			}
		}
		Common::SleepCurrentThread(SLEEP_DURATION);
	}
}
