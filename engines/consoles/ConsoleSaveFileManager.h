/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * This file is dual-licensed.
 * In addition to the GPLv3 license mentioned above, this code is also
 * licensed under LGPL 2.1. See LICENSES/COPYING.LGPL file for the
 * full text of the license.
 *
 */

#pragma once

#include "common/savefile.h"
#include "common/save-container-name.h"
#include <map> // For achievements list

#define INVALID_USER_ID 0xffff
#define MOUNT_NAME_TOTAL_LENGTH 32

class ConsoleSaveFileManager : public Common::SaveFileManager
{
public:
	ConsoleSaveFileManager();
	~ConsoleSaveFileManager();
	bool initialize(const int32 activityId, const Common::Language language) override;
	bool commitChanges() override;

	inline Common::String getContainerName(const int32 gameType, const Common::Language language) {
		if (gameType < static_cast<int32_t>(SaveDataContainerLibrary::GameType::Gob1) || gameType > static_cast<int32_t>(SaveDataContainerLibrary::GameType::Gob3)) {
			error("Cannot get container name for game %d!", gameType);
			return {};
		}

		const char* const containerName = RAG_GOB123_GetContainerName(static_cast<SaveDataContainerLibrary::GameType>(gameType), language);

		if (containerName == nullptr) {
			warning("Cannot get container name for language %d!", language);
			return {};
		}

		return containerName;
	}

	bool mountSaveData() override;
	bool umountSaveData() override;
	void pollEvents() override;
	int32_t read(const std::filesystem::path path, void* buffer) override { return 0; }
	int32_t save(const std::filesystem::path path, const void* buffer, const int32_t bufferSize) override { return 0; }

private:
	int32_t m_userId = 0xffff;
	bool m_initialized = false;
	char m_mountName[MOUNT_NAME_TOTAL_LENGTH] = "\0";
	std::map<uint32, uint32> m_achievements = std::map<uint32, uint32>();
};