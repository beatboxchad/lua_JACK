lua_JACK
=========================

This module exposes functions from JACK to Lua. It was originally intended as a
simple way to grab BBT information from JACK transport, then I got the hang of
this and realized it's not too useful if I don't go ahead and wire up the rest
of JACK's functionality too (now that I feel confident enough to think I
actually can). That makes this a much larger work in progress.

Current functionality includes:

- a function to become a JACK client (kinda need that)

- a function called showtime() which returns any available transport
	information (not all of it just yet, and I'll document this in clearer detail
	soon).

Planned functionality is pretty much everything else, in order of sensibility.
The ability to talk to MIDI on JACK is next, whereas the ability to become
transport master is further down. My other project which necessitates this one
is a MIDI sequencer interface (in Lua, obviously) where you live code instead
of using a traditional tracker/sequencer interface. I want it to sync up with
my loops in Sooperlooper, so this.
