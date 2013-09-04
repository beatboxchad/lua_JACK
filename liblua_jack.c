/* Lua talks to JACK transport! Rad!
 *
 * Copyright (C) 2013 Chad Cassady <chad@beatboxchad.com>
 * Many thanks to this dude: https://github.com/x42/jack_midi_clock.  And also
 * to the super-badass JACK folks who provided this example:
 * http://jackit.sourceforge.net/cgi-bin/lxr/http/source/example-clients/showtime.c
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
 * This is some of the first C I've ever written, other than some cannibalized
 * stuff on Arduino. It's gonna be horrible, and probably won't even work for
 * awhile. Please do not read this if bad code makes you cry. Tackling such a
 * complex project as my first go is a decision I alone should have to live
 * with ;)
 * 
 * This is an incomplete implementation, but it works, and that's amazing to
 * me. I will expand this to be a general-purpose interface to JACK from Lua,
 * but I can grab my BBT info now.
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
//for lack of a better understanding on how to properly link things at compile time, I'm going to try the dynamic library open trick
#include <dlfcn.h>

//void *dlopen(const char *libjack, int RTLD_NOW); // this looks like it's maybe just a prototype.

static jack_client_t *client; 	
static jack_status_t *status;

// My understanding was that each function has its own private stack Lua reads
// its shit from, each time it's called. Thus Lua has to be the one to remember
// shit and pass it properly... Like what's our client. How do I share this
// information? There's no main() here, so how do I handle this in Lua? There
// is a way, I'm just a noob. James told me that statics will be able to be
// read, and it does sure enough compile.

// maybe I should have an init function that declares the client pointer

static int showtime (lua_State *L) {
	//jack_client_t *client;
	//lua_gettable(L, LUA_REGISTRYINDEX);
	//void client_addy = lua_gettop(L);
	//client = &client_addy;

	//lua_pushstring(L, "client_pointer");
	//char *client_name = lua_tostring(L, 1);
	static jack_position_t current; 
	static jack_transport_state_t transport_state;
	

	transport_state = jack_transport_query(client, &current); // undefined symbol? wtf?

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
		num_arguments = num_arguments + 3;
	}
	// I don't know what this is for yet, I only really care about BBT actually,
	// but probably should include it.
	if (current.valid & JackPositionTimecode) {
		lua_pushnumber(L, current.frame_time);
		lua_pushnumber(L, current.next_time);
		num_arguments = num_arguments + 2;
	}
	return num_arguments;
}

/* need to reimplement this, but need to learn how to talk to Lua

void jack_shutdown (void *arg) {
	exit (1);
}

*/


static int become_a_client(lua_State *L) {
	/* try to become a client of the JACK server */
	char *client_name = lua_tostring(L, 1);
	client = jack_client_open(client_name, 0, status);
		return 1;
	//put the client reference into the registry
	//lua_pushstring(L, "client_pointer");
	//lua_pushlightuserdata(L, *client);
	//lua_settable(L, LUA_REGISTRYINDEX);
	// I'm trying to put the the pointer into the registry, and the only way I
	// can see to do that is pushlightuserdata. Next I need to figure out how to
	// retrieve it, if I've even done this part right.
}

/* All of the following still needs to be implemented


	* tell the JACK server to call `jack_shutdown()' if
		 it ever shuts down, either entirely, or if it
		 just decides to stop calling us.
	*

	jack_on_shutdown (client, jack_shutdown, 0);

	 tell the JACK server that we are ready to roll 

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		return 1;
	}
	
int close_client
	char client_name = lua_tolstring(L, 1, NULL);
	jack_client_close (client_name);
	return 0;

/ gotta do that thing with the macro that pushes all the available functions
*/

static const struct luaL_Reg lua_jack [] = {
	{"showtime", showtime},
	{"become_a_client", become_a_client},
	{NULL, NULL}
};

int luaopen_liblua_jack (lua_State *L) {
	luaL_newlib(L, lua_jack);
	return 1;

}
