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

#ifndef GOB_GAME_H
#define GOB_GAME_H

#include "common/str.h"

#include "gob/levels.h"
#include "gob/util.h"
#include "gob/video.h"
#include "gob/sound/sounddesc.h"
#include "gob/hotspots.h"

namespace Gob {

	class Script;
	class Resources;
	class Variables;
	class Hotspots;

	class Environments {
	public:
		static const uint8 kEnvironmentCount = 20;

		Environments(GobEngine* vm);
		~Environments();

		void set(uint8 env);
		void get(uint8 env) const;
		void checkIfInMenu(uint8 env) const;

		const Common::String& getTotFile(uint8 env) const;

		bool has(Variables* variables, uint8 startEnv = 0, int16 except = -1) const;
		bool has(Script* script, uint8 startEnv = 0, int16 except = -1) const;
		bool has(Resources* resources, uint8 startEnv = 0, int16 except = -1) const;

		void deleted(Variables* variables);

		void clear();

		bool setMedia(uint8 env);
		bool getMedia(uint8 env);
		bool clearMedia(uint8 env);

	private:
		struct Environment {
			int32          cursorHotspotX;
			int32          cursorHotspotY;
			Common::String totFile;
			Variables* variables;
			Script* script;
			Resources* resources;
		};

		struct Media {
			SurfacePtr sprites[10];
			SoundDesc  sounds[10];
			Font* fonts[17];
		};

		GobEngine* _vm;

		Environment _environments[kEnvironmentCount];
		Media       _media[kEnvironmentCount];
	};

	class TotFunctions {
	public:
		TotFunctions(GobEngine* vm);
		~TotFunctions();

		int find(const Common::String& totFile) const;

		bool load(const Common::String& totFile);
		bool unload(const Common::String& totFile);

		bool call(const Common::String& totFile, const Common::String& function) const;
		bool call(const Common::String& totFile, uint16 offset) const;

	private:
		static const uint8 kTotCount = 100;

		struct Function {
			Common::String name;
			byte type;
			uint16 offset;
		};

		struct Tot {
			Common::String file;

			Common::List<Function> functions;

			Script* script;
			Resources* resources;
		};

		GobEngine* _vm;

		Tot _tots[kTotCount];

		bool loadTot(Tot& tot, const Common::String& file);
		void freeTot(Tot& tot);

		bool loadIDE(Tot& tot);

		int findFree() const;

		bool call(const Tot& tot, uint16 offset) const;
	};

	class Game {
	public:
		/// <summary>
		/// States used trying to load the first slot at startup.
		/// Only used by Gobliins 2 and Goblins 3 **if** a save is detected at launch.
		/// The first Gobliiins game does not perform like this as we automatically load the last TOT file 
		/// depending on the last level / password found.
		/// </summary>
		enum class LoadAutoState {
			NOT_PLANNED,                     // Do not perform the auto load (stop there)
			PLANNED,                         // Planned, but not started yet...
			WAITING_FOR_MOVING_ON_MENU,      // Moving the cursor on top of the screen, to trigger the context menu
			DID_MOVE_ON_MENU,                // Context menu has been triggered, waiting for the flag to click
			WAITING_FOR_CLICK_ON_MENU,       // Click on the Save/Load button, of the Context menu
			DID_CLICK_ON_MENU,               // Click has been performed, waiting for confirmation
			WILL_MOVE_ON_LOAD,               // Confirmed, we can move the cursor to the "LOAD" button
			WAITING_FOR_SELECTING_LOADING,   // Waiting for move confirmation (position)
			WAITING_FOR_CLICK_ON_LOADING,    // Waiting for the second confirmation (hovered)
			DO_CLICK_ON_LOADING,             // Can now click on the "LOAD" button
			DID_CLICK_ON_LOAD,               // Click has been performed, waiting for confirmation
			WAITING_FOR_CLICK_ON_FIRST_SLOT, // Confirmation, perform a second click to confirm the slot
			DID_CLICK_ON_FIRST_SLOT,         // Done, waiting for confirmation...
			DONE,                            // Confirmed, can now stop there
		};
		Script* _script;
		Resources* _resources;
		Hotspots* _hotspots;
		ManagedCheats* _managedCheats;

		// Current TOT script
		Common::String _curTotFile     = Common::String("");
		// TOT script to load
		Common::String _totToLoad      = Common::String("");
		// The TOT script that loaded MENU.TOT
		Common::String _menuLoadedFromTot = "";

		int32 _startTimeKey;
		MouseButtons _mouseButtons;

		bool _noScroll;
		bool _preventScroll;

		bool  _wantScroll;
		int16 _wantScrollX;
		int16 _wantScrollY;

		byte _handleMouse;
		char _forceHandleMouse;
		/*
		BEGIN RAG HACKS
		*/ 
		// Gobliiins + Gobliins 2 + Goblins 3
		bool skipIntro = false;                  // for Gobliiins, Gobliins 2 & Goblins 3 -> do not show the introduction & credits screen starting each game
		Gob::Levels::Level _highestReachedLevel = Gob::Levels::UNKNOWN;
		// Gobliiins only
		bool shouldFirePasswordInputs = false;   // Only for Gobliiins (1) -> type automatically the password
		bool _gob1_simulateSaveLoad = true;       // There's no save in Gob1, we just reload the last played level.
		// Gobliins 2 + Goblins 3
		bool firstStepForAutoSave = false;       // Only for Gobliins 2 & Goblins 3 -> does not list the slots of the game **only when saving**
		bool secondStepForAutoSave = false;      // Only for Gobliins 2 & Goblins 3 -> automatically accept the message **when saving**
		LoadAutoState loadFirstSlotAutomatically = LoadAutoState::NOT_PLANNED; // Only for Gobliins 2 & Goblins 3 -> fire the "load first slot" event once intro is skipped
		// For user's configuration
		int desiredBorderIndex = 0;

		// Let the interpreter do its work, but disable some inputs / outputs **for the player**.
		// The inputs / outputs can be: the display, the audio, and the joysticks.
		void pausePlayerState(const bool pause);
		inline bool isPlayerStatePaused() const { return _waitForRetrace; }
		/*
		END RAG HACKS
		*/

		Game(GobEngine* vm, bool skipIntro = false);
		virtual ~Game();

		void prepareStart();

		void playTot(int16 function);

		void capturePush(int16 left, int16 top, int16 width, int16 height);
		void capturePop(char doDraw);

		void freeSoundSlot(int16 slot);

		void wantScroll(int16 x, int16 y);
		void evaluateScroll();

		int16 checkKeys(int16* pMousex = 0, int16* pMouseY = 0,
			MouseButtons* pButtons = 0, char handleMouse = 0);
		void start();

		void totSub(int8 flags, const Common::String& totFile);
		void switchTotSub(int16 index, int16 function);

		void deletedVars(Variables* variables);

		bool loadFunctions(const Common::String& tot, uint16 flags);
		bool callFunction(const Common::String& tot, const Common::String& function, int16 param);

	protected:
		GobEngine* _vm;

		char _tempStr[256];

		// Capture
		Common::Rectangle _captureStack[20];
		int16 _captureCount;

		// For totSub()
		int8 _curEnvironment;
		int8 _numEnvironments;
		Environments _environments;

		TotFunctions _totFunctions;

		/*
		BEGIN RAG HACKS
		*/
		// Only for Gobliins 2 & Goblins 3 -> do not redraw the screen while the user is saving (does not work while loading data)
		bool _waitForRetrace = false;
		/*
		END RAG HACKS
		*/

		void clearUnusedEnvironment();
	};

} // End of namespace Gob

#endif // GOB_GAME_H
