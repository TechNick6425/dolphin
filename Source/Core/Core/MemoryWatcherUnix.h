// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "MemoryWatcher.h"

class MemoryWatcherUnix : public MemoryWatcher
{
public:
	MemoryWatcherUnix();
	void Destroy();
	void Create();

protected:
	bool OpenSocket(const std::string& path);
	void WatcherThread();
};