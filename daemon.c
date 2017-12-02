volatile sig_atomic_t reloadLogSig = 0;
volatile sig_atomic_t terminateSig = 0;

void logsReopen(){					// logrotation can send SIGHUP to notify that log file should be reopened due it's cleaned up
	if (freopen("/var/log/loperIRCLogBot.log", "a", stdout) == NULL ){
		syslog(LOG_ERR, "Failed to reopen /var/log/loperIRCLogBot.log after SIGHUP notificatuib recieved");
		exit(EXIT_FAILURE);				
	}
	setlinebuf(stdout);						// line buffer
	if (freopen("/var/log/loperIRCLogBot.log", "a", stderr) == NULL ){
		syslog(LOG_ERR, "Failed to reopen /var/log/loperIRCLogBot.log after SIGHUP notificatuib recieved");
		exit(EXIT_FAILURE);				
	}
	setvbuf(stderr, NULL, _IONBF, 0);				// no buffering for srderr
	reloadLogSig = 0;
}

void killBySignalRecieved(irc_session_t * session){
	if (irc_cmd_quit(session, "interrupted by signal") != 0)	// "Return code 0 means the command was sent to the IRC server successfully. 
		irc_disconnect(session);				// This does not mean the operation succeed, and you need to wait for the appropriate 
	fclose(stdin);							// event or for the error code via event_numeric event."(c)docs. :wSo FUCK THIS SHIT. It would be always EOF.
	fclose(stdout);							// In case servers stuck, we would have to wait AGES before app dies. No thanks.
	fclose(stderr);
	if (remove(DEF_PID_FILE) < 0)					// delete PIDFile.
		syslog(LOG_ERR, "Can't remove PID file: /var/run/loperIRCLogBot.pid");
	syslog(LOG_NOTICE, "Interrupted by SIGTERM");			// change to LOG_NOTICE
	closelog();
	exit(EXIT_SUCCESS);
}

// Signal handler
void signalHandler (int sig){
	switch (sig){
		case SIGHUP:
			reloadLogSig = 1;
			break;
		case SIGTERM:
			terminateSig = 1;
			break;
		case SIGINT:
			_exit(2);		// TODO: document this behavior as variant of application returned values
			break;
		default:
			break;
	}
}
void daemonMode(){
	setlogmask (LOG_UPTO (LOG_ERR));
	openlog("loperIRCLogBot", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);
	
	FILE * pidFile;

	pid_t pid;

	// PERFORM FIRST FORK
	pid = fork();
	if (pid < 0) { 	
		syslog(LOG_ERR, "Failed to make first fork");
		exit(EXIT_FAILURE);				// report to log
	}
	if (pid > 0) 						// Exit parent process. pid of is not 0, then it's parent. pid = 0 then it's child.
		exit(EXIT_SUCCESS);				// report to log?
	if (chdir("/") < 0) {					// change working directory of the process to /
		syslog(LOG_ERR, "Failed to change process dir to /");
		exit(EXIT_FAILURE);				// it's safe to replace to 'return' statement
	}
	umask(0);						// set umask for using filesystem as we want
	if (setsid() < 0) {					// set SID
		syslog(LOG_ERR, "Failed to set SID");
		exit(EXIT_FAILURE);
	}
	// PERFORM SECOND FORK
	pid = fork();
	if (pid < 0){
		syslog(LOG_ERR, "Failed to make second fork");
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
		exit(EXIT_SUCCESS);

	// Save process PID to file /var/run/loperIRCLogBot.pid		// TODO: consider using of 'PIDFile' from  libutil.h 
	if ((pidFile = fopen(DEF_PID_FILE, "w")) != NULL){			// + Consider using HFS3 and /run instead of /var/run. NOTE: could be a problem for embedded system, i.e. OWRT doesn't provide this filder
		fprintf(pidFile,"%ld\n", (long) getpid());
		fclose(pidFile);
	}
	else {
		syslog(LOG_ERR, "Failed to create PID file /var/run/loperIRCLogBot.pid");	
		exit(EXIT_FAILURE);				
	}
	// #############  DEFINE SIGNAL HANDLERS #######################
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	
	sa.sa_handler = signalHandler;

	if (sigfillset(&sa.sa_mask) < 0){
		syslog(LOG_ERR, "Can't set mask of blocked signals");	
		exit(EXIT_FAILURE);
	};								// block all signals when execute
	sa.sa_flags = SA_RESTART;					// restart interrupted system calls
	
	if (sigaction(SIGHUP, &sa, NULL) < 0) {
		syslog(LOG_ERR, "Can't set sigaction() to handle application signals: SIGHUP");	
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGINT, &sa, NULL) < 0) {
		syslog(LOG_ERR, "Can't set sigaction() to handle application signals: SIGINT");	
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGTERM, &sa, NULL) < 0) {
		syslog(LOG_ERR, "Can't set sigaction() to handle application signals: SIGTERM");	
		exit(EXIT_FAILURE);
	}

	// #############   SIGNAL HANDLERS SET   #######################
	//doing it using guidelines form man daemon
	if (freopen("/dev/null", "rb", stdin) == NULL ){
		syslog(LOG_ERR, "Failed to close 'stdin'");	
		exit(EXIT_FAILURE);				
	}
	if (freopen("/var/log/loperIRCLogBot.log", "a", stdout) == NULL ){
		syslog(LOG_ERR, "Failed to redirect 'stdout' to /var/log/loperIRCLogBot.log");	
		exit(EXIT_FAILURE);				
	}
	setlinebuf(stdout);						// line buffer
	if (freopen("/var/log/loperIRCLogBot.log", "a", stderr) == NULL ){
		syslog(LOG_ERR, "Failed to redirect 'stderr' to /var/log/loperIRCLogBot.log");	
		exit(EXIT_FAILURE);				
	}
	setvbuf(stderr, NULL, _IONBF, 0);				// no buffering for srderr

	//syslog(LOG_ERR, "SUCCESS");	
	syslog(LOG_NOTICE, "loperIRCLogBot successfuly started");
}
