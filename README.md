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
 * PID file handling
 * Loglevels
 * -HUP reload (test not done, should be done in server)
 * Implement returned exceptions (C error handling)
 * Implement innotify directory change filetable update
 * REALTIME mode implementatio				On holdback because communication
 * Preforking and other client-handling insanity
 * Innotify directory md5sum recalculation
 * Robustness improvements
 * Further testing
 * Documentation and insanity (mostly documentation)

Preforking and other client-handling insanity
---------------------------------------------
 * Corrupt file handling implementation improvement
 * Minimum server number
 * Maximum connections before quit on forked

Documentation and insanity
--------------------------
 * Document configuration files (more or less under development at the moment.)

How to use BISON Transfer in its preliminary standalone mode
------------------------------------------------------------
BISON is configured to feel a lot like a normal Linux daemon.

BISON-Transferd is the daemon that runs on the plane.
BISON-Transfer is the client that is run on the server at ground.

What does Brandon define as a normal BISON daemon?
Configuration files are stored in the `\etc` directory.
Variable data is stored in `/var`.

So how exactly does one use BISON-Transfer?

BISON-Transfer is mostly self-configuring (although BISON-Master is supposed
to do this automatically.)
If you want, it is possible to supplement your own configuration by changing
the configuration file that results out of the self-configuration phase from
one run of the server.  (Or by modifying a previously generated configuration
file)

The configuration variables, in Brandon's opinion, are aptly named and should
be fairly intuitive.

In order to queue files to be sent to the destination server, one must put
files into the transmit directory.  This is the same directory that is defined
in the configuration file.

It is possible for multiple recievers to connect to one sender, and it is only
possible for multiple senders to connect to one reciever if the file names
within the transmit directory do not coincide.  Having files with the same
name within two different transitters to the same recieve directory will
result in a condition where one file continually does not synchronize
correctly.
