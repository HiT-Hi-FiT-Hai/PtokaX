PtokaX 0.4.1.1
---------------

This is PtokaX version without gui able to run from console or as windows service.
You can use for that version config files from gui version of PtokaX or you can create own config files, see examples in cfg.example directory.

Command line commands available:
-c <configdir>		- absolute path to PtokaX config directory (where will PtokaX have cfg, logs, scripts and texts directories).
-d			- run as daemon.
-h			- show help.
-v			- show PtokaX version with build date and time.

In case when that version not start for you check logs directory, in most cases it log problem.

When no config dir is specified then PtokaX running from console use actual directory, PtokaX running as daemon use current_user_home/.PtokaX !

Note: IP-to-Country database file is available on http://ip-to-country.webhosting.info/downloads/ip-to-country.csv.zip .. unpack it to configdir/cfg directory.
Note: This PtokaX use Lua 5.1.3 for scripting !
Note: This version have new scripting interface incompatible with all scripts for 0.3.6.0c and older !

Homepage: http://www.PtokaX.org

PtokaX LUA Scripts forums:
	http://luaboard.sytes.net
	http://lua.uknnet.com

PtokaX Wiki: http://wiki.ptokax.ath.cx

Please report all bugs to PPK@PtokaX.org or to me on my hub PePeK.czdc.org:4861

Latest PtokaX changelog: http://wiki.ptokax.ath.cx/doku.php?id=changelogs:ptokax:changes_after_0.330_15.31

This application uses the IP-to-Country Database
provided by WebHosting.Info (http://www.webhosting.info),
available from http://ip-to-country.webhosting.info.