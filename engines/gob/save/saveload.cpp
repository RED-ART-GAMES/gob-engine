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

#include "gob/gob.h"
#include "gob/save/saveload.h"
#include "gob/global.h"
#include "gob/video.h"
#include "gob/draw.h"

namespace Gob {

	Common::String getSaveDataFilename(const Gob::GameType type, const int32 features, const Common::Language language) {
		switch (type) {
			// Language checks on Gob1 are unnecessary as each level is completely independent (no inventory saved).
			case Gob::GameType::kGameTypeGob1: return "gob1.data";
				
			case Gob::GameType::kGameTypeGob2: {
				switch (language) {
					case Common::Language::EN_ANY: [[fallthrough]];
					case Common::Language::EN_GRB: return "gob2_english_grb.data";
					case Common::Language::EN_USA: return "gob2_english_usa.data";
					case Common::Language::ES_ESP: return "gob2_spanish.data";
					case Common::Language::IT_ITA: return "gob2_italian.data";
					case Common::Language::FR_FRA: return "gob2_french.data";
					case Common::Language::DE_DEU: return "gob2_german.data";
				}

				break;
			}

			case Gob::GameType::kGameTypeGob3: {
				switch (language) {
					case Common::Language::EN_ANY: [[fallthrough]];
					case Common::Language::EN_GRB: return "gob3_english_grb.data";
					case Common::Language::EN_USA: return "gob3_english_usa.data";
					case Common::Language::ES_ESP: return "gob3_spanish.data";
					case Common::Language::IT_ITA: return "gob3_italian.data";
					case Common::Language::FR_FRA: return "gob3_french.data";
					case Common::Language::DE_DEU: return "gob3_german.data";
				}

				break;
			}
		}

		error("Can't find save data for game %d in language %d!", static_cast<int>(type), static_cast<int>(language));

		return "";
	}

	SaveLoad::SaveLoad(GobEngine* vm) : _vm(vm) {
	}

	SaveLoad::~SaveLoad() {
	}

	const char* SaveLoad::stripPath(const char* fileName, char separator) {
		const char* backSlash;
		if ((backSlash = strrchr(fileName, separator)))
			return backSlash + 1;

		return fileName;
	}


	Common::String SaveLoad::replacePathSeparators(const char* path, char newSeparator) {
		Common::String result = path;
		for (char& c : result) {
			if (c != newSeparator && (c == '\\' || c == '/' || c == ':'))
				c = newSeparator;
		}

		return result;
	}

	Common::List<Common::String> SaveLoad::getFilesMatchingPattern(const char* pattern) const {
		warning("SaveLoad::getFilesMatchingPattern not implemented");
		return Common::List<Common::String>();
	}

	int32 SaveLoad::getSize(const char* fileName) {
		info("Requested size of save file \"%s\"", fileName);

		SaveHandler* handler = getHandler(fileName);

		if (!handler) {
			warning("No save handler for \"%s\"", fileName);
			return -1;
		}

		int32 size = handler->getSize();

		info("Size is %d", size);

		return size;
	}

	bool SaveLoad::load(const char* fileName, int16 dataVar, int32 size, int32 offset) {
		info("Requested loading of save file \"%s\" - %d, %d, %d",
			fileName, dataVar, size, offset);

		SaveHandler* handler = getHandler(fileName);

		if (!handler) {
			warning("No save handler for \"%s\" (%d, %d, %d)", fileName, dataVar, size, offset);
			return false;
		}

		if (!handler->load(dataVar, size, offset)) {
			const char* desc = getDescription(fileName);

			if (!desc)
				desc = "Unknown";

			warning("Could not load %s (\"%s\" (%d, %d, %d))",
				desc, fileName, dataVar, size, offset);
			return false;
		}

		info("Successfully loaded game");
		return true;
	}

	bool SaveLoad::loadToRaw(const char* fileName, byte* ptr, int32 size, int32 offset) {
		info("Requested loading of save file \"%s\" - raw %p, %d, %d",
			fileName, (void*)ptr, size, offset);

		SaveHandler* handler = getHandler(fileName);

		if (!handler) {
			warning("No save handler for \"%s\" (raw %p, %d, %d)", fileName, (void*)ptr, size, offset);
			return false;
		}

		if (!handler->loadToRaw(ptr, size, offset)) {
			const char* desc = getDescription(fileName);

			if (!desc)
				desc = "Unknown";

			warning("Could not load %s (\"%s\" (raw %p, %d, %d))",
				desc, fileName, (void*)ptr, size, offset);
			return false;
		}

		info("Successfully loaded game");
		return true;
	}

	bool SaveLoad::save(const char* fileName, int16 dataVar, int32 size, int32 offset) {
		info("Requested saving of save file \"%s\" - %d, %d, %d",
			fileName, dataVar, size, offset);

		SaveHandler* handler = getHandler(fileName);

		if (!handler) {
			warning("No save handler for \"%s\" (%d, %d, %d)", fileName, dataVar, size, offset);
			return false;
		}

		if (!handler->save(dataVar, size, offset)) {
			const char* desc = getDescription(fileName);

			if (!desc)
				desc = "Unknown";

			warning("Could not save %s (\"%s\" (%d, %d, %d))",
				desc, fileName, dataVar, size, offset);
			return false;
		}

		info("Successfully saved game");
		return true;
	}

	bool SaveLoad::saveFromRaw(const char* fileName, byte* ptr, int32 size, int32 offset) {
		debug(SoftLevel::kFilesystem, "Requested saving of save file \"%s\" - raw %p, %d, %d",
			fileName, (void*)ptr, size, offset);

		SaveHandler* handler = getHandler(fileName);

		if (!handler) {
			warning("No save handler for \"%s\" (raw %p, %d, %d)", fileName, (void*)ptr, size, offset);
			return false;
		}

		if (!handler->saveFromRaw(ptr, size, offset)) {
			const char* desc = getDescription(fileName);

			if (!desc)
				desc = "Unknown";

			warning("Could not save %s (\"%s\" (raw %p, %d, %d))",
				desc, fileName, (void*)ptr, size, offset);
			return false;
		}

		info("Successfully saved game");
		return true;
	}

	bool SaveLoad::copySaveGame(const char* fileNameSrc, const char* fileNameDest) {
		SaveHandler* handlerSrc = getHandler(fileNameSrc);

		if (!handlerSrc) {
			warning("copySaveGame: no save handler for source \"%s\" ", fileNameSrc);
			return false;
		}

		SaveHandler* handlerDest = getHandler(fileNameDest);
		if (!handlerDest) {
			warning("copySaveGame: no save handler for destination \"%s\" ", fileNameDest);
			return false;
		}

		int32 size = handlerSrc->getSize();
		if (size == -1) {
			warning("copySaveGame: source file \"%s\" does not exists", fileNameSrc);
			return false;
		}

		byte* buffer = new byte[size];

		if (!handlerSrc->loadToRaw(buffer, size, 0)) {
			const char* desc = getDescription(fileNameSrc);

			if (!desc)
				desc = "Unknown";

			warning("Could not load %s (\"%s\") for copying to %s", desc, fileNameSrc, fileNameDest);
			delete[] buffer;
			return false;
		}

		if (!handlerDest->saveFromRaw(buffer, size, 0)) {
			const char* desc = getDescription(fileNameDest);

			if (!desc)
				desc = "Unknown";

			warning("Could not save %s (\"%s\") when copying from %s", desc, fileNameDest, fileNameSrc);
			delete[] buffer;
			return false;
		}

		info("Successfully copied saved game");
		delete[] buffer;
		return true;
	}

	SaveLoad::SaveMode SaveLoad::getSaveMode(const char* fileName) const {
		return kSaveModeNone;
	}

	SaveHandler* SaveLoad::getHandler(const char* fileName) const {
		return nullptr;
	}

	const char* SaveLoad::getDescription(const char* fileName) const {
		return nullptr;
	}

	bool SaveLoad::commitChanges(const char* filename) {
		return g_system->getSavefileManager()->commitChanges();
	}

} // End of namespace Gob
