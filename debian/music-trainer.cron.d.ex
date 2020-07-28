#
# Regular cron jobs for the music-trainer package
#
0 4	* * *	root	[ -x /usr/bin/music-trainer_maintenance ] && /usr/bin/music-trainer_maintenance
