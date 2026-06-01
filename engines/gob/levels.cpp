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

#include "gob/levels.h"

std::optional<Common::String> Gob::Levels::Gob1::getPasswordFromLevel(const Gob::Levels::Level level) {
	const auto it = MAPPED_LEVEL_PASSWORDS.find(level);
	if (it == MAPPED_LEVEL_PASSWORDS.end()) return std::nullopt;
	return std::optional<Common::String>(it->second);
}

std::optional<Common::String> Gob::Levels::Gob1::getPasswordFromLevel(const Common::String& level) {
	for (const auto& [levelId, levelName] : MAPPED_LEVEL_NAMES) {
		if (levelName.matchString(level)) return getPasswordFromLevel(levelId);
	}
	return std::nullopt;
}

Gob::Levels::Level Gob::Levels::Gob1::getLevelIdFromLevelName(const Common::String& levelName) {
	for (const auto& [id, name] : MAPPED_LEVEL_NAMES) {
		if (levelName.matchString(name)) return id;
	}
	return Gob::Levels::Level::UNKNOWN;
}

Common::String Gob::Levels::Gob2::getLevelIdStringFrom(const Gob::Levels::Level level) {
	// Only 7 levels in Gob2
	switch (level) {
	case Gob::Levels::Level::ONE:   return "1";
	case Gob::Levels::Level::TWO:	return "2";
	case Gob::Levels::Level::THREE: return "3";
	case Gob::Levels::Level::FOUR:  return "4";
	case Gob::Levels::Level::FIVE:  return "5";
	case Gob::Levels::Level::SIX:   return "6";
	case Gob::Levels::Level::SEVEN: return "7";
	case Gob::Levels::Level::EIGHT: return "8";
	default: return "1";
	}
}

Common::String Gob::Levels::Gob3::getLevelIdStringFrom(const Gob::Levels::Level level) {
	// Only 10 levels in Gob2
	switch (level) {
	case Gob::Levels::Level::ONE:   return "1";
	case Gob::Levels::Level::TWO:	return "2";
	case Gob::Levels::Level::THREE: return "3";
	case Gob::Levels::Level::FOUR:  return "4";
	case Gob::Levels::Level::FIVE:  return "5";
	case Gob::Levels::Level::SIX:   return "6";
	case Gob::Levels::Level::SEVEN: return "7";
	case Gob::Levels::Level::EIGHT: return "8";
	case Gob::Levels::Level::NINE:  return "9";
	case Gob::Levels::Level::TEN:   return "10";
	default: return "1";
	}
}