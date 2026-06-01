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

#include "common/system.h"
#include "common/error.h"
#include "common/file.h"
#include "audio/mixer.h"

// used by gob
class Engine
{
public:
	Engine(OSystem* syst);
	virtual ~Engine();
	virtual void initializePath(const Common::Path& gamePath);
	virtual Common::Error run(const bool shouldSkipIntro, const int forcedToLevel) = 0;
	virtual void syncSoundSettings();
	static bool shouldQuit();
	bool existExtractedCDAudioFiles(uint track = 1);
	bool isDataAndCDAudioReadFromSameCD();
	void warnMissingExtractedCDAudio();
public:
	OSystem* _system;
	Audio::Mixer* _mixer;
protected:
	// Set in Gob
	Common::String _targetName;
};
