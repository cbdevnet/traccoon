traccoon is a lightweight multi-threaded BitTorrent tracker implementation,
using an SQLite database as backing store.

The latest development snapshot may be accessed via cloning
https://github.com/cbdevnet/traccoon. Should that location become unavailable in
the future, a new location will be announced somewhere on https://dev.cbcdn.com/.

Release tarballs may be downloaded from the tag overview page.

Build prerequisites:
	libpthread
	libsqlite3-dev

Build by running 'make' or, alternatively, compiling traccoon.c by hand.
Verbosity may be controlled by editing traccoon.h 
 (eg. having DEBUG(a) expand to nothing, thus disabling DEBUG level output)

Setup instructions
	Run 
	 ./traccoon -h
	in order to see available options.

	Create a compatible SQLite backing file with the -c option.
	Optionally edit the database structure to suit your use case
	 (do not remove or rename any columns, tables or views).
	
	Run traccoon with the -f option in order to use the created file.


This distribution includes the following supporting tools:
	torrentinfo
		Utility for reading, analyzing and parsing .torrent files.
		Windows builds are supported.
		Build Prerequisites:
			libssl-dev

	getpeers
		Utility for querying trackers for BitTorrent peers and analyzing
		Tracker communication.
		Windows builds are supported.
		Build prerequisites:
			libssl-dev

	For more information, see the respective folders.
