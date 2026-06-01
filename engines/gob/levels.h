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

#ifndef GOB_LEVELS_H
#define GOB_LEVELS_H

#include "common/str.h"
#include <map>
#include <optional>

namespace Gob {
namespace Levels {

	/// <summary>
	/// Shared enum for Gob1, Gob2 and Gob3.
	/// The "Level" enum is appropriate for Gob1, not for Gob2 & Gob3 which is more a "Chapter" idea.
	/// The idea of "Chapter" contains multiple levels, but we are **not** interested in saving & loading levels for Gob2 & Gob3, compared to Gob1.
	/// Currently you can't know which TOT scripts are contained per chapter for Gob2 and Gob3.
	/// </summary>
	enum Level
	{
		UNKNOWN = 0,
		ONE = 1,
		TWO,
		THREE,
		FOUR,
		FIVE,
		SIX,
		SEVEN,
		EIGHT,     // Gob 2 END OF CHAPTERS
		NINE,
		TEN,       // Gob 3 END OF CHAPTERS
		ELEVEN,
		TWELVE,
		THIRTEEN,
		FOURTEEN,
		FIFTEEN,
		SIXTEEN,
		SEVENTEEN,
		EIGHTEEN,
		NINETEEN,
		TWENTY,
		TWENTYONE,
		TWENTYTWO, // Gob 1 LAST LEVEL
		TWENTYTHREE,
		TWENTYFOUR,
		TWENTYFIVE,
		TWENTYSIX,
		TWENTYSEVEN,
		TWENTYEIGHT,
		TWENTYNINE,
		THIRTY,
		THIRTYONE,
		THIRTYTWO // For Woodruff ?
	};
	
namespace Gob1 {

	/// <summary>
	/// Mapping table to get the tot file of a given level
	/// </summary>
	static const std::map<Gob::Levels::Level, Common::String> MAPPED_LEVEL_NAMES = {
		{ Gob::Levels::Level::ONE,         "avt003.tot"},
		{ Gob::Levels::Level::TWO,         "avt00.tot"},
		{ Gob::Levels::Level::THREE,       "avt002.tot"},
		{ Gob::Levels::Level::FOUR,        "avt004.tot"},
		{ Gob::Levels::Level::FIVE,        "avt005.tot"},
		{ Gob::Levels::Level::SIX,         "avt001.tot"},
		{ Gob::Levels::Level::SEVEN,       "avt007.tot"},
		{ Gob::Levels::Level::EIGHT,       "avt006.tot"},
		{ Gob::Levels::Level::NINE,		   "avt009.tot"},
		{ Gob::Levels::Level::TEN,		   "avt008.tot"},
		{ Gob::Levels::Level::ELEVEN,	   "avt010.tot"},
		{ Gob::Levels::Level::TWELVE,	   "avt011.tot"},
		{ Gob::Levels::Level::THIRTEEN,	   "avt012.tot"},
		{ Gob::Levels::Level::FOURTEEN,	   "avt014.tot"},
		{ Gob::Levels::Level::FIFTEEN,	   "avt015.tot"},
		{ Gob::Levels::Level::SIXTEEN,     "avt016.tot"},
		{ Gob::Levels::Level::SEVENTEEN,   "avt017.tot"},
		{ Gob::Levels::Level::EIGHTEEN,    "avt018.tot"},
		{ Gob::Levels::Level::NINETEEN,	   "avt019.tot"},
		{ Gob::Levels::Level::TWENTY,      "avt020.tot"},
		{ Gob::Levels::Level::TWENTYONE,   "avt021.tot"},
		{ Gob::Levels::Level::TWENTYTWO,   "avt022.tot"},
		{ Gob::Levels::Level::TWENTYTHREE, "final.tot"},
	};

	/// <summary>
	/// Mapping table to get the password to reach the right level
	/// </summary>
	static const std::map<Gob::Levels::Level, Common::String> MAPPED_LEVEL_PASSWORDS = {
		{ Gob::Levels::Level::TWO,         "VQVQFDE"},
		{ Gob::Levels::Level::THREE,       "ICIGCAA"},
		{ Gob::Levels::Level::FOUR,        "ECPQPCC"},
		{ Gob::Levels::Level::FIVE,        "FTWKFEN"},
		{ Gob::Levels::Level::SIX,         "HQWFTFW"},
		{ Gob::Levels::Level::SEVEN,       "DWNDGBW"},
		{ Gob::Levels::Level::EIGHT,       "JCJCJHM"},
		{ Gob::Levels::Level::NINE,		   "ICVGCGT"},
		{ Gob::Levels::Level::TEN,		   "LQPCUJV"},
		{ Gob::Levels::Level::ELEVEN,	   "HNWVGKB"},
		{ Gob::Levels::Level::TWELVE,	   "FTQKVLE"},
		{ Gob::Levels::Level::THIRTEEN,	   "DCPLQMH"},
		{ Gob::Levels::Level::FOURTEEN,	   "EWDGPNL"},
		{ Gob::Levels::Level::FIFTEEN,	   "TCNGTOV"},
		{ Gob::Levels::Level::SIXTEEN,     "TCVQRPM"},
		{ Gob::Levels::Level::SEVENTEEN,   "IQDNKQO"},
		{ Gob::Levels::Level::EIGHTEEN,    "KKKPURE"},
		{ Gob::Levels::Level::NINETEEN,	   "NGOGKSP"},
		{ Gob::Levels::Level::TWENTY,      "NNGWTTO"},
		{ Gob::Levels::Level::TWENTYONE,   "LGWFGUS"},
		{ Gob::Levels::Level::TWENTYTWO,   "TQNGFVC"},
	};

	/// <summary>
	/// Return a password based on the level id passed as parameter
	/// </summary>
	/// <param name="level">Level to get the password from</param>
	/// <returns>The password, as an optional string type</returns>
	std::optional<Common::String> getPasswordFromLevel(const Gob::Levels::Level level);

	/// <summary>
	/// Return a password based on the level tot file passed as parameter
	/// </summary>
	/// <param name="level">Tot file to get the password from</param>
	/// <returns>The password, as an optional string type</returns>
	std::optional<Common::String> getPasswordFromLevel(const Common::String& level);

	/// <summary>
	/// Return a level id based on the level tot file passed as parameter
	/// </summary>
	/// <param name="levelName">Tot file to get the password from</param>
	/// <returns>The level id that corresponds to the tot file. If the level is unknown or corrupted then it should return Level::UNKNOWN.</returns>
	Gob::Levels::Level getLevelIdFromLevelName(const Common::String& levelName);

}

namespace Gob2 {
	/// <summary>
	/// Mapping table to get the tot file of a given level
	/// </summary>
	static const std::map<Gob::Levels::Level, Common::String> MAPPED_LEVEL_NAMES = {
		{ Gob::Levels::Level::TWO,   "gob08.tot"},
		{ Gob::Levels::Level::THREE, "gob10.tot"},
		{ Gob::Levels::Level::FOUR,  "gob12.tot"},
		{ Gob::Levels::Level::FIVE,  "gob14.tot"},
		{ Gob::Levels::Level::SIX,   "gob18.tot"},
		{ Gob::Levels::Level::SEVEN, "gob21.tot"},
		{ Gob::Levels::Level::EIGHT, "final.tot"},
	};

	Common::String getLevelIdStringFrom(const Gob::Levels::Level level);
}

namespace Gob3 {
/// <summary>
	/// Mapping table to get the tot file of a given level
	/// </summary>
	static const std::map<Gob::Levels::Level, Common::String> MAPPED_LEVEL_NAMES = {
		{ Gob::Levels::Level::TWO,   "emap1002.tot"},
		{ Gob::Levels::Level::THREE, "emap1003.tot"},
		{ Gob::Levels::Level::FOUR,  "emap1004.tot"},
		{ Gob::Levels::Level::FIVE,  "emap1007.tot"},
		{ Gob::Levels::Level::SIX,   "emap1010.tot"},
		{ Gob::Levels::Level::SEVEN, "emap1013.tot"},
		{ Gob::Levels::Level::EIGHT, "emap1015.tot"},
		{ Gob::Levels::Level::NINE,  "emap1017.tot"},
		{ Gob::Levels::Level::TEN,   "emap1019.tot"},
	};

	Common::String getLevelIdStringFrom(const Gob::Levels::Level level);

}
}
}

#endif // GOB_LEVELS_H