/* Lua talks to JACK transport! Rad!
 *
 * Copyright (C) 2013 Chad Cassady <chad@beatboxchad.com>
 *
 * Many thanks to this dude: https://github.com/x42/jack_midi_clock.  And also
 * to the super-badass JACK folks who provided this example:
 * http://jackit.sourceforge.net/cgi-bin/lxr/http/source/example-clients/showtime.c
 * 
 * Both of those examples really helped me wrap my mind around everything.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is an incomplete implementation, but it works, and that's amazing to
 * me. I will expand this to be a general-purpose interface to JACK from Lua,
 * but I can grab my BBT info now.
 *
 * TODO:
 * - come up with a better name for the thing.
 * - maybe rename the repo
 * - add midi support
 * - add to this list, because there's a lot to do.
 * 
 */

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>


static jack_client_t *client; 	
static jack_status_t *status;
static jack_port_t *midi_out;

static int showtime (lua_State *L) {
	static jack_position_t current; 
	static jack_transport_state_t transport_state;
	

	transport_state = jack_transport_query(client, &current);

	/* The number of arguments we'll be returning to the stack for Lua to have
	 * depends on what transport information is currently available, so I'll just
	 * increment it as we go
	*/

	int num_arguments = 0;

	lua_pushnumber(L, current.frame);
	num_arguments++;

  switch (transport_state) {
		case JackTransportStopped:
			lua_pushstring(L, "state: Stopped");
			num_arguments++;
			break;
		case JackTransportRolling:
			lua_pushstring(L, "state: Rolling");
			num_arguments++;
			break;
		case JackTransportStarting:
			lua_pushstring(L, "state: Starting");
			num_arguments++;
			break;
		default:
			lua_pushstring(L, "state: [unknown]");
			num_arguments++;
	}
	// push BBT information to the stack if it exists
  if (current.valid & JackPositionBBT) {
		lua_pushnumber(L, current.bar);
		lua_pushnumber(L, current.beat);
		lua_pushnumber(L, current.tick);
		lua_pushnumber(L, current.beats_per_bar);
		lua_pushnumber(L, current.beat_type);
		lua_pushnumber(L, current.ticks_per_beat);
		lua_pushnumber(L, current.beats_per_minute);
		num_arguments = num_arguments + 7;
	}
	// I don't know what this is for yet, I only really care about BBT actually,
	// but probably should include it.
	//if (current.valid & JackPositionTimecode) {
		lua_pushnumber(L, current.frame_time);
		lua_pushnumber(L, current.next_time);
		lua_pushnumber(L, current.usecs);
		num_arguments = num_arguments + 3;
	//}
	return num_arguments;
}



static int open_client(lua_State *L) {
	/* try to become a client of the JACK server */
	char *client_name = lua_tostring(L, 1);
	client = jack_client_open(client_name, 0, status);
	jack_activate (client); // TODO add some error handling around this. 
	return 1;
};

static int close_client(lua_State *L) {
	jack_client_close (client);
	return 0;
};

// register an outbound MIDI port
static int register_midi_out(lua_State *L) {
	char *port_name = lua_tostring(L,1);
	midi_out = jack_port_register(client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsTerminal, 2048);
	return 0;
};

// write some data to that port. 

static const struct luaL_Reg lua_jack [] = {
	{"showtime", showtime},
	{"open_client", open_client},
	{"close_client", close_client},
	{"register_midi_out", register_midi_out},
	{NULL, NULL}
};

int luaopen_liblua_jack (lua_State *L) {
	luaL_newlib(L, lua_jack);
	return 1;
}

/* 
 * These are leftovers from the example program I adapted this from at first.
 * Since this is a library I don't know if I need to register a callback for if
 * jack shuts down.  
 */

/*
void jack_shutdown (void *arg) {
	exit (1);
}
*/

/* tell the JACK server to call `jack_shutdown()' if it ever shuts down, either
 * entirely, or if it just decides to stop calling us.  
 */

/*
	jack_on_shutdown (client, jack_shutdown, 0);

	 tell the JACK server that we are ready to roll 

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		return 1;
	}

*/
