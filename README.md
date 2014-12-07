Brandon Branch Info
===================
This is a large rewrite of the server for a large amount of forking and
increased reliability of the entire system.  Bear with it for now.

I am rewriting a large portion of the codebase in my branch because it is
necessary in order to completely transition over into C++ from the original
C server.  Although it was the original design intent for the server to be in
C in order to make the server more portable, it has become massively easier 
to develop in C++ -- especially with the advent of the Boost:: C++ libraries.

Beautiful C is perhaps easier than creating beautiful C++.  It is hard to
create beautiful C++ because there are so many concepts to integrate into the
code at the same time, and syntax and semantics takes up most of the code.
(Albeit, the code has less lines to debug.)

BISON-Transfer
==============

This is the transfer server in the series of BISON UAS products made by the
communications team (HAN(Hardened Antenna Network)) of UCR-UAS.

Unfortunately, the software right now is a little divided and quite incomplete.
And this is to be expected until the first version comes out.

Goals
-----

The general goals of BISON-Transfer are quite simple:
 * Reliability
 * Flexibility
 * "Hardness" (There's a reason we're called the Hardened Antenna Network)
 * Ease of configuration

General Implementation
----------------------

Because of the general rules that we would like to abide by, the entire system
is implemented in a rather insane fashion.

There are a few daemons that should be running around:
 * BISON-Configuration -- Configuration management daemon (including reloading)
 * BISON-Transfer -- File transfer daemon
 * BISON-View -- Userspace (except for the server) interaction daemon

And all of these different daemons will have different ways of talking with
each other.  Sorry.

The first to start up will be the configuration daemon.  The rest of the daemons
can run without a configuration daemon, but the configuration will be changing
throughout the flight, and configuration file initialization cannot be completed
without either a sample configuration or the assistance of the configuration
daemon.  (For right now, the transfer daemons will run fine without the
configuration daemon, and will soon be capable of self-generating configuration
files)

The second to start up will be the Transfer daemons (server and client).  The
server will be reloaded (with a -HUP signal) from the configuration
daemon after the configuration file is reloaded from the daemon's commands.
The client, however, will only need to be reloaded on server address changes
(which should never happen!).

The third to start up will be the View client.  The Flight Operator will have
permission to edit the imaging mode.  Doing so will initiate a TCP connection
with the configuration daemon to regenerate configuration files for the server.
After the configuration daemon is done regenerating the files, it will tell the
transfer server daemon to reload its files.

TODO List (by order of implementation)
---------------------------------------
 * Communication model re-creation
 * REALTIME mode implementation
 * Preforking and other client-handling insanity
 * -HUP reload (test not done, should be done in server)
 * Robustness audit
 * Robustness improvements
 * Loglevel improments
 * Further testing
 * Documentation and insanity (mostly documentation)

Preforking and other client-handling insanity
---------------------------------------------
 * Shared filetable memory
 * Minimum server number
 * Move filetable handling to client
 * Maximum connections before quit on forked
