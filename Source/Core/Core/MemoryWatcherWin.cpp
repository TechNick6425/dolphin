// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <windows.h>
#include <iostream>

#include "Common/FileUtil.h"
#include "Common/Logging/Log.h"
#include "Common/Thread.h"
#include "Core/MemoryWatcherWin.h"
#include "Core/HW/Memmap.h"

// We don't want to kill the cpu, so sleep for this long after polling.
static const int SLEEP_DURATION = 2; // ms

static HANDLE m_pipe;

void MemoryWatcherWin::Create()
{
	if (!LoadAddresses(File::GetUserPath(F_MEMORYWATCHERLOCATIONS_IDX)))
		return;
	if (!OpenSocket(File::GetUserPath(F_MEMORYWATCHERSOCKET_IDX)))
		return;
	m_running = true;
	m_watcher_thread = std::thread(&MemoryWatcherWin::WatcherThread, this);
}

void MemoryWatcherWin::Destroy()
{
	if (!m_running)
		return;

	m_running = false;
	m_watcher_thread.join();

	CloseHandle(m_pipe);
}

bool MemoryWatcherWin::OpenSocket(const std::string& path)
{
	m_pipe = CreateNamedPipe(L"\\\\.\\dolphin-emu\\mem-watch", 
		PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT,
		1,
		0,
		0,
		0,
		nullptr);

	if(m_pipe == INVALID_HANDLE_VALUE) return false;
	return true;
}

void MemoryWatcherWin::WatcherThread()
{
	if(!ConnectNamedPipe(m_pipe, NULL))
	{
		INFO_LOG("Unable to connect WIN32 named pipe for MemoryWatcher.");
		return;
	}

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
				if(!WriteFile(m_pipe, message, sizeof(char) * message.length(), NULL, NULL))
				{
					INFO_LOG("Unable to update memory addresses for MemoryWatcher.");
				}
			}
		}
		Common::SleepCurrentThread(SLEEP_DURATION);
	}
}
