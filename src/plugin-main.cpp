/*
Plugin Name
Copyright (C) 2026 Owen Isenhart oisenhart.college@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
// so what do we ned
// we need a frontend that has configuration options
//    - a number input for delay in seconds
//    - an option to either cut the delay on button press or fill in the delay with an image/source
//    - the actual button to trigger the dump
// we need the logic to handle the dump
//    - if the option is to cut the delay, then we need to clear the heap/stack/buffer and make the next frame the current one
//    - if the option is to fill the delay with an image/source, then we need to change the data of all the frames in the delay to the image/source and then make the next frame the current one

// we should also consider allowing the user to only dump a certain amount of the delay
// for example, if the delay is 10 seconds and the user holds the butotn for 5 seconds, then we should only dump the first 5 seconds of the delay

// classes we'll need:
// - a class to represent the delay buffer, which will hold the frames and their timestamps
// - a class to represent the configuration options, which will hold the delay time and the dump type
// - a class to represent the frontend, which will handle the user interface and the button
// and then, this class will combine everything and handle the logic of dumping the delay when the button is pressed based on the configuration options

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
