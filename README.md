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
 * Generated MD5 parsing (almost implemented.)
 * Client code cleanup
 * Better client housekeeping (circular buffers and insanity, anyone?)
 * -HUP reload (test not done, should be done in server)
