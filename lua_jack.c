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
 * - add a process callback at the appropriate time
 * - add midi support (in progress)
 * - 
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

/* --- How I think I should implement the MIDI events ---
 *
 * I should create a global buffer I push MIDI events onto. queue_midi will do
 * this. Then process() will take that buffer and pop off as many events as
 * applicable within the nframes parameter. This means that my buffer
 * (accessible to all funcitons) should have some sort of metadata about the
 * timing of the MIDI events in terms of frames. Lua's gonna be doing it like,
 * "right now" but that should be easy to translate to "what right now was
 * right then" for each event. 
 *
 */

static int queue_midi(lua_State *L) {

/* for now, I'm going to send realtime messages every time the process callback
 * is called. 
 
 * This is probably not going to work for a sequencer, and I'm going to have to
 * decide where to put the logic for that. I think that stuff belongs unexposed
 * to Lua. I want to keep the actual lua interface simple and easy, and do all
 * the number-crunching down here. Thinking of how that would ideally look:
 * 
 * Lua will schedule MIDI events passing down the raw MIDI data (that's okay
 * with me for now, though it'd be nifty to make it more user-friendly in the
 * future) and the BBT timeslot the event should take place in. Then, this
 * module will translate the BBT passed into an actual frame time and push the
 * message into a struct with the frame at which it should occur as a key.
 * Maybe. Another approach may be better, but my C skills are still
 * rudimentary. Maybe I should look at examples of how to create a real actual
 * buffer.

 * Anyway, after the message has been queued, the process callback will
 * actually set up the buffer, pull relevant MIDI events from the struct for
 * within nframes, and queue 'em up. Then it'll clean up the struct. Hey - how
 * do I do that?

 * QUESTION: Will this method of queing events be tolerant of a
 * fluctuating tempo? JACK transport design does not take into account variable
 * speed. Will we end up skipping frames? Will it sound like shit? TIME WILL
 * TELL
 * 
 * Actually, although I can't find much on how quickly the process cycles go, I
 * think I can just keep it to realtime MIDI messages. jack_midi_clock has some
 * logic I will steal for this. 
 */

	// but before I get started with that, let's play around with some bit
	// operations to queue up some MIDI events! yay!

	uint8_t *buffer;
	uint8_t status_byte;
	uint8_t data_byte1;
	uint8_t data_byte2;

	// let's code with the assumption of notes. 3 bytes, a status followed by two
	// data events. This does not include note-off. That will sound obnoxious.
	// But I will not think of that right now.


	//buffer = jack_midi_event_reserve(port_buf, 0, 3);
	buffer = 0; // to get it to compile, this isn't done.
	// this isn't going to work because this function can't be called back by
	// JACK. It takes different arguments because it needs to talk to Lua.


	status_byte = lua_tonumber(L, 1);
	data_byte1 = lua_tonumber(L, 2);
	data_byte2 = lua_tonumber(L, 3);
	// int data = status_byte << 16 | data_byte1 << 8 | data_byte2; // I wonder if this is right. We'll find out.
	// naw, I'll just do what the jack_midi_clock guy did.
	if(buffer) {
		buffer[0] = status_byte;
		buffer[1] = data_byte1;
		buffer[2] = data_byte2;
	}
	return 0;
}

static int process (jack_nframes_t nframes, void *arg) {

/* so I'm asking JACK to call this when needed. So this function will change
 * depending on what we're trying to do. Hmm. This stuff here from the
 * simple_client JACK example is to copy the data from the input port to the
 * output port, real simple:

	jack_default_audio_sample_t *out = (jack_default_audio_sample_t *)
	jack_port_get_buffer (output_port, nframes);
	jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port, nframes);
	memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);
	return 0;      

 * However, that's not what I have in mind right this second. Instead I have in
 * mind writing MIDI data. But again, if this is going to be a general-purpose
 * interface to JACK then I'm gonna end up with different needs depending on
 * what we're trying to do from our Lua script. When we start doing audio
 * stuff, then we're gonna end up doing those operations on the callback. And
 * I'll have to figure out if I can put that all in one function, or register
 * different ones based on what we're doing in Lua, whether we can process
 * audio and MIDI data all at once. This could get quite complex.  I'm slightly
 * overwhelmed by that, so to avoid analysis paralysis for now I'm just gonna
 * put in code that gets the MIDI part done and refactor it later.
 *
*/

  /* here's a port buffer we'll need, from midi_out */
  void* port_buf = jack_port_get_buffer(midi_out, nframes);

  /* prepare MIDI buffer */
  jack_midi_clear_buffer(port_buf);

	/* I will need to figure out how many frames nframes is, and if there will be
	 * a need for me to readahead. The Lua midi sequencer is built in such a way
	 * that it sends realtime messages by comparing what beat we're on. Perhaps
	 * an inprocess client is the way to go to circumvent that whole issue.
	 * Although I'm amidst considering another aspect of the same problem in
	 * sequencer.lua
	*/

	//process_midi_output(nframes);
  return 0;
	
}


static int showframe (lua_State *L) {
	static jack_position_t current; 
	lua_pushnumber(L, current.frame);
	return 1;

}

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
	return 1;
};

static int close_client(lua_State *L) {
	jack_client_close (client);
	return 0;
};

// register an outbound MIDI port
static int register_midi_out(lua_State *L) {
	char *port_name = lua_tostring(L,1);
	midi_out = jack_port_register(client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsTerminal, 0);
	return 0;
};

// write some data to that port. 


	
static int process_callback(lua_State *L) {
	jack_set_process_callback(client, process, 0);
	return 0;
}
	
// after ports are up and running and such, tell JACK we're ready
static int activate(lua_State *L) {
	jack_activate (client); // TODO add some error handling around this. 
	return 0;
}

static const struct luaL_Reg lua_jack [] = {
	{"showtime", showtime},
	{"showframe", showframe},
	{"open_client", open_client},
	{"close_client", close_client},
	{"register_midi_out", register_midi_out},
	{"activate", activate},
	{"process_callback", process_callback},
	{"queue_midi", queue_midi},
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
