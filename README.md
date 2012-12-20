TARDIS
======

Time tracking for Time Lords!

[![Build Status](https://secure.travis-ci.org/wlangstroth/tardis.png)](http://travis-ci.org/wlangstroth/tardis)

Or people who spend most of their time on the command line, and always forget
to open that browser window, sign in, bugger about with the mouse until
some drop-down gives you the correct time, click on the project, find the
activity, select the ... oh, screw it! I'm writing my own!

Note: this really is for Time Lords. There isn't any serious error handling, and
if you screw up, your best bet is to fix things with sql commands.


Usage
-----

To create a new entry,

    $ tardis start project "a description of the project"

To stop for the day,

    $ tardis stop

You can also shorten "start" to "s", like so:

    $ tardis s another-project "this is a different project"

Other subcommands:

    $ tardis all

shows you all of your entries in an attractive format.

    $ tardis r[eport]

shows you the amount of time you've spent on different projects.

    $ tardis b[ackup]

backs up your sqlite database file to ~/.tardis/todays-date.db

    $ tardis end 3 "2012-05-11 09:00:00"

amends the entry with id 3 to end at 9:00 in the morning.


Database
--------

The SQLite database file is located under your home directory, in the ~/.tardis
directory, as `current.db`.


Dependencies
------------

* sqlite3


Licence
-------

MIT
