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

#include "common/stream.h"
#include "common/graphics.h"

#include "graphics/palette.h"

#include "gob/gob.h"
#include "gob/goblin.h" // To switch goblin, for Gob2 and Gob3
#include "gob/util.h"
#include "gob/global.h"
#include "gob/dataio.h"
#include "gob/draw.h"
#include "gob/game.h"
#include "gob/video.h"
#include "gob/videoplayer.h"
#include "gob/sound/sound.h"

namespace Gob {

	Util::Util(GobEngine* vm) : _vm(vm) {
		_mouseButtons = kMouseButtonsNone;
		_keyBufferHead = 0;
		_keyBufferTail = 0;
		_fastMode = 0;
		_frameRate = 12;
		_frameWaitTime = 0;
		_startFrameTime = 0;

		_keyState = 0;
	}

	uint32 Util::getTimeKey() {
		return g_system->getMillis() * _vm->_global->_speedFactor;
	}

	int16 Util::getRandom(int16 max) {
		if (max == 0)
			return 0;

		return _vm->_rnd.getRandomNumber(max - 1);
	}

	void Util::beep(int16 freq) {
		if (_vm->_global->_soundFlags == 0)
			return;

		_vm->_sound->speakerOn(freq, 50);
	}

	void Util::notifyPaused(uint32 duration) {
		_startFrameTime += duration;
	}

	void Util::delay(uint16 msecs) {
		g_system->delayMillis(msecs / _vm->_global->_speedFactor);
	}

	void Util::longDelay(uint16 msecs) {
		uint32 time = g_system->getMillis() * _vm->_global->_speedFactor + msecs;
		do {
			_vm->_video->waitRetrace();
			processInput();
			delay(1);
		} while (!_vm->shouldQuit() &&
			((g_system->getMillis() * _vm->_global->_speedFactor) < time));
	}

	void Util::initInput() {
		_mouseButtons = kMouseButtonsNone;
		_keyBufferHead = _keyBufferTail = 0;
	}

	void Util::forceChangeAspectRatio() {
		auto* graphicsManager = g_system->getGraphicsManager();

		g_system->setStretchMode(graphicsManager->getDesiredAspectRatioEnum() == AspectRatioEnum::PixelPerfect ? STRETCH_PIXEL_PERFECT : STRETCH_FIT);

		// Needed because of dynamic rendering
		// applyGfx destroys everything, including original renderer object...
		// if applyGfx is called then we need to recreate everything from scratch, including our imgui instance!
		// **applyGfx** should not be called, handleResizeImpl is more than enough for our needs
		int height = graphicsManager->getWindowHeight();
		int width = graphicsManager->getWindowWidth();
		graphicsManager->handleResizeImpl(width, height);

		g_system->fillScreen(0);
		_vm->_video->dirtyRectsAll(); // Force the vm to redraw everything!
	}

	/// <summary>
	/// Switch to the next goblin if all conditions are met:
	/// 1. the next goblin is not busy,
	/// 2. the animation of the next goblin is set,
	/// 3. the index points to a valid array index.
	/// The switch is done left clicking on an object (inventory or goblin directly), and
	/// the mouse release event is only transmitted if the left click has been performaed.
	/// So, if the requirements are not met the left click event is not transmitted, and
	/// the mouse release event should be dropped.
	/// </summary>
	/// <param name="event">The full button event, to know if the user pressed or released the button</param>
	void Util::switchGoblin(Common::Event& event) {
		/// <summary>
		/// Only used in Gob2 and Gob3, in order to know which goblin is used
		/// </summary>
		static int _currentGoblin = 0;
		static int _nbGoblinsPerGame[3] = { 3, 3, 2 };
		/// <summary>
		/// The last position of the cursor after clicking on object / goblin, to switch
		/// to another character
		/// </summary>
		static Common::Point _lastKnownPosition = Common::Point(0, 0);
		/// <summary>
		/// Save if a left click has been "down" when firing the "up" event
		/// </summary>
		/// <param name="event"></param>
		static bool _hasClicked = false;

		Common::Event clickEvent;

		// If the event is only the "up" button, then go back to initial position
		// and launch a "mouse release" event
		// This is common for Gob1, Gob2 and Gob3
		if (event.type == Common::EventType::EVENT_JOYBUTTON_UP) {
			if (!_hasClicked) return; // If the up does not follow a "down" event, then return without notifying the system
			clickEvent.type = Common::EventType::EVENT_LBUTTONUP;
			clickEvent.mouse = _lastKnownPosition;
			_lastKnownPosition = Common::Point(0, 0);
			_hasClicked = false;
			g_system->getEventManager()->notifyEvent(clickEvent);
			return;
		}

		// Prepare for left clicking (down)
		clickEvent.type = Common::EventType::EVENT_LBUTTONDOWN;
		int16 x = 0; int16 y = 0;
		_vm->_util->getMouseState(&x, &y, NULL);
		// Save the current position, to go back when "up" left click event has been fired
		_lastKnownPosition = Common::Point(x, y);

		if (Gob::GameType::kGameTypeGob1 == _vm->getGameType()) {
			// The switch for Gob1 is trivial: there is a medaillon at position 167,185 **in game**
			// and you can click on it to switch to another gob
			// No more than that!
			_hasClicked = true; // Always true for Gob1
			constexpr Common::Point medaillonPositionGobliins1 = Common::Point(167, 185);
			clickEvent.mouse = medaillonPositionGobliins1;
		}
		else if (Gob::GameType::kGameTypeGob2 == _vm->getGameType()) {

			// This can happen if the player clicks on switch the gob during level loading (or cinematic)
			if (NULL == _vm->_mult->_objects) {
				// In this case, do not simulate the click!
				_hasClicked = false;
				_lastKnownPosition = Common::Point(0, 0);
				return;
			}

			// First, we have to find the available / playable characters
			// Once we have this, we can switch to another character clicking on it
			// There is no trap with scrolling here!
			int newGoblin = (++_currentGoblin) % _nbGoblinsPerGame[1];

			// Exception case: the prince buffoon !
			// The prince is a third character that is not playable everywhere (only one level)
			// The way to check we can play it is to take a look at its state (because the object will exists in
			// the array of objects in any case)
			if (newGoblin == 2) {
				Mult::Mult_Object* princeObj = &_vm->_mult->_objects[newGoblin];
				Mult::Mult_AnimData* princeAnim = princeObj->pAnimData;
				// Disable the Prince Buffoon character for the entire Gob21 script
				if (_vm->isCurrentTot("GOB21.TOT")) { newGoblin = 0; }
				// The default behaviour
				else {
					// DO NOT CHECK the 'isBusy' flag !
					// isBusy is used when the goblin is not **selected** (!!!) and does not have anything to do when the goblin canno't be played
					// This means that switching characters at the end of Gob18 script will move characters, and not "disable" the checks...
					// See issue #23206
					if ((princeAnim->state <= 0) || (princeAnim->nextState <= 0) || (princeAnim->isPaused != 0)) { newGoblin = 0; }
				}
			}

			Common::Point goblinPosition = _vm->_goblin->getGoblinPosition(newGoblin);
			if (goblinPosition.x <= 0 && goblinPosition.y <= 0) {
				_hasClicked = false;
				_lastKnownPosition = Common::Point(0, 0);
				return; // Do not simulate the click!
			}
			_hasClicked = true;
			_currentGoblin = newGoblin;
			clickEvent.mouse = goblinPosition;
		}
		else if (Gob::GameType::kGameTypeGob3 == _vm->getGameType()) {
			// First, we have to find the available / playable characters
			// Once we have this, we can switch to another character clicking on it
			// Be careful because there can a trap with scrolling here!
			int newGoblin = (++_currentGoblin) % _nbGoblinsPerGame[2];
			Common::Point goblinPosition = _vm->_goblin->getGoblinPosition(newGoblin);
			// Remove the scrolling position horizontally, because they will be added again 
			// when computing in the raw map ("map world", not "screen world")
			goblinPosition.x -= _vm->_video->_scrollOffsetX;
			if (goblinPosition.x <= 0 && goblinPosition.y <= 0) {
				_hasClicked = false;
				_lastKnownPosition = Common::Point(0, 0);
				return; // Do not simulate the click!
			}
			_hasClicked = true;
			_currentGoblin = newGoblin;
			clickEvent.mouse = goblinPosition;
		}

		// Notify the system of the left click
		g_system->getEventManager()->notifyEvent(clickEvent);
	}

	bool Util::preprocessJoystickButtonEvent(Common::Event& event) {
		bool ans = false;
		if (event.type != Common::EventType::EVENT_JOYBUTTON_UP && event.type != Common::EventType::EVENT_JOYBUTTON_DOWN) return ans;
		switch (event.joystick.button) {
		case Common::JoystickButton::JOYSTICK_BUTTON_A: // Default for left click (handle mouse)
		case Common::JoystickButton::JOYSTICK_BUTTON_B: // Default for right click (inventory)
		{
			Common::Event clickEvent;
			if (event.joystick.button == Common::JoystickButton::JOYSTICK_BUTTON_A) clickEvent.type = (event.type == Common::EventType::EVENT_JOYBUTTON_DOWN) ? Common::EventType::EVENT_LBUTTONDOWN : Common::EventType::EVENT_LBUTTONUP;
			else																	clickEvent.type = (event.type == Common::EventType::EVENT_JOYBUTTON_DOWN) ? Common::EventType::EVENT_RBUTTONDOWN : Common::EventType::EVENT_RBUTTONUP;
			int16 x = 0; int16 y = 0;
			_vm->_util->getMouseState(&x, &y, NULL);
			// Substracting the scroll offset values is needed for Goblins 3, as the game scrolls horizontally
			// This has no real behaviour for Gob1 and Gob2
			clickEvent.mouse = Common::Point((x - _vm->_video->_scrollOffsetX), (y - _vm->_video->_scrollOffsetY));
			g_system->getEventManager()->notifyEvent(clickEvent);
		}
		break;
		case Common::JoystickButton::JOYSTICK_BUTTON_DPAD_UP:
		case Common::JoystickButton::JOYSTICK_BUTTON_DPAD_DOWN:
		{
			bool isDpadUp = Common::JoystickButton::JOYSTICK_BUTTON_DPAD_UP == event.joystick.button;
			if (event.type == Common::EventType::EVENT_JOYBUTTON_DOWN) {
				auto* graphicsManager = g_system->getGraphicsManager();
				switch (graphicsManager->getDesiredAspectRatioEnum()) {
				case AspectRatioEnum::FourByThree:   graphicsManager->setDesiredAspectRatio(isDpadUp ? AspectRatio(16, 10) : AspectRatio(1, 1));   break;
				case AspectRatioEnum::SixteenByTen:  graphicsManager->setDesiredAspectRatio(isDpadUp ? AspectRatio(16, 9)  : AspectRatio(4, 3));   break;
				case AspectRatioEnum::SixteenByNine: graphicsManager->setDesiredAspectRatio(isDpadUp ? AspectRatio(1, 1)   : AspectRatio(16, 10)); break;
				case AspectRatioEnum::PixelPerfect:  graphicsManager->setDesiredAspectRatio(isDpadUp ? AspectRatio(4, 3)   : AspectRatio(16, 9));  break;
				default: warning("Unknown aspect ratio set! Should not happen!"); break;
				}
				forceChangeAspectRatio();
			}
		}
		break;
		case Common::JoystickButton::JOYSTICK_BUTTON_DPAD_LEFT:
		{
			if (event.type == Common::EventType::EVENT_JOYBUTTON_DOWN) {
				_vm->_game->desiredBorderIndex--;
				if (_vm->_game->desiredBorderIndex < 0) _vm->_game->desiredBorderIndex = g_system->getGraphicsManager()->getNumberOfBorders() - 1;
				g_system->getGraphicsManager()->switchBorder(_vm->_game->desiredBorderIndex);
			}
		}
		break;
		case Common::JoystickButton::JOYSTICK_BUTTON_DPAD_RIGHT:
		{
			if (event.type == Common::EventType::EVENT_JOYBUTTON_DOWN) {
				_vm->_game->desiredBorderIndex = (++_vm->_game->desiredBorderIndex) % g_system->getGraphicsManager()->getNumberOfBorders();
				g_system->getGraphicsManager()->switchBorder(_vm->_game->desiredBorderIndex);
			}
		}
		break;
		case Common::JoystickButton::JOYSTICK_BUTTON_RIGHT_SHOULDER: // Display hints (hotspots / game objects)
		{
			auto manager = g_system->getGraphicsManager();
			if (NULL != manager) manager->displayHotspots(event.type == Common::EventType::EVENT_JOYBUTTON_DOWN);
		}
		break;
		case Common::JoystickButton::JOYSTICK_BUTTON_LEFT_SHOULDER: // Display hints (hotspots / game objects)
			switchGoblin(event);
			break;
		case Common::JoystickButton::JOYSTICK_BUTTON_BACK:
		case Common::JoystickButton::JOYSTICK_BUTTON_START:
			// Feature #24264, to move both the mouse & cursor to the first icon of the MENU.TOT if not displayed yet
			// Only move once the player released the "Back" button !!!
			// Do it for Gob2 & Gob3 **and** not currently in MENU.TOT
			if (
				(_vm->getGameType() == Gob::GameType::kGameTypeGob2 || _vm->getGameType() == Gob::GameType::kGameTypeGob3)
				&& Common::EventType::EVENT_JOYBUTTON_UP == event.type
				&& !_vm->isCurrentTot("MENU.TOT")
			) {
				GraphicsManager* graphicsManager = g_system->getGraphicsManager();

				if (graphicsManager == nullptr) {
					return ans;
				}

				// Warp the mouse cursor to the top left corner of the screen.
				SDL_Event event;

				event.tfinger.touchId   = SDL_TOUCH_MOUSEID;
				event.type              = SDL_FINGERMOTION;
				event.tfinger.type      = SDL_FINGERMOTION;
				event.tfinger.x         = 40.f / 320.f; // Range is between 0 and 1, and the game screen size is 320. We want to go to the 40th pixels out of those 320.
				event.tfinger.y         = 0; // Looks bad, but it's gotta be 0 in order to trigger the menu.
				event.tfinger.fingerId  = 0; // First finger.
				event.tfinger.dx        = 0; // Unused.
				event.tfinger.dy        = 0; // Unused.
				event.tfinger.pressure  = 1; // Unused.
				event.tfinger.timestamp = 0; // Unused.
				event.tfinger.windowID  = 0; // Unused.

				SDL_PushEvent(&event);
			} else if (true) {
				// For Gobliiins, consider the key as an ESCAPE key, like JOYSTICK_BUTTON_START.
				event.kbd.keycode = Common::KEYCODE_ESCAPE;
				ans = true;
			}

			break;
		default: break;
		}
		return ans;
	}

	void Util::processInput(bool scroll) {
		Common::Event event;
		Common::EventManager* eventMan = g_system->getEventManager();
		int16 x = 0, y = 0;
		bool hasMove = false;

#ifdef XBOX_SERIES
		// Force joystick motion events on Xbox only, as the SDL API does not
		// work as the same as the PlayStation & NX one.
		// We have to manually enter "mouse" motion events as the background events
		// are disabled on XInput controllers (outch...).
		// Do not change the SDL API for Xbox as this might break the behaviour of native games
		// with Xbox controller support.
		forceJoystickAxisMotion();
#endif

		_vm->_vidPlayer->updateLive();

		while (eventMan->pollEvent(event)) {
			switch (event.type) {
			case Common::EventType::EVENT_MOUSEMOVE:
				hasMove = true;
				x = event.mouse.x;
				y = event.mouse.y;
				break;
			case Common::EventType::EVENT_LBUTTONDOWN:
				_mouseButtons = (MouseButtons)(((uint32)_mouseButtons) | ((uint32)kMouseButtonsLeft));
				break;
			case Common::EventType::EVENT_RBUTTONDOWN:
				// Remove the left button click first (touch event behaviour)
				_mouseButtons = (MouseButtons)(((uint32)_mouseButtons) & ~((uint32)kMouseButtonsLeft));
				_mouseButtons = (MouseButtons)(((uint32)_mouseButtons) | ((uint32)kMouseButtonsRight));
				break;
			case Common::EventType::EVENT_LBUTTONUP:
				_mouseButtons = (MouseButtons)(((uint32)_mouseButtons) & ~((uint32)kMouseButtonsLeft));
				break;
			case Common::EventType::EVENT_RBUTTONUP:
				_mouseButtons = (MouseButtons)(((uint32)_mouseButtons) & ~((uint32)kMouseButtonsRight));
				break;
			case Common::EventType::EVENT_KEYDOWN:
				keyDown(event);

				if (event.kbd.hasFlags(Common::KBD_CTRL)) {
					if (event.kbd.keycode == Common::KEYCODE_f)
						_fastMode ^= 1;
					else if (event.kbd.keycode == Common::KEYCODE_g)
						_fastMode ^= 2;
					else if (event.kbd.keycode == Common::KEYCODE_p)
						//_vm->pauseGame();
						break;
				}
				addKeyToBuffer(event.kbd);
				break;
			case Common::EventType::EVENT_KEYUP:
				keyUp(event);
				break;
			case Common::EventType::EVENT_JOYBUTTON_DOWN:
				if (preprocessJoystickButtonEvent(event)) {
					keyDown(event);
					addKeyToBuffer(event.kbd);
				}
				break;
			case Common::EventType::EVENT_JOYBUTTON_UP:
				if (preprocessJoystickButtonEvent(event)) keyUp(event);
				break;
			default:
				break;
			}
		}

		if (_vm->_game->firstStepForAutoSave) {
			// Delay event to fire JOYBUTTON_UP event
			// Used for Gob2 & Gob3 for auto-save & auto-load (confirmation screen)!
			{
				Common::Event event;
				event.type = Common::EventType::EVENT_JOYBUTTON_UP;
				event.joystick.button = 0;
				g_system->getEventManager()->notifyEvent(event);
			}
			_vm->_game->firstStepForAutoSave = false;
		}

		_vm->_global->_speedFactor = MIN(_fastMode + 1, 3);
		if (hasMove && scroll) {
			x = CLIP(x, _vm->_global->_mouseMinX, _vm->_global->_mouseMaxX);
			y = CLIP(y, _vm->_global->_mouseMinY, _vm->_global->_mouseMaxY);

			x -= _vm->_video->_screenDeltaX;
			y -= _vm->_video->_screenDeltaY;

			_vm->_util->setMousePos(x, y);
			_vm->_game->wantScroll(x, y);

			// WORKAROUND:
			// Force a check of the mouse in order to fix the sofa bug. This apply only for Gob3, and only
			// in the impacted TOT file so that the second screen animation is not broken.
			if ((_vm->getGameType() == kGameTypeGob3) && _vm->isCurrentTot("EMAP1008.TOT"))
				_vm->_game->evaluateScroll();
		}
	}

	void Util::clearKeyBuf() {
		processInput();
		_keyBufferHead = _keyBufferTail = 0;
	}

	bool Util::keyBufferEmpty() {
		return (_keyBufferHead == _keyBufferTail);
	}

	void Util::addKeyToBuffer(const Common::KeyState& key) {
		if ((_keyBufferHead + 1) % KEYBUFSIZE == _keyBufferTail) {
			warning("key buffer overflow");
			return;
		}

		_keyBuffer[_keyBufferHead] = key;
		_keyBufferHead = (_keyBufferHead + 1) % KEYBUFSIZE;
	}

	bool Util::getKeyFromBuffer(Common::KeyState& key) {
		if (_keyBufferHead == _keyBufferTail) return false;

		key = _keyBuffer[_keyBufferTail];
		_keyBufferTail = (_keyBufferTail + 1) % KEYBUFSIZE;

		return true;
	}

	static const uint16 kLatin1ToCP850[] = {
		0xFF, 0xAD, 0xBD, 0x9C, 0xCF, 0xBE, 0xDD, 0xF5, 0xF9, 0xB8, 0xA6, 0xAE, 0xAA, 0xF0, 0xA9, 0xEE,
		0xF8, 0xF1, 0xFD, 0xFC, 0xEF, 0xE6, 0xF4, 0xFA, 0xF7, 0xFB, 0xA7, 0xAF, 0xAC, 0xAB, 0xF3, 0xA8,
		0xB7, 0xB5, 0xB6, 0xC7, 0x8E, 0x8F, 0x92, 0x80, 0xD4, 0x90, 0xD2, 0xD3, 0xDE, 0xD6, 0xD7, 0xD8,
		0xD1, 0xA5, 0xE3, 0xE0, 0xE2, 0xE5, 0x99, 0x9E, 0x9D, 0xEB, 0xE9, 0xEA, 0x9A, 0xED, 0xE8, 0xE1,
		0x85, 0xA0, 0x83, 0xC6, 0x84, 0x86, 0x91, 0x87, 0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B,
		0xD0, 0xA4, 0x95, 0xA2, 0x93, 0xE4, 0x94, 0xF6, 0x9B, 0x97, 0xA3, 0x96, 0x81, 0xEC, 0xE7, 0x98
	};

	int16 Util::toCP850(uint16 latin1) {
		if ((latin1 < 0xA0) || ((latin1 - 0xA0) >= ARRAYSIZE(kLatin1ToCP850)))
			return 0;

		return kLatin1ToCP850[latin1 - 0xA0];
	}

	int16 Util::translateKey(const Common::KeyState& key) {
		static struct keyS {
			int64 from;
			int16 to;
		} keys[] = {
			{Common::KEYCODE_BACKSPACE, kKeyBackspace},
			{Common::KEYCODE_SPACE,     kKeySpace    },
			{Common::KEYCODE_RETURN,    kKeyReturn   },
			{Common::KEYCODE_ESCAPE,    kKeyEscape   },
			{Common::KEYCODE_DELETE,    kKeyDelete   },
			{Common::KEYCODE_UP,        kKeyUp       },
			{Common::KEYCODE_DOWN,      kKeyDown     },
			{Common::KEYCODE_RIGHT,     kKeyRight    },
			{Common::KEYCODE_LEFT,      kKeyLeft     },
			{Common::KEYCODE_F1,        kKeyF1       },
			{Common::KEYCODE_F2,        kKeyF2       },
			{Common::KEYCODE_F3,        kKeyF3       },
			{Common::KEYCODE_F4,        kKeyF4       },
			{Common::KEYCODE_F5,        kKeyEscape   },
			{Common::KEYCODE_F6,        kKeyF6       },
			{Common::KEYCODE_F7,        kKeyF7       },
			{Common::KEYCODE_F8,        kKeyF8       },
			{Common::KEYCODE_F9,        kKeyF9       },
			{Common::KEYCODE_F10,       kKeyF10      }
		};

		// Translate special keys
		for (int i = 0; i < ARRAYSIZE(keys); i++)
			if (key.keycode == keys[i].from)
				return keys[i].to;

		// Return the ascii value, for text input
		if ((key.ascii >= 32) && (key.ascii <= 127))
			return key.ascii;

		// Translate international characters into CP850 characters
		if ((key.ascii >= 160) && (key.ascii <= 255))
			return toCP850(key.ascii);

		return 0;
	}

	static const uint8 kLowerToUpper[][2] = {
		{0x81, 0x9A},
		{0x82, 0x90},
		{0x83, 0xB6},
		{0x84, 0x8E},
		{0x85, 0xB7},
		{0x86, 0x8F},
		{0x87, 0x80},
		{0x88, 0xD2},
		{0x89, 0xD3},
		{0x8A, 0xD4},
		{0x8B, 0xD8},
		{0x8C, 0xD7},
		{0x8D, 0xDE},
		{0x91, 0x92},
		{0x93, 0xE2},
		{0x94, 0x99},
		{0x95, 0xE3},
		{0x96, 0xEA},
		{0x97, 0xEB},
		{0x95, 0xE3},
		{0x96, 0xEA},
		{0x97, 0xEB},
		{0x9B, 0x9D},
		{0xA0, 0xB5},
		{0xA1, 0xD6},
		{0xA2, 0xE0},
		{0xA3, 0xE9},
		{0xA4, 0xA5},
		{0xC6, 0xC7},
		{0xD0, 0xD1},
		{0xE4, 0xE5},
		{0xE7, 0xE8},
		{0xEC, 0xED}
	};

	char Util::toCP850Lower(char cp850) {
		const uint8 cp = (unsigned char)cp850;
		if (cp <= 32)
			return cp850;

		if (cp <= 127)
			return tolower(cp850);

		for (uint i = 0; i < ARRAYSIZE(kLowerToUpper); i++)
			if (cp == kLowerToUpper[i][1])
				return (char)kLowerToUpper[i][0];

		return cp850;
	}

	char Util::toCP850Upper(char cp850) {
		const uint8 cp = (unsigned char)cp850;
		if (cp <= 32)
			return cp850;

		if (cp <= 127)
			return toupper(cp850);

		for (uint i = 0; i < ARRAYSIZE(kLowerToUpper); i++)
			if (cp == kLowerToUpper[i][0])
				return (char)kLowerToUpper[i][1];

		return cp850;
	}

	int16 Util::getKey() {
		Common::KeyState key;

		while (!getKeyFromBuffer(key)) {
			processInput();

			if (keyBufferEmpty())
				g_system->delayMillis(10 / _vm->_global->_speedFactor);
		}
		return translateKey(key);
	}

	int16 Util::checkKey() {
		Common::KeyState key;

		getKeyFromBuffer(key);

		return translateKey(key);
	}

	bool Util::checkKey(int16& key) {
		Common::KeyState keyS;

		if (!getKeyFromBuffer(keyS))
			return false;

		key = translateKey(keyS);

		return true;
	}

	bool Util::keyPressed() {
		int16 key = checkKey();
		if (key)
			return true;

		int16 x, y;
		MouseButtons buttons;

		getMouseState(&x, &y, &buttons);
		return buttons != kMouseButtonsNone;
	}

	void Util::getMouseState(int16* pX, int16* pY, MouseButtons* pButtons) {
		Common::Point mouse = g_system->getEventManager()->getMousePos();
		*pX = mouse.x + _vm->_video->_scrollOffsetX - _vm->_video->_screenDeltaX;
		*pY = mouse.y + _vm->_video->_scrollOffsetY - _vm->_video->_screenDeltaY;

		if (pButtons != nullptr)
			*pButtons = _mouseButtons;
	}

	void Util::setMousePos(int16 x, int16 y) {
		x = CLIP<int>(x + _vm->_video->_screenDeltaX, 0, _vm->_width - 1);
		y = CLIP<int>(y + _vm->_video->_screenDeltaY, 0, _vm->_height - 1);
		g_system->getEventManager()->purgeMouseEvents();
		g_system->warpMouse(x, y);
	}

	void Util::waitMouseUp() {
		do {
			processInput();
			if (_mouseButtons != kMouseButtonsNone)
				delay(10);
		} while (_mouseButtons != kMouseButtonsNone);
	}

	void Util::waitMouseDown() {
		int16 x;
		int16 y;
		MouseButtons buttons;

		do {
			processInput();
			getMouseState(&x, &y, &buttons);
			if (buttons == 0)
				delay(10);
		} while (buttons == 0);
	}

	void Util::waitMouseRelease(char drawMouse) {
		MouseButtons buttons;
		int16 mouseX;
		int16 mouseY;

		_vm->_game->checkKeys(&mouseX, &mouseY, &buttons, drawMouse);
		while (buttons != 0) {
			if (drawMouse != 0)
				_vm->_draw->animateCursor(2);
			delay(10);
			_vm->_game->checkKeys(&mouseX, &mouseY, &buttons, drawMouse);
		}
	}

	void Util::forceMouseUp(bool onlyWhenSynced) {
		if (onlyWhenSynced && (_vm->_game->_mouseButtons != _mouseButtons))
			return;

		_vm->_game->_mouseButtons = kMouseButtonsNone;
		_mouseButtons = kMouseButtonsNone;
	}

	void Util::forceJoystickAxisMotion() {
		// There is a flaw on Xbox, where events are not pushed for joystick axis!
		// Manually get the values & push the events for Xbox only, which is not the case for PS5 / Switch
		// This is done by design on SDL (!!!!) so including this code on our SDL version for Xbox
		// might break things in the future, which is why we are doing this directly in the engine...
		SDL_GameController* _controller = SDL_GameControllerOpen(0);
		if (NULL != _controller && !_vm->_game->isPlayerStatePaused()) {
			{
				const auto leftXValue = SDL_GameControllerGetAxis(_controller, SDL_CONTROLLER_AXIS_LEFTX);
				if (leftXValue != 0) {
					SDL_Event leftXEvent{};
					leftXEvent.type = SDL_CONTROLLERAXISMOTION;
					leftXEvent.caxis.axis = SDL_CONTROLLER_AXIS_LEFTX;
					leftXEvent.caxis.value = leftXValue;
					SDL_PushEvent(&leftXEvent);
				}
			}
			{
				const auto leftYValue = SDL_GameControllerGetAxis(_controller, SDL_CONTROLLER_AXIS_LEFTY);
				if (leftYValue != 0) {
					SDL_Event leftYEvent{};
					leftYEvent.type = SDL_CONTROLLERAXISMOTION;
					leftYEvent.caxis.axis = SDL_CONTROLLER_AXIS_LEFTY;
					leftYEvent.caxis.value = leftYValue;
					SDL_PushEvent(&leftYEvent);
				}
			}
		}
	}

	void Util::clearPalette() {
		int16 i;
		byte colors[768];

		_vm->validateVideoMode(_vm->_global->_videoMode);

		if (_vm->_global->_setAllPalette) {
			if (_vm->getPixelFormat().bytesPerPixel == 1) {
				memset(colors, 0, sizeof(colors));
				g_system->getPaletteManager()->setPalette(colors, 0, 256);
			}

			return;
		}

		for (i = 0; i < 16; i++)
			_vm->_video->setPalElem(i, 0, 0, 0, 0, _vm->_global->_videoMode);
	}

	int16 Util::getFrameRate() {
		return _frameRate;
	}

	void Util::setFrameRate(int16 rate) {
		if (rate == 0)
			rate = 1;

		_frameRate = rate;
		_frameWaitTime = 1000 / rate;
		_startFrameTime = getTimeKey();
	}

	void Util::notifyNewAnim() {
		_startFrameTime = getTimeKey();
	}

	void Util::waitEndFrame(bool handleInput) {
		int32 time;

		time = getTimeKey() - _startFrameTime;
		if ((time > 1000) || (time < 0)) {
			_vm->_video->retrace();
			_startFrameTime = getTimeKey();
			return;
		}

		int32 toWait = 0;
		do {
			if (toWait > 0)
				delay(MIN<int>(toWait, 10));

			if (handleInput)
				processInput();

			_vm->_video->retrace();

			time = getTimeKey() - _startFrameTime;
			toWait = _frameWaitTime - time;
		} while (toWait > 0);

		_startFrameTime = getTimeKey();
	}

	void Util::setScrollOffset(int16 x, int16 y) {
		processInput();

		if (x >= 0)
			_vm->_video->_scrollOffsetX = x;
		else
			_vm->_video->_scrollOffsetX = _vm->_draw->_scrollOffsetX;

		if (y >= 0)
			_vm->_video->_scrollOffsetY = y;
		else
			_vm->_video->_scrollOffsetY = _vm->_draw->_scrollOffsetY;

		_vm->_video->waitRetrace();
	}

	void Util::insertStr(const char* str1, char* str2, int16 pos) {
		int len1 = strlen(str1);
		int len2 = strlen(str2);
		int from = MIN((int)pos, len2);

		for (int i = len2; i >= from; i--)
			str2[len1 + i] = str2[i];
		for (int i = 0; i < len1; i++)
			str2[i + from] = str1[i];
	}

	void Util::cutFromStr(char* str, int16 from, int16 cutlen) {
		int len = strlen(str);

		if (from >= len)
			return;
		if ((from + cutlen) > len) {
			str[from] = 0;
			return;
		}

		int i = from;
		do {
			str[i] = str[i + cutlen];
			i++;
		} while (str[i] != 0);
	}

	// A copy of this utility function is used by fileio.cpp.
	void Util::replaceChar(char* str, char c1, char c2) {
		while ((str = strchr(str, c1)))
			*str = c2;
	}

	static const char trStr1[] =
		"       '   + - :0123456789: <=>  abcdefghijklmnopqrstuvwxyz      "
		"abcdefghijklmnopqrstuvwxyz     ";
	static const char trStr2[] =
		" ueaaaaceeeiii     ooouu        aioun"
		"                                                           ";
	static const char trStr3[] = "                                ";

	void Util::cleanupStr(char* str) {
		char* start, * end;
		char buf[300];

		Common::strcpy_s(buf, trStr1);
		Common::strcat_s(buf, trStr2);
		Common::strcat_s(buf, trStr3);

		// Translating "wrong" characters
		for (size_t i = 0; i < strlen(str); i++)
			str[i] = buf[MIN<int>(str[i] - 32, 32)];

		// Trim spaces left
		while (str[0] == ' ')
			cutFromStr(str, 0, 1);

		// Trim spaces right
		while ((*str != '\0') && (str[strlen(str) - 1] == ' '))
			cutFromStr(str, strlen(str) - 1, 1);

		// Merge double spaces
		start = strchr(str, ' ');
		while (start) {
			if (start[1] == ' ') {
				cutFromStr(str, start - str, 1);
				continue;
			}

			end = strchr(start + 1, ' ');
			start = end ? end + 1 : nullptr;
		}
	}

	void Util::listInsertFront(List* list, void* data) {
		ListNode* node;

		node = new ListNode;
		if (list->pHead) {
			node->pData = data;
			node->pNext = list->pHead;
			node->pPrev = nullptr;
			list->pHead->pPrev = node;
			list->pHead = node;
		}
		else {
			list->pHead = node;
			list->pTail = node;
			node->pData = data;
			node->pNext = nullptr;
			node->pPrev = nullptr;
		}
	}

	void Util::listInsertBack(List* list, void* data) {
		ListNode* node;

		if (list->pHead != nullptr) {
			if (list->pTail == nullptr) {
				list->pTail = list->pHead;
				warning("Util::listInsertBack(): Broken list");
			}

			node = new ListNode;
			node->pData = data;
			node->pPrev = list->pTail;
			node->pNext = nullptr;
			list->pTail->pNext = node;
			list->pTail = node;
		}
		else
			listInsertFront(list, data);
	}

	void Util::listDropFront(List* list) {
		if (list->pHead->pNext == nullptr) {
			delete list->pHead;
			list->pHead = nullptr;
			list->pTail = nullptr;
		}
		else {
			list->pHead = list->pHead->pNext;
			delete list->pHead->pPrev;
			list->pHead->pPrev = nullptr;
		}
	}

	void Util::deleteList(List* list) {
		while (list->pHead)
			listDropFront(list);

		delete list;
	}

#if 0
	char* Util::setExtension(char* str, const char* ext) {
		assert(str && ext);

		if (str[0] == '\0')
			return str;

		char* dot = strrchr(str, '.');
		if (dot)
			*dot = '\0';

		strcat(str, ext);
		return str;
	}
#endif

	Common::String Util::setExtension(const Common::String& str, const Common::String& ext) {
		if (str.empty())
			return str;

		const char* dot = strrchr(str.c_str(), '.');
		if (dot)
			return Common::String(str.c_str(), dot - str.c_str()) + ext;

		return str + ext;
	}

	Common::String Util::readString(Common::SeekableReadStream& stream, int n) {
		Common::String str;

		char c;
		while (n-- > 0) {
			if ((c = stream.readByte()) == '\0')
				break;

			str += c;
		}

		if (n > 0)
			stream.skip(n);

		return str;
	}

	/* NOT IMPLEMENTED */
	void Util::checkJoystick() {
		_vm->_global->_useJoystick = 0;
	}

	uint32 Util::getKeyState() const {
		return _keyState;
	}

	void Util::keyDown(const Common::Event& event) {
		if (event.kbd.keycode == Common::KEYCODE_UP)
			_keyState |= 0x0001;
		else if (event.kbd.keycode == Common::KEYCODE_DOWN)
			_keyState |= 0x0002;
		else if (event.kbd.keycode == Common::KEYCODE_RIGHT)
			_keyState |= 0x0004;
		else if (event.kbd.keycode == Common::KEYCODE_LEFT)
			_keyState |= 0x0008;
		else if (event.kbd.keycode == Common::KEYCODE_SPACE)
			_keyState |= 0x0020;
		else if (event.kbd.keycode == Common::KEYCODE_ESCAPE)
			_keyState |= 0x0040;
	}

	void Util::keyUp(const Common::Event& event) {
		if (event.kbd.keycode == Common::KEYCODE_UP)
			_keyState &= ~0x0001;
		else if (event.kbd.keycode == Common::KEYCODE_DOWN)
			_keyState &= ~0x0002;
		else if (event.kbd.keycode == Common::KEYCODE_RIGHT)
			_keyState &= ~0x0004;
		else if (event.kbd.keycode == Common::KEYCODE_LEFT)
			_keyState &= ~0x0008;
		else if (event.kbd.keycode == Common::KEYCODE_SPACE)
			_keyState &= ~0x0020;
		else if (event.kbd.keycode == Common::KEYCODE_ESCAPE)
			_keyState &= ~0x0040;
	}

} // End of namespace Gob
