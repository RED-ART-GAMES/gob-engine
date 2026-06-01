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

#include "engines/engine.h"
#include "common/config-manager.h"
#include "common/audiocd/audiocd.h"
#include "common/events.h"
#include "common/archive.h"

Engine::Engine(OSystem* syst)
	: _system(syst)
	, _mixer(syst->getMixer())
{
}

Engine::~Engine()
{
}

void Engine::initializePath(const Common::Path& gamePath) { SearchMan.addDirectory(gamePath.toString(), gamePath.toString(), 0, 6); }
bool Engine::existExtractedCDAudioFiles(uint track) { return g_system->getAudioCDManager()->existExtractedCDAudioFiles(track); }
bool Engine::isDataAndCDAudioReadFromSameCD() { return g_system->getAudioCDManager()->isDataAndCDAudioReadFromSameCD(); }
void Engine::warnMissingExtractedCDAudio() { error("CD is missing!"); }
// Only events have shouldQuit
bool Engine::shouldQuit() { return g_system->getEventManager()->shouldQuit(); }

void Engine::syncSoundSettings()
{
	// checked gob/gob.cpp and volume is gotten in confman wwith these vars
	_mixer->setVolumeForSoundType(Audio::SoundType::kMusicSoundType, ConfMan.getInt("music_volume"));
	_mixer->setVolumeForSoundType(Audio::SoundType::kSFXSoundType, ConfMan.getInt("sfx_volume"));
	_mixer->setVolumeForSoundType(Audio::SoundType::kSpeechSoundType, ConfMan.getInt("speech_volume"));
}