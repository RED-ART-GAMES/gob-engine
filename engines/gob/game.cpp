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

#include "common/_endian.h"
#include "common/str.h"
#include "common/graphics.h"

#include "gob/gob.h"
#include "gob/game.h"
#include "gob/global.h"
#include "gob/dataio.h"
#include "gob/variables.h"
#include "gob/script.h"
#include "gob/resources.h"
#include "gob/hotspots.h"
#include "gob/inter.h"
#include "gob/draw.h"
#include "gob/mult.h"
#include "gob/scenery.h"
#include "gob/videoplayer.h"
#include "gob/sound/sound.h"
#include "gob/save/saveload.h" // To get save data file names & paths
#include "nlohmann/json.hpp"
#include <iostream>

namespace Gob {

	/// <summary>
	/// Filename that contains all hotspots, read from the ROM filesystem
	/// </summary>
	constexpr char* HOTSPOTS_FILE = (char*)"HOTSPOTS.json";

	Environments::Environments(GobEngine* vm) : _vm(vm) {
		for (uint i = 0; i < kEnvironmentCount; i++) {
			Environment& e = _environments[i];
			Media& m = _media[i];

			e.cursorHotspotX = 0;
			e.cursorHotspotY = 0;
			e.variables = nullptr;
			e.script = nullptr;
			e.resources = nullptr;

			for (int j = 0; j < 17; j++)
				m.fonts[j] = nullptr;
		}
	}

	Environments::~Environments() {
		clear();
	}

	void Environments::clear() {
		// Deleting unique variables, script and resources

		for (uint i = 0; i < kEnvironmentCount; i++) {
			if (_vm->_inter && (_environments[i].variables == _vm->_inter->_variables))
				continue;

			if (!has(_environments[i].variables, i + 1))
				delete _environments[i].variables;
		}

		for (uint i = 0; i < kEnvironmentCount; i++) {
			if (_environments[i].script == _vm->_game->_script)
				continue;

			if (!has(_environments[i].script, i + 1))
				delete _environments[i].script;
		}

		for (uint i = 0; i < kEnvironmentCount; i++) {
			if (_environments[i].resources == _vm->_game->_resources)
				continue;

			if (!has(_environments[i].resources, i + 1))
				delete _environments[i].resources;
		}

		for (uint i = 0; i < kEnvironmentCount; i++)
			clearMedia(i);
	}

	void Environments::set(uint8 env) {
		if (env >= kEnvironmentCount)
			return;

		Environment& e = _environments[env];

		// If it already has a unique script or resource assigned, delete them
		if ((e.script != _vm->_game->_script) && !has(e.script, 0, env))
			delete e.script;
		if ((e.resources != _vm->_game->_resources) && !has(e.resources, 0, env))
			delete e.resources;

		e.cursorHotspotX = _vm->_draw->_cursorHotspotXVar;
		e.cursorHotspotY = _vm->_draw->_cursorHotspotYVar;
		e.script = _vm->_game->_script;
		e.resources = _vm->_game->_resources;
		e.variables = _vm->_inter->_variables;
		e.totFile = _vm->_game->_curTotFile;
	}

	void Environments::checkIfInMenu(uint8 env) const {
		if (env >= kEnvironmentCount)
			return;

		const Environment& e = _environments[env];

		// Part of condition to fix issue #24162
		//
		// save the current tot, in order to know the progression
		// GOB21.TOT -> MENU.TOT -> ... ?
		if (e.totFile.equalsIgnoreCase("MENU.TOT")) {
			debug(kInterpreter, "Saving '%s' as previous loaded TOT file (from MENU.TOT)...", _vm->_game->_curTotFile.c_str());
			_vm->_game->_menuLoadedFromTot = _vm->_game->_curTotFile;
		}
	}

	void Environments::get(uint8 env) const {
		if (env >= kEnvironmentCount)
			return;

		const Environment& e = _environments[env];

		_vm->_draw->_cursorHotspotXVar = e.cursorHotspotX;
		_vm->_draw->_cursorHotspotYVar = e.cursorHotspotY;
		_vm->_game->_script = e.script;
		_vm->_game->_resources = e.resources;
		_vm->_inter->_variables = e.variables;
		_vm->_game->_curTotFile = e.totFile;
	}

	const Common::String& Environments::getTotFile(uint8 env) const {
		assert(env < kEnvironmentCount);

		return _environments[env].totFile;
	}

	bool Environments::has(Variables* variables, uint8 startEnv, int16 except) const {
		for (uint i = startEnv; i < kEnvironmentCount; i++) {
			if ((except >= 0) && (((uint16)except) == i))
				continue;

			if (_environments[i].variables == variables)
				return true;
		}

		return false;
	}

	bool Environments::has(Script* script, uint8 startEnv, int16 except) const {
		for (uint i = startEnv; i < kEnvironmentCount; i++) {
			if ((except >= 0) && (((uint16)except) == i))
				continue;

			if (_environments[i].script == script)
				return true;
		}

		return false;
	}

	bool Environments::has(Resources* resources, uint8 startEnv, int16 except) const {
		for (uint i = startEnv; i < kEnvironmentCount; i++) {
			if ((except >= 0) && (((uint16)except) == i))
				continue;

			if (_environments[i].resources == resources)
				return true;
		}

		return false;
	}

	void Environments::deleted(Variables* variables) {
		for (uint i = 0; i < kEnvironmentCount; i++) {
			if (_environments[i].variables == variables)
				_environments[i].variables = nullptr;
		}
	}

	bool Environments::clearMedia(uint8 env) {
		if (env >= kEnvironmentCount)
			return false;

		Media& m = _media[env];

		for (int i = 0; i < 10; i++)
			m.sprites[i].reset();

		for (int i = 0; i < 10; i++)
			m.sounds[i].free();

		for (int i = 0; i < 17; i++) {
			delete m.fonts[i];
			m.fonts[i] = nullptr;
		}

		return true;
	}

	bool Environments::setMedia(uint8 env) {
		if (env >= kEnvironmentCount)
			return false;

		clearMedia(env);

		Media& m = _media[env];

		for (int i = 0; i < 10; i++) {
			m.sprites[i] = _vm->_draw->_spritesArray[i];
			_vm->_draw->_spritesArray[i].reset();
		}

		for (int i = 0; i < 10; i++) {
			SoundDesc* sound = _vm->_sound->sampleGetBySlot(i);
			if (sound)
				m.sounds[i].swap(*sound);
		}

		int n = MIN(Draw::kFontCount, 17);
		for (int i = 0; i < n; i++) {
			m.fonts[i] = _vm->_draw->_fonts[i];
			_vm->_draw->_fonts[i] = nullptr;
		}

		return true;
	}

	bool Environments::getMedia(uint8 env) {
		if (env >= kEnvironmentCount)
			return false;

		Media& m = _media[env];

		for (int i = 0; i < 10; i++) {
			_vm->_draw->_spritesArray[i] = m.sprites[i];
			m.sprites[i].reset();
		}

		for (int i = 0; i < 10; i++) {
			SoundDesc* sound = _vm->_sound->sampleGetBySlot(i);
			if (sound)
				m.sounds[i].swap(*sound);
			m.sounds[i].free();
		}

		int n = MIN(Draw::kFontCount, 17);
		for (int i = 0; i < n; i++) {
			delete _vm->_draw->_fonts[i];
			_vm->_draw->_fonts[i] = m.fonts[i];
			m.fonts[i] = nullptr;
		}

		return true;
	}


	TotFunctions::TotFunctions(GobEngine* vm) : _vm(vm) {
		for (uint8 i = 0; i < kTotCount; i++) {
			_tots[i].script = nullptr;
			_tots[i].resources = nullptr;
		}
	}

	TotFunctions::~TotFunctions() {
		for (uint8 i = 0; i < kTotCount; i++)
			freeTot(_tots[i]);
	}

	bool TotFunctions::loadTot(Tot& tot, const Common::String& file) {
		tot.script = new Script(_vm);
		tot.resources = new Resources(_vm);

		if (!tot.script->load(file) || !tot.resources->load(file)) {
			freeTot(tot);
			return false;
		}

		return true;
	}

	void TotFunctions::freeTot(Tot& tot) {
		delete tot.script;
		delete tot.resources;

		tot.script = nullptr;
		tot.resources = nullptr;

		tot.file.clear();
		tot.functions.clear();
	}

	bool TotFunctions::loadIDE(Tot& tot) {
		// Mapping file of function names -> function numbers/offsets
		Common::String ideFile = Util::setExtension(tot.file, ".IDE");
		Common::SeekableReadStream* ide = _vm->_dataIO->getFile(ideFile);
		if (!ide)
			// No mapping file => No named functions
			return true;

		char buffer[17];

		uint32 count = ide->readUint16LE();
		for (uint32 i = 0; i < count; i++) {
			Function function;

			function.type = ide->readByte();

			ide->read(buffer, 17);
			buffer[16] = '\0';

			function.name = buffer;

			ide->skip(2); // Unknown;
			function.offset = ide->readUint16LE();
			ide->skip(2); // Unknown;

			if ((function.type != 0x47) && (function.type != 0x67))
				continue;

			tot.script->seek(function.offset);

			if (tot.script->readByte() != 1) {
				warning("TotFunctions::loadIDE(): IDE corrupt");
				return false;
			}

			debug(SoftLevel::kInterpreter, "Function 0x%02X: \"%s\"", function.type,
				function.name.c_str());
			tot.functions.push_back(function);
		}

		tot.script->seek(0);
		return true;
	}

	int TotFunctions::find(const Common::String& totFile) const {
		for (int i = 0; i < kTotCount; i++)
			if (_tots[i].file.equalsIgnoreCase(totFile))
				return i;

		return -1;
	}

	int TotFunctions::findFree() const {
		for (int i = 0; i < kTotCount; i++)
			if (_tots[i].file.empty())
				return i;

		return -1;
	}

	bool TotFunctions::load(const Common::String& totFile) {
		if (find(totFile) >= 0) {
			warning("TotFunctions::load(): \"%s\" already loaded", totFile.c_str());
			return false;
		}

		int index = findFree();
		if (index < 0) {
			warning("TotFunctions::load(): No free space for \"%s\"", totFile.c_str());
			return false;
		}

		Tot& tot = _tots[index];
		if (!loadTot(tot, totFile))
			return false;

		tot.file = totFile;

		if (!loadIDE(tot)) {
			freeTot(tot);
			return false;
		}

		return true;
	}

	bool TotFunctions::unload(const Common::String& totFile) {
		int index = find(totFile);
		if (index < 0) {
			warning("TotFunctions::unload(): \"%s\" not loaded", totFile.c_str());
			return false;
		}

		Tot& tot = _tots[index];

		if (_vm->_game->_script == tot.script)
			_vm->_game->_script = nullptr;
		if (_vm->_game->_resources == tot.resources)
			_vm->_game->_resources = nullptr;

		freeTot(tot);

		return true;
	}

	bool TotFunctions::call(const Common::String& totFile,
		const Common::String& function) const {

		int index = find(totFile);
		if (index < 0) {
			warning("TotFunctions::call(): No such TOT \"%s\"", totFile.c_str());
			return false;
		}

		const Tot& tot = _tots[index];

		uint16 offset = 0;
		Common::List<Function>::const_iterator it;
		for (it = tot.functions.begin(); it != tot.functions.end(); ++it) {
			if (it->name.equalsIgnoreCase(function)) {
				offset = it->offset;
				break;
			}
		}

		if (offset == 0) {
			warning("TotFunctions::call(): No such function \"%s\" in \"%s\"",
				function.c_str(), totFile.c_str());
			return false;
		}

		return call(tot, offset);
	}

	bool TotFunctions::call(const Common::String& totFile, uint16 offset) const {
		int index = find(totFile);
		if (index < 0) {
			warning("TotFunctions::call(): No such TOT \"%s\"", totFile.c_str());
			return false;
		}

		return call(_tots[index], offset);
	}

	bool TotFunctions::call(const Tot& tot, uint16 offset) const {
		Script* script = _vm->_game->_script;
		Resources* resources = _vm->_game->_resources;
		Common::String curtotFile = _vm->_game->_curTotFile;

		_vm->_game->_script = tot.script;
		_vm->_game->_resources = tot.resources;
		_vm->_game->_curTotFile = tot.file;

		_vm->_game->playTot(offset);

		_vm->_game->_script = script;
		_vm->_game->_resources = resources;
		_vm->_game->_curTotFile = curtotFile;

		return true;
	}


	Game::Game(GobEngine* vm, bool _skipIntro) : _vm(vm), _environments(_vm), _totFunctions(_vm) {
		_captureCount = 0;

		_startTimeKey = 0;
		_mouseButtons = kMouseButtonsNone;

		_handleMouse = 0;
		_forceHandleMouse = 0;
		_noScroll = true;
		_preventScroll = false;

		_wantScroll = false;
		_wantScrollX = 0;
		_wantScrollY = 0;

		_tempStr[0] = 0;

		_numEnvironments = 0;
		_curEnvironment = 0;

		skipIntro = _skipIntro;

		_script = new Script(_vm);
		_resources = new Resources(_vm);
		_hotspots = new Hotspots(_vm);
		_managedCheats = new ManagedCheats();
	}

	Game::~Game() {
		delete _script;
		delete _resources;
		delete _hotspots;
		delete _managedCheats;
	}

	void Game::prepareStart() {
		_vm->_global->_pPaletteDesc->unused2 = _vm->_draw->_unusedPalette2;
		_vm->_global->_pPaletteDesc->unused1 = _vm->_draw->_unusedPalette1;
		_vm->_global->_pPaletteDesc->vgaPal = _vm->_draw->_vgaPalette;

		_vm->_video->setFullPalette(_vm->_global->_pPaletteDesc);

		_vm->_draw->initScreen();
		_vm->_draw->_frontSurface->fillRect(0, 0,
			_vm->_video->_surfWidth - 1, _vm->_video->_surfHeight - 1, 1);

		_vm->_util->setMousePos(152, 92);
		_vm->_draw->_cursorX = _vm->_global->_inter_mouseX = 152;
		_vm->_draw->_cursorY = _vm->_global->_inter_mouseY = 92;

		_vm->_draw->_invalidatedCount = 0;
		_vm->_draw->_noInvalidated = true;
		_vm->_draw->_applyPal = false;
		_vm->_draw->_paletteCleared = false;

		for (int i = 0; i < 40; i++) {
			_vm->_draw->_cursorAnimLow[i] = -1;
			_vm->_draw->_cursorAnimDelays[i] = 0;
			_vm->_draw->_cursorAnimHigh[i] = 0;
		}

		_vm->_draw->_renderFlags = 0;
		_vm->_draw->_backDeltaX = 0;
		_vm->_draw->_backDeltaY = 0;

		_startTimeKey = _vm->_util->getTimeKey();

#pragma region Load Hotspots
		// LOAD HOTSPOTS
		{
			Common::String pathExtension = Common::String("");
			if (_vm->getGameType() == kGameTypeGob1)      pathExtension = Common::String("Gob1");
			else if (_vm->getGameType() == kGameTypeGob2) pathExtension = Common::String("Gob2");
			else if (_vm->getGameType() == kGameTypeGob3) pathExtension = Common::String("Gob3");
			else error("unknown path extension to get %s file...", HOTSPOTS_FILE);

			Common::InSaveFile* hotspotsFilePath = _vm->_system->getSavefileManager()->openFileForLoading(Common::Path(pathExtension).appendComponent(HOTSPOTS_FILE).toString());
			if (NULL != hotspotsFilePath) {
				int64 fileSize = hotspotsFilePath->size();
				char* stream = (char*)malloc(1 + (sizeof(char) * fileSize));
				if (NULL == stream) error("cannot allocate memory for the hotspots file stream!");
				memset(stream, 0x0, fileSize + 1);
				hotspotsFilePath->read(stream, fileSize);

				nlohmann::json data = nlohmann::json::parse(stream, NULL, true, true);

				if (!data.is_null() && data.is_object()) {
					auto obj = data.get<nlohmann::json::object_t>();
					for (auto& [level, subobject] : obj) {
						// objects_to_display parser...
						// >= 0x8000 as hotspot ids begin at 0x8000 in interpreter code !
						// std::vector<std::string>("32664", "32665", ...);
						if (!subobject["objects_to_display"].is_null()) {
							auto objects_to_display = subobject["objects_to_display"].template get<std::vector<std::string>>();
							std::vector<std::int16_t> objects_x_offset;
							std::vector<std::int16_t> objects_y_offset;

							if (!subobject["x_offset"].is_null()) {
								objects_x_offset = subobject["x_offset"].template get<std::vector<std::int16_t>>();
							}
							if (!subobject["y_offset"].is_null()) {
								objects_y_offset = subobject["y_offset"].template get<std::vector<std::int16_t>>();
							}

							uint16 i = 0;
							for (const auto& object : objects_to_display) {
								uint16 objectId = atoi(object.c_str());
								int16 x_offset = 0;
								int16 y_offset = 0;

								if (objects_x_offset.size() >= (i + 1)) {
									x_offset = objects_x_offset[i];
								}
								if (objects_y_offset.size() >= (i + 1)) {
									y_offset = objects_y_offset[i];
								}

								_vm->_game->_managedCheats->allowToDisplay(level, Gob::ManagedCheats::Kind::kObject, objectId, x_offset, y_offset);
								i++;
							}
						}
						else {
							warning("No 'objects_to_display' key in %s, for level %s! No game object cheats to display by default...", HOTSPOTS_FILE, level.c_str());
						}
						// hotspots_to_display parser...
						// >= 0 as object ids begin at 0x0 in interpreter code !
						// std::vector<std::string>("0", "1", "2", ...);
						if (!subobject["hotspots_to_display"].is_null()) {
							auto hotspots_to_display = subobject["hotspots_to_display"].template get<std::vector<std::string>>();

							std::vector<std::int16_t> hotspots_x_offset;
							std::vector<std::int16_t> hotspot_y_offset;

							if (!subobject["x_offset"].is_null()) {
								hotspots_x_offset = subobject["x_offset"].template get<std::vector<std::int16_t>>();
							}
							if (!subobject["y_offset"].is_null()) {
								hotspot_y_offset = subobject["y_offset"].template get<std::vector<std::int16_t>>();
							}

							uint16 i = 0;
							for (const auto& object : hotspots_to_display) {
								int16 x_offset = 0;
								int16 y_offset = 0;

								if (hotspots_x_offset.size() >= (i + 1)) {
									x_offset = hotspots_x_offset[i];
								}
								if (hotspot_y_offset.size() >= (i + 1)) {
									y_offset = hotspot_y_offset[i];
								}

								uint16 objectId = atoi(object.c_str());
								_vm->_game->_managedCheats->allowToDisplay(level, Gob::ManagedCheats::Kind::kHotspot, objectId, x_offset, y_offset);
								i++;
							}
						}
						else {
							warning("No 'hotspots_to_display' key in %s, for level %s! No game hotspot cheats to display by default...", HOTSPOTS_FILE, level.c_str());
						}
						// custom_hotspots parser
						// {
						// "name": "custom_id", // Must be a string, can be considered as an id (to display in debug mode) !!!
						// "x" : 10, // top left coordinate (x axis)
						// "y" : 10, // top left coordinate (y axis)
						// "w" : 1,  // width of the hotspot
						// "h" : 1   // height of the hotspot
						// }
						if (!subobject["custom_hotspots"].is_null()) {
							auto custom_hotspots = subobject["custom_hotspots"].get<std::vector<nlohmann::json::object_t>>();
							for (const auto& hotspot : custom_hotspots) {
								const std::string id = hotspot.find("name")->second;

								const int16_t left   = hotspot.find("x")->second;
								const int16_t top    = hotspot.find("y")->second;

								const int16_t right  = left + static_cast<int16_t>(hotspot.find("w")->second);
								const int16_t bottom = top  + static_cast<int16_t>(hotspot.find("h")->second);

								/*
								* Converting from virtual to window coordinates here would be wrong. We need to apply the transformation
								* on virtual coordinates every time the aspect ratio change, so we must keep the original coordinates.
								*/
								_vm->_game->_managedCheats->addCustomCheat(level, Common::String(id.c_str()), { left, top }, { right, bottom });
							}
						}
						else {
							warning("No 'custom_hotspots' key in %s, for level %s! No custom hotspot cheats to display by default...", HOTSPOTS_FILE, level.c_str());
						}
					}
				}

				free(stream);
			}
			else {
				warning("Did not found any '%s' file in the current setting... please check it!!", HOTSPOTS_FILE);
			}
		}
#pragma endregion Load Hotspots

		const bool useSavedProgressData = !_vm->_wipeDataAtLaunch;

#pragma region Gob1 check
		// HACK for Gobliiins 1,
		// In order to launch the last played level at startup, we save the last played .tot file
		// and read the data file in order to get the **last** played level only (the file should only contains
		// one line, which is the last played .tot filename).
		// If the file does not exists then we open by default the first level!
		if (_vm->getGameType() == kGameTypeGob1) {
			_highestReachedLevel = Gob::Levels::Level::ONE; // Default to Level One if not found !

#if defined(_FOR_QA)
			// Do not perform a load if we are in QA mode, and if the QA asked to test a level in priority!
			if (-1 != _vm->_debug->_levelToLoad) {
				// TODO : should we fix the limit programatically instead of checking
				// the value '23' directly ?
				if (_vm->_debug->_levelToLoad == Gob::Levels::Level::UNKNOWN || 
					_vm->_debug->_levelToLoad > Gob::Levels::Level::TWENTYTHREE) 
				{
					error("cannot force a level of value '0' or greater than '23'");
				}
				skipIntro = true;
				_highestReachedLevel = (Gob::Levels::Level)_vm->_debug->_levelToLoad;
			}
#endif			

			g_system->getSavefileManager()->mountSaveData();

			// Perform a load, and check the last played level
			Common::String saveDataFilename = getSaveDataFilename(_vm->getGameType(), _vm->getFeatures(), _vm->_language);

			if (auto saveFile = std::unique_ptr<Common::InSaveFile> { g_system->getSavefileManager()->openSaveForLoading(saveDataFilename) }; saveFile != nullptr) {
				// SAVE DATA STRUCTURE
				// 
				// Line 1: The level (as an integer)
				// Line 2: The border index (as an integer), should be between 0 and the maximum number of borders set in the graphics manager
				// Line 3: The aspect ratio (as an integer), should be between 0 and UNKNOWN (but should not be UNKNOWN... need a check!!)
				// 
				// Please check in inter_v1, in the o1_loadTot function, for more information!
				skipIntro |= useSavedProgressData;

				// We must read the line even if we end up ignoring it.
				Common::String levelIdAsString = saveFile->readLine(); // The first line should tell which tot file to load

				// Gobliiins does not need an automated way to load the first slot as we load
				// directly the last played TOT script.
				// The variable `loadFirstSlotAutomatically` should always be set to NOT_PLANNED
				// playing Gobliiins.
				if (useSavedProgressData) {
					loadFirstSlotAutomatically = LoadAutoState::NOT_PLANNED;

#ifdef _FOR_QA
					if (_vm->_debug->_levelToLoad == -1)
#endif
					{
						_highestReachedLevel = Gob::Levels::Gob1::getLevelIdFromLevelName(levelIdAsString);
					}
				}

				{
					Common::String borderEnumAsString = saveFile->readLine();
					int borderIndex = atoi(borderEnumAsString.c_str());
					if (borderIndex < 0 || borderIndex > g_system->getGraphicsManager()->getNumberOfBorders()) borderIndex = 0;
					_vm->_game->desiredBorderIndex = borderIndex;
					g_system->getGraphicsManager()->switchBorder(borderIndex);
				}

				{
					Common::String desiredAspectRatioAsString = saveFile->readLine();
					int desiredAspectRatio = atoi(desiredAspectRatioAsString.c_str());
					g_system->setDesiredAspectRatio(AspectRatioEnum(desiredAspectRatio));
					_vm->_util->forceChangeAspectRatio();
				}

				if (_vm->_wipeDataAtLaunch) {
					_vm->_inter->saveGobFile("Unknown");
				}
			} else {
				_vm->_inter->saveGobFile("Unknown");
			}

			g_system->getSavefileManager()->umountSaveData();
		}
#pragma endregion Gob1 Check

#pragma region Gob2 and Gob3 check
		else if (_vm->getGameType() == kGameTypeGob2 || _vm->getGameType() == kGameTypeGob3) {
			const Gob::Levels::Level MAX_LEVEL_GAME = _vm->getGameType() == kGameTypeGob2 ? Gob::Levels::Level::EIGHT : Gob::Levels::Level::ELEVEN;
			const bool               canSkipIntro   = _vm->getGameType() != kGameTypeGob2 || !(_vm->getFeatures() & kFeaturesAdLib) || _vm->_language != Common::Language::EN_USA;

			_highestReachedLevel = Gob::Levels::Level::ONE; // Set to first level by default

#if defined(_FOR_QA)
			// Do not perform a load if we are in QA mode, and if the QA asked to test a level in priority!
			if (-1 != _vm->_debug->_levelToLoad) {
				if ((_vm->_debug->_levelToLoad == Gob::Levels::Level::UNKNOWN) ||
					_vm->_debug->_levelToLoad > MAX_LEVEL_GAME)
				{
					error("for Gobliins 2, cannot force a level of value '0' or greater than '8'");
				}
				// Skip introduction only if the game is not Gob2 Floppy USA version
				// There is a known bug for Gob2 Floppy USA where skipping the intro would break the game when clicking on the top menu
				// To know more about skipping the credits screen in Gob2 Floppy USA, please check the Inter_v1::o1_repeatUntil function in inter_v1.cpp
				skipIntro = canSkipIntro;
				_highestReachedLevel = (Gob::Levels::Level)_vm->_debug->_levelToLoad;
			}
#endif	

			g_system->getSavefileManager()->mountSaveData();

			Common::String saveDataFilename = getSaveDataFilename(_vm->getGameType(), _vm->getFeatures(), _vm->_language);

			if (auto saveFile = std::unique_ptr<Common::InSaveFile> { g_system->getSavefileManager()->openSaveForLoading(saveDataFilename) }; saveFile != nullptr) {
				// SAVE DATA STRUCTURE
				// Line 1: The level (as an integer)
				// Line 2: The border index (as an integer), should be between 0 and the maximum number of borders set in the graphics manager
				// Line 3: The aspect ratio (as an integer), should be between 0 and UNKNOWN (but should not be UNKNOWN... need a check!!)
				//
				// Please check in inter_v2, in the o2_writeData function, for more information!

				// Skip introduction only if the game is not Gob2 Floppy USA version
				// There is a known bug for Gob2 Floppy USA where skipping the intro would break the game when clicking on the top menu
				// To know more about skipping the credits screen in Gob2 Floppy USA, please check the Inter_v1::o1_repeatUntil function in inter_v1.cpp
				skipIntro = canSkipIntro && (skipIntro || useSavedProgressData);

#pragma region Auto Load First Slot
				if (useSavedProgressData) {
					// Plan to load the first slot (LOAD process)
					loadFirstSlotAutomatically = LoadAutoState::PLANNED;

					// Do not display anything while the LOAD process is being launched
					_vm->_game->pausePlayerState(true);
				}
#pragma endregion Auto Load First Slot

				// We must read the line even if we end up ignoring it.
				Common::String fileLine = saveFile->readLine();
				const int levelId = atoi(fileLine.c_str());

				if (useSavedProgressData && levelId > 0) {
#ifdef _FOR_QA
					if (_vm->_debug->_levelToLoad == -1)
#endif
					{
						_highestReachedLevel = (Gob::Levels::Level) levelId;
					}
				}

				{
					Common::String borderEnumAsString = saveFile->readLine();
					int borderIndex = atoi(borderEnumAsString.c_str());
					if (borderIndex < 0 || borderIndex > g_system->getGraphicsManager()->getNumberOfBorders()) borderIndex = 0;
					_vm->_game->desiredBorderIndex = borderIndex;
					g_system->getGraphicsManager()->switchBorder(borderIndex);
				}

				{
					Common::String desiredAspectRatioAsString = saveFile->readLine();
					int desiredAspectRatio = atoi(desiredAspectRatioAsString.c_str());
					g_system->setDesiredAspectRatio(AspectRatioEnum(desiredAspectRatio));
					_vm->_util->forceChangeAspectRatio();
				}

				if (_vm->_wipeDataAtLaunch) {
					_vm->_inter->saveGobFile("-1");
				}
			} else {
				_vm->_inter->saveGobFile("-1");
			}

			g_system->getSavefileManager()->umountSaveData();

			_vm->_wipeDataAtLaunch = false;
		}
#pragma endregion Gob2 And Gob3 Check
	}

	void Game::playTot(int16 function) {
		int16* oldNestLevel = _vm->_inter->_nestLevel;
		int16* oldBreakFrom = _vm->_inter->_breakFromLevel;
		int16* oldCaptureCounter = _vm->_scenery->_pCaptureCounter;

		_script->push();

		int16 captureCounter = 0;
		int16 breakFrom;
		int16 nestLevel;
		_vm->_inter->_nestLevel = &nestLevel;
		_vm->_inter->_breakFromLevel = &breakFrom;
		_vm->_scenery->_pCaptureCounter = &captureCounter;

		_vm->_assignSpeakerOnHookPosition.reset();

		if ((_vm->getFeatures() & kFeaturesCD) == 0 && _vm->getGameType() == kGameTypeGob2) {
			const std::optional<uint8_t> level = Inter_v1::tryExtractGob2LevelNumber(
				_curTotFile.matchString("menu.tot", true)
				? _vm->_game->_menuLoadedFromTot
				: _curTotFile);

			if (level.has_value()) {
				static int32_t hookPositions[] = {
					15'757, /* GOB00 */ 16'026, /* GOB01 */ 17'578, /* GOB02 */ 17'457, /* GOB03 */ 19'865, /* GOB04 */
					18'342, /* GOB05 */ 20'565, /* GOB06 */ 17'997, /* GOB07 */ 18'476, /* GOB08 */ 15'968, /* GOB09 */
					17'194, /* GOB10 */ 18'949, /* GOB11 */ 17'207, /* GOB12 */ 19'391, /* GOB13 */ 19'130, /* GOB14 */
					20'261, /* GOB15 */ 18'567, /* GOB16 */ 19'602, /* GOB17 */ 19'711, /* GOB18 */ 20'104, /* GOB19 */
					21'541, /* GOB20 */ 16'708, /* GOB21 */ 16'509, /* GOB22 */ };

				_vm->_assignSpeakerOnHookPosition = hookPositions[*level];
			}
		}

		if (function <= 0) {
			while (!_vm->shouldQuit()) {
				if (_vm->_inter->_variables)
					_vm->_draw->animateCursor(4);

				if (function != -1) {
					_vm->_inter->initControlVars(1);

					for (int i = 0; i < 4; i++) {
						_vm->_draw->_fontToSprite[i].sprite = -1;
						_vm->_draw->_fontToSprite[i].base = -1;
						_vm->_draw->_fontToSprite[i].width = -1;
						_vm->_draw->_fontToSprite[i].height = -1;
					}

					// Gobliiins music stopping
					if (_vm->getGameType() == kGameTypeGob1) {
						_vm->_sound->adlibStop();
						_vm->_sound->cdStop();
					}

					_vm->_mult->initAll();
					_vm->_mult->zeroMultData();

					_vm->_draw->_spritesArray[Draw::kFrontSurface] = _vm->_draw->_frontSurface;
					_vm->_draw->_spritesArray[Draw::kBackSurface] = _vm->_draw->_backSurface;
					_vm->_draw->_cursorSpritesBack = _vm->_draw->_cursorSprites;
				}
				else
					_vm->_inter->initControlVars(0);

				_vm->_draw->_cursorHotspotXVar = -1;
				_totToLoad.clear();

				if ((_curTotFile.empty()) && (!_script->isLoaded()))
					break;

				if (function == -2) {
					_vm->_vidPlayer->closeVideo();
					function = 0;
				}

				if (!_script->load(_curTotFile)) {
					_vm->_draw->blitCursor();
					_vm->_inter->_terminate = 2;
					break;
				}

				if (_vm->getGameType() == kGameTypeAdibou1 ||
					_vm->getGameType() == kGameTypeAdi2) { // TODO: maybe needed by other games
					_vm->_draw->_needAdjust = _vm->_game->_script->getData()[58];
					if (_vm->_draw->_needAdjust <= 1 || _vm->_draw->_needAdjust >= 8)
						_vm->_draw->_needAdjust = 2;
				}

				_resources->load(_curTotFile);

				_vm->_global->_inter_animDataSize = _script->getAnimDataSize();
				if (!_vm->_inter->_variables)
					_vm->_inter->allocateVars(_script->getVariablesCount() & 0xFFFF);

				_script->seek(_script->getFunctionOffset(TOTFile::kFunctionStart));

				_vm->_inter->renewTimeInVars();

				if (_vm->_inter->_variables) {
					WRITE_VAR(13, _vm->_global->_useMouse);
					WRITE_VAR(14, _vm->_global->_soundFlags);
					WRITE_VAR(15, _vm->_global->_fakeVideoMode);

					if (_vm->getGameType() == kGameTypeGeisha)
						WRITE_VAR(57, _vm->_global->_language);
					else
						WRITE_VAR(16, _vm->_global->_language);

					// WORKAROUND: Inca2 seems to depend on that variable to be cleared
					if (_vm->getGameType() == kGameTypeInca2)
						WRITE_VAR(59, 0);
				}

				_vm->_inter->callSub(2);

				if (!_totToLoad.empty())
					_vm->_inter->_terminate = 0;

				_vm->_draw->blitInvalidated();

				_script->unload();

				_resources->unload();

				for (int i = 0; i < *_vm->_scenery->_pCaptureCounter; i++)
					capturePop(0);

				if (function != -1) {
					_vm->_goblin->freeObjects();

					_vm->_sound->blasterStop(0);

					for (int i = 0; i < Sound::kSoundsCount; i++) {
						SoundDesc* sound = _vm->_sound->sampleGetBySlot(i);

						if (sound &&
							((sound->getType() == SOUND_SND) || (sound->getType() == SOUND_WAV)))
							_vm->_sound->sampleFree(sound);
					}
				}

				_vm->_draw->closeAllWin();

				if (_totToLoad.empty())
					break;

				_curTotFile = _totToLoad;

			}
		}
		else {
			_vm->_inter->initControlVars(0);
			_vm->_scenery->_pCaptureCounter = oldCaptureCounter;

			if (function > 13)
				_script->seek(function);
			else
				_script->seek(_script->getFunctionOffset(function + 1));

			_vm->_inter->callSub(2);

			if (_vm->_inter->_terminate != 0)
				_vm->_inter->_terminate = 2;
		}

		_vm->_inter->_nestLevel = oldNestLevel;
		_vm->_inter->_breakFromLevel = oldBreakFrom;
		_vm->_scenery->_pCaptureCounter = oldCaptureCounter;

		_script->pop();
	}

	void Game::capturePush(int16 left, int16 top, int16 width, int16 height) {
		int16 right;

		if (_captureCount == 20)
			error("Game::capturePush(): Capture stack overflow");

		_vm->_draw->adjustCoords(0, &left, &top);
		_vm->_draw->adjustCoords(0, &width, &height);

		_captureStack[_captureCount].left = left;
		_captureStack[_captureCount].top = top;
		_captureStack[_captureCount].right = left + width;
		_captureStack[_captureCount].bottom = top + height;

		_vm->_draw->_spriteTop = top;
		_vm->_draw->_spriteBottom = height;

		right = left + width - 1;
		left &= 0xFFF0;
		right |= 0xF;

		_vm->_draw->initSpriteSurf(Draw::kCaptureSurface + _captureCount, right - left + 1, height, 0);

		_vm->_draw->_sourceSurface = Draw::kBackSurface;
		_vm->_draw->_destSurface = Draw::kCaptureSurface + _captureCount;

		_vm->_draw->_spriteLeft = left;
		_vm->_draw->_spriteRight = right - left + 1;
		_vm->_draw->_destSpriteX = 0;
		_vm->_draw->_destSpriteY = 0;
		_vm->_draw->_transparency = 0;

		int16 savedNeedAdjust = _vm->_draw->_needAdjust;
		_vm->_draw->_needAdjust = 10;
		_vm->_draw->spriteOperation(DRAW_BLITSURF);
		_vm->_draw->_needAdjust = savedNeedAdjust;
		_captureCount++;
	}

	void Game::capturePop(char doDraw) {
		if (_captureCount <= 0)
			return;

		_captureCount--;
		if (doDraw) {
			_vm->_draw->_destSpriteX = _captureStack[_captureCount].left;
			_vm->_draw->_destSpriteY = _captureStack[_captureCount].top;
			_vm->_draw->_spriteRight =
				_captureStack[_captureCount].width();
			_vm->_draw->_spriteBottom =
				_captureStack[_captureCount].height();

			_vm->_draw->_transparency = 0;
			_vm->_draw->_sourceSurface = Draw::kCaptureSurface + _captureCount;
			_vm->_draw->_destSurface = Draw::kBackSurface;
			_vm->_draw->_spriteLeft = _vm->_draw->_destSpriteX & 0xF;
			_vm->_draw->_spriteTop = 0;
			int16 savedNeedAdjust = _vm->_draw->_needAdjust;
			_vm->_draw->_needAdjust = 10;
			_vm->_draw->spriteOperation(DRAW_BLITSURF);
			_vm->_draw->_needAdjust = savedNeedAdjust;
		}
		_vm->_draw->freeSprite(Draw::kCaptureSurface + _captureCount);
	}

	void Game::freeSoundSlot(int16 slot) {
		if (slot == -1)
			slot = _vm->_game->_script->readValExpr();

		_vm->_sound->sampleFree(_vm->_sound->sampleGetBySlot(slot));
	}

	void Game::wantScroll(int16 x, int16 y) {
		_wantScroll = true;
		_wantScrollX = x;
		_wantScrollY = y;
	}

	void Game::evaluateScroll() {
		if (_noScroll || _preventScroll || !_wantScroll)
			return;

		if ((_vm->_global->_videoMode != 0x14) && (_vm->_global->_videoMode != 0x18))
			return;

		if ((_wantScrollX == 0) && (_vm->_draw->_scrollOffsetX > 0)) {
			uint16 off;

			off = MIN(_vm->_draw->_cursorWidth, _vm->_draw->_scrollOffsetX);
			off = MAX(off / 2, 1);
			_vm->_draw->_scrollOffsetX -= off;
			_vm->_video->dirtyRectsAll();
		}
		else if ((_wantScrollY == 0) && (_vm->_draw->_scrollOffsetY > 0)) {
			uint16 off;

			off = MIN(_vm->_draw->_cursorHeight, _vm->_draw->_scrollOffsetY);
			off = MAX(off / 2, 1);
			_vm->_draw->_scrollOffsetY -= off;
			_vm->_video->dirtyRectsAll();
		}

		int16 cursorRight = _wantScrollX + _vm->_draw->_cursorWidth;
		int16 screenRight = _vm->_draw->_scrollOffsetX + _vm->_width;
		int16 cursorBottom = _wantScrollY + _vm->_draw->_cursorHeight;
		int16 screenBottom = _vm->_draw->_scrollOffsetY + _vm->_height;

		if ((cursorRight >= _vm->_width) &&
			(screenRight < _vm->_video->_surfWidth)) {
			uint16 off;

			off = MIN(_vm->_draw->_cursorWidth,
				(int16)(_vm->_video->_surfWidth - screenRight));
			off = MAX(off / 2, 1);

			_vm->_draw->_scrollOffsetX += off;
			_vm->_video->dirtyRectsAll();

			_vm->_util->setMousePos(_vm->_width - _vm->_draw->_cursorWidth, _wantScrollY);
		}
		else if ((cursorBottom >= (_vm->_height - _vm->_video->_splitHeight2)) &&
			(screenBottom < _vm->_video->_surfHeight)) {
			uint16 off;

			off = MIN(_vm->_draw->_cursorHeight,
				(int16)(_vm->_video->_surfHeight - screenBottom));
			off = MAX(off / 2, 1);

			_vm->_draw->_scrollOffsetY += off;
			_vm->_video->dirtyRectsAll();

			_vm->_util->setMousePos(_wantScrollX,
				_vm->_height - _vm->_video->_splitHeight2 - _vm->_draw->_cursorHeight);
		}

		_vm->_util->setScrollOffset();

		_wantScroll = false;
	}

	int16 Game::checkKeys(int16* pMouseX, int16* pMouseY,
		MouseButtons* pButtons, char handleMouse) {

		_vm->_util->processInput(true);

		if (_vm->_mult->_multData && _vm->_inter->_variables &&
			(VAR(58) != 0)) {
			if (_vm->_mult->_multData->frameStart != (int)VAR(58) - 1)
				_vm->_mult->_multData->frameStart++;
			else
				_vm->_mult->_multData->frameStart = 0;

			_vm->_mult->playMult(_vm->_mult->_multData->frameStart + VAR(57),
				_vm->_mult->_multData->frameStart + VAR(57), 1, handleMouse);
		}

		if ((_vm->_inter->_soundEndTimeKey != 0) &&
			(_vm->_util->getTimeKey() >= _vm->_inter->_soundEndTimeKey)) {
			_vm->_sound->blasterStop(_vm->_inter->_soundStopVal);
			_vm->_inter->_soundEndTimeKey = 0;
		}

		if (pMouseX && pMouseY && pButtons) {
			_vm->_util->getMouseState(pMouseX, pMouseY, pButtons);

			if (*pButtons == kMouseButtonsBoth)
				*pButtons = kMouseButtonsNone;
		}

		return _vm->_util->checkKey();
	}

	void Game::start() {
		prepareStart();
		playTot(-2);

		_vm->_draw->closeScreen();

		for (int i = 0; i < Draw::kSpriteCount; i++)
			_vm->_draw->freeSprite(i);
		_vm->_draw->_scummvmCursor.reset();
	}

	// flagbits: 0 = freeInterVariables, 1 = function -1
	void Game::totSub(int8 flags, const Common::String& totFile) {
		int8 curBackupPos;

		if ((flags == 16) || (flags == 17)) {
			// Prefetch tot data + delete prefetched data
			return;
		}

		if (_numEnvironments >= Environments::kEnvironmentCount)
			error("Game::totSub(): Environments overflow");

		_environments.set(_numEnvironments);

		if (flags == 18) {
			warning("Backuping media to %d", _numEnvironments);
			_environments.setMedia(_numEnvironments);
		}

		curBackupPos = _curEnvironment;
		_numEnvironments++;
		_curEnvironment = _numEnvironments;

		_script = new Script(_vm);
		_resources = new Resources(_vm);

		if (flags & 0x80)
			warning("Addy Stub: Game::totSub(), flags & 0x80");

		if (flags & 5)
			_vm->_inter->_variables = nullptr;

		_curTotFile = totFile + ".TOT";

		if (_vm->_inter->_terminate != 0) {
			clearUnusedEnvironment();
			return;
		}

		if (!(flags & 0x20))
			_hotspots->push(0, true);

		if ((flags == 18) || (flags & 0x06))
			playTot(-1);
		else
			playTot(0);

		if (_vm->_inter->_terminate != 2)
			_vm->_inter->_terminate = 0;

		if (!(flags & 0x20)) {
			_hotspots->clear();
			_hotspots->pop();
			// Force to clear the cheats list when transitioning from one level to another
			if (g_system->getGraphicsManager()) g_system->getGraphicsManager()->clearHotspotCheatsList();
		}

		if ((flags & 5) && _vm->_inter->_variables)
			_vm->_inter->delocateVars();

		clearUnusedEnvironment();

		_numEnvironments--;
		_curEnvironment = curBackupPos;
		_environments.get(_numEnvironments);

		if (flags == 18) {
			warning("Restoring media from %d", _numEnvironments);
			_environments.getMedia(_numEnvironments);
		}

		_vm->_global->_inter_animDataSize = _script->getAnimDataSize();
	}

	void Game::switchTotSub(int16 index, int16 function) {
		int16 backupedCount;
		int16 curBackupPos;

		if ((_numEnvironments - index) < 1)
			return;

		int16 newPos = _curEnvironment - index - ((index >= 0) ? 1 : 0);
		if (newPos >= Environments::kEnvironmentCount)
			return;

		// WORKAROUND: Some versions don't make the MOVEMENT menu item unselectable
		// in the dreamland screen, resulting in a crash when it's clicked.
		if ((_vm->getGameType() == kGameTypeGob2) && (index == -1) && (function == 7) &&
			_environments.getTotFile(newPos).equalsIgnoreCase("gob06.tot"))
			return;

		curBackupPos = _curEnvironment;
		backupedCount = _numEnvironments;
		if (_curEnvironment == _numEnvironments)
			_environments.set(_numEnvironments++);

		_curEnvironment -= index;
		if (index >= 0)
			_curEnvironment--;

		clearUnusedEnvironment();

		// Part of fixing the issue #24162
		// If the new environment is MENU.TOT then save the current (future parent) script name
		// Also useful for the footsteps hook which needs to know the actual tot file when the
		// menu is opened.
		if (Gob::GameType::kGameTypeGob2 == _vm->getGameType()) {
			_environments.checkIfInMenu(_curEnvironment);
		}

		_environments.get(_curEnvironment);

		if (_vm->_inter->_terminate != 0) {
			clearUnusedEnvironment();
			return;
		}

		_hotspots->push(0, true);
		playTot(function);

		if (_vm->_inter->_terminate != 2)
			_vm->_inter->_terminate = 0;

		// Goblins 3 need specific timeset to redraw, in contrary of Gobliins 2 where everything
		// is done in o1_drawOperations
		if (Gob::GameType::kGameTypeGob3 == _vm->getGameType() && _vm->_game->isPlayerStatePaused() && (Gob::Game::LoadAutoState::NOT_PLANNED == _vm->_game->loadFirstSlotAutomatically)) {
			_vm->_game->pausePlayerState(false);
			// Force purge all mouse events, to avoid very weird conflicts
			// if the user is interacting with the controllers while saving the game
			g_system->getEventManager()->purgeMouseEvents();
		}

		_hotspots->pop();

		clearUnusedEnvironment();
		// Force to clear the cheats list when transitioning from one level to another
		if (g_system->getGraphicsManager()) g_system->getGraphicsManager()->clearHotspotCheatsList();



		_curEnvironment = curBackupPos;
		_numEnvironments = backupedCount;
		_environments.get(_curEnvironment);
	}

	void Game::deletedVars(Variables* variables) {
		_environments.deleted(variables);
	}

	void Game::clearUnusedEnvironment() {
		if (!_environments.has(_script)) {
			delete _script;
			_script = nullptr;
		}
		if (!_environments.has(_resources)) {
			delete _resources;
			_resources = nullptr;
		}
	}

	bool Game::loadFunctions(const Common::String& tot, uint16 flags) {
		if ((flags & 0xFFFE) != 0) {
			warning("Game::loadFunctions(): Unknown flags 0x%04X", flags);
			return false;
		}

		bool unload = (flags & 0x1) != 0;

		if (unload) {
			debug(SoftLevel::kInterpreter, "Unloading function for \"%s\"", tot.c_str());
			return _totFunctions.unload(tot);
		}

		debug(SoftLevel::kInterpreter, "Loading function for \"%s\"", tot.c_str());
		return _totFunctions.load(tot);
	}

	bool Game::callFunction(const Common::String& tot, const Common::String& function,
		int16 param) {

		if (param != 0) {
			warning("Game::callFunction(): param != 0 (%d)", param);
			return false;
		}

		debug(SoftLevel::kInterpreter, "Calling function \"%s\":\"%s\"",
			tot.c_str(), function.c_str());

		uint16 offset = atoi(function.c_str());
		if (offset != 0)
			return _totFunctions.call(tot, offset);

		if (function.size() > 16)
			return _totFunctions.call(tot, Common::String(function.c_str(), 16));

		return _totFunctions.call(tot, function);
	}

	void Game::pausePlayerState(const bool pause) {
		debug(kGame, "The player is set to '%s'", pause ? (char*)"PAUSE" : (char*)"UNPAUSE");
		// Pause / Continue the interpreter rendering
		_vm->_game->_waitForRetrace = pause;
		// Pause / Continue the audio
		pause ? _vm->_mixer->mute() : _vm->_mixer->unmute();
		// Pause / Continue the SDL Joystick (only) inputs
		SDL_JoystickEventState(pause ? SDL_DISABLE : SDL_ENABLE);
	}

} // End of namespace Gob
