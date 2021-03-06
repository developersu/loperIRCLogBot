/***********************************************************************************
 * Author: Dmitry Isaenko                                                          *
 * License: GNU GPL v.3                                                            *
 * Version: 1.5                                                                    *
 * Site: https://developersu.blogspot.com/                                         *
 * 2017, Russia                                                                    *
 ***********************************************************************************/

#include "libircclient/libircclient.h"
#include "libircclient/libirc_rfcnumeric.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>	// only to get PATH_MAX
#include <unistd.h>	// for using setsid. 
#include <syslog.h>	// Use syslog
#include <signal.h>	// Use signals
#include <sys/select.h>	// Use pselect

#include "defined_values.h"
#include "daemon.c"
#include "config.c"	// see load_config and typedef of configuration structure in here

// TODO make it multi-channel

int createDir(char * logdir, char * newDirName){
	struct stat st = {0};
	
	char dir[PATH_MAX];
	
	strcpy(dir, logdir);
	strcat(dir, newDirName);		// add bot.conf to path-to-executable

	if (stat(dir, &st) == -1) {
		if (mkdir(dir, 0777) != 0 )
			return 1;
	}
	return 0;
}

// Function to print time stamps when writing to console
const char * printTimeStamp(){			// const function, static array.. one say this function should be replaced to something better. It's make me sad.
	static char timeStamp[22];
	time_t now = time(NULL);

	strftime(timeStamp, 22, "%d %b %Y %X", localtime(&now));
	return timeStamp;
}

// Mainly used to update is_alive
void  event_ctcp_rep(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count){	// handle own ctcp requests - get recieved data and do something with it
	configuration *C = load_config(RETURN_STRUCT, "");		//load config to get current nickname

	#ifdef PING_DEBUG
	printf("\nReply from my CTCP, where USER = %s\n", origin);
	#endif
	if (strncmp(origin, C->nick, strlen(C->nick)) == 0){		// if bot generated response, then it's ping for internal use. (Ok, documentation is 80% clear on this, it will work but in future should be checked)
		is_alive(1, time(NULL), session);
	}
}

/* Handle timeouts before re-connect (rewrite in January 2038)
 state = 0 - get status
 state = 1 - write status

 returns 0 - OK
 returns 1 - not OK
*/
int is_alive( int state, time_t timeGot, irc_session_t * session){				// in future, it should be stand-alone thread
	static time_t timeSav;	
	static int pingRequestSent = 0;			// 0 - not sent, 1 - already sent
	configuration *C = load_config(RETURN_STRUCT, "");		//load config to get current nickname

	#ifdef PING_DEBUG
	printf("is_alive:ping = %d\n", pingRequestSent);
	#endif

	if (state == 0)	{	
		if ( ( timeGot - timeSav) > TIME_TO_SEND_PING ) {	// ok, it's time to send PING to yourself or probably we have problems
			if (pingRequestSent == 0){			// if we didn't sent PING request before, let's do it and change pingRequestSent
				#ifdef PING_DEBUG
				printf("PANIC - Request sent\n");
				printf("  stored time   = %ld\n",timeSav);
				printf("  recieved time = %ld\n",timeGot);
				#endif
				irc_cmd_ctcp_request(session, C->nick, "PING");
				pingRequestSent = 1;
				return 0;
			}
			else{						// we already sent PING
				if (( timeGot - timeSav) > (TIME_TO_SEND_PING+TIME_TO_WAIT_PING)){	// we know, that the time is greater than stored one, so we add 3min(180) and if it's greater, then re-connect
					#ifdef PING_DEBUG
					printf("Now we have problems\n");
					#endif
					return 1;
				}
				else 
					return 0;
			}
		}
	}
	else if ( state == 1 ){
		timeSav = timeGot;				// re-write stored time
		pingRequestSent = 0;					// reset pingRequestSent
		#ifdef PING_DEBUG
		printf("\n1-WRITE event\nTime Saved (re-written)       = %ld\n", timeSav);					//debug
		printf("TIME_TO_SEND_PING            	= %d\n", TIME_TO_SEND_PING);						//debug
		#endif
		return 0;
	}
	return 0;		// how it could be possible to come here? ok, whatever
}
void event_connect (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{	
	is_alive(1, time(NULL), session );				// this time really 'initial' timestamp written into the watchdog function

	configuration *C = load_config(RETURN_STRUCT, "");				// dump_event (session, event, origin, params, count);
	#ifdef X_MODE
	irc_cmd_user_mode (session, "+x");			// TODO clarify this shit
	#endif
	irc_cmd_join (session, C->channel, 0);
}

void dump_event (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{									// TODO: Re-write. Actually, this implementation is also pice of shit
#ifdef DEBUG								// It cause too many system calls even we don't need them
	char buf[512];
	int cnt;

	buf[0] = '\0';

	for ( cnt = 0; cnt < count; cnt++ ) {
		if ( cnt )
			strcat (buf, "|");
		strcat (buf, params[cnt]);
	
	}
#endif
	static time_t floodTrackTime = 0;
	char nowTime[20];
	char nickBuf[128];
	char hostBuf[1024];
	time_t now = time(NULL);
	char realLogPath[PATH_MAX];

	FILE * fp;		// set FD for the file for logs
	
	configuration *C = load_config(RETURN_STRUCT, "");
	
	// Set name for the log file
	irc_target_get_nick (origin, nickBuf, 128);
	irc_target_get_host (origin, hostBuf, 1024);	
	
	strftime(nowTime, 20, "logs/%Y-%m-%d.txt", localtime(&now));
	strcpy(realLogPath, C->logPath);
	strcat(realLogPath, nowTime);

 	if ( (fp = fopen (realLogPath, "ab")) != NULL )
	{
		strftime(nowTime,20, "[%H:%M:%S] ", localtime(&now));

		!(strcmp(event,"CHANNEL")) ? fprintf(fp,"%s <%s>: %s\n",nowTime, nickBuf, params[1]): 
		!(strcmp(event,"ACTION"))  ? fprintf(fp,"%s %s %s\n",nowTime, nickBuf, params[1]): 
		!(strcmp(event,"JOIN"))    ? fprintf(fp,"%s >>  %s [%s] joined %s\n",nowTime, nickBuf, hostBuf, params[0]): 
		!(strcmp(event,"INVITE"))  ? fprintf(fp,"%s %s invites %s to %s\n",nowTime, nickBuf, params[0], params[1]): 
		!(strcmp(event,"KICK"))    ? fprintf(fp,"%s !<< %s kicked by %s [%s] with reason: %s\n",nowTime, params[1], nickBuf, hostBuf, params[2]):
		!(strcmp(event,"MODE"))    ? strcmp(nickBuf, hostBuf) == 0 ? 0: fprintf(fp,"%s -!- %s [%s] set %s %s\n",nowTime, nickBuf, hostBuf, params[1], params[2] ? params[2]:""): 
		!(strcmp(event,"PART"))    ? fprintf(fp,"%s <<  %s [%s] parted: %s \n",nowTime, nickBuf, hostBuf, params[1]): 
		!(strcmp(event,"TOPIC"))   ? fprintf(fp,"%s -!- %s [%s] has changed topic to: %s \n",nowTime, nickBuf, hostBuf, params[1]): 
		!(strcmp(event,"QUIT"))    ? fprintf(fp,"%s <<  %s [%s] quit: %s \n",nowTime, nickBuf, hostBuf, params[0]): 
		!(strcmp(event,"NICK"))    ? fprintf(fp,"%s -!- %s [%s] changed nick to: %s \n",nowTime, nickBuf, hostBuf, params[0]): 0;

		if (!strcmp(event,"KICK")){
		// Reloading configuration in case our nick has been changed recently
			if ( !(strcmp(params[1], C->nick)) && C->reJoin == 1 ){
				event_connect (session, event, origin, params, count);
			}
		}
		else if (!strcmp(event, "CHANNEL"))					// works like shit. Should be separated thread
			if (strncmp(params[1], C->nick, strlen(C->nick)) == 0 )	
				if ((now - floodTrackTime) >= C->floodTimeOut){
					irc_cmd_msg(session, params[0], C->link);
					floodTrackTime = now;
					fprintf(fp,"%s <%s>: %s\n",nowTime, C->nick, C->link);
				}

		fclose (fp);
	}
	else {
		printf("Unable to open/create log file: check folder permissions\n");			// and die, but it doesn't die.. well. Ok for now.
		exit(EXIT_FAILURE);									// TODO: QA: integration testing needed
	}

	#if defined (DEBUG)	
	printf ("Event \"%s\", origin: \"%s\", params: %d [%s]\n", event, origin ? origin : "NULL", cnt, buf);
	#endif
}

void event_ctcp_req(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	dump_event (session, event, origin, params, count);

	time_t now = time(NULL);
	char curTime[138];
	strftime(curTime, 138, "TIME %a %b %d %H:%M:%S %Z %Y", localtime(&now));

	if( strcmp (params[0], "VERSION") == 0 ){
		dump_event (session, event, origin, params, count);
		irc_cmd_ctcp_reply(session, origin, "VERSION loperIRCLogBot v."__CUR_VER__);
	}
	else if( strcmp (params[0], "SOURCE") == 0 ){
		dump_event (session, event, origin, params, count);
		irc_cmd_ctcp_reply(session, origin, "SOURCE loperIRCLogBot v."__CUR_VER__);
	}
	else if( strcmp (params[0], "TIME") == 0 ){
		dump_event (session, event, origin, params, count);
		irc_cmd_ctcp_reply(session, origin, curTime);
	}
	else if( strncmp(params[0], "PING", 4) == 0 ){				
		dump_event (session, event, origin, params, count);
		irc_cmd_ctcp_reply (session, origin, params[0]);
	}
	else if( strcmp (params[0], "CLIENTINFO") == 0 ){
		dump_event (session, event, origin, params, count);
		irc_cmd_ctcp_reply(session, origin, "CLIENTINFO loperIRCLogBot v."__CUR_VER__" - Supported tags: VERSION, SOURCE, TIME, PING, CLIENTINFO");
	}
}

void event_join (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	dump_event (session, event, origin, params, count);
	irc_cmd_user_mode (session, "+i");
}

void nick_change(irc_session_t * session){		
	static char append[] = "|0";
	configuration *C = load_config(RETURN_STRUCT, "");
	
	if ( append[1] > '9')				// Refactoring request
		append[1] = '0';			// if all your nicknames with |0 |2 ... |9 are already occupied, we're fucked up. Really, there is no good solution in this code.
	
	// Let's say that we have string like 1234567|0, and maxNickLength = 10, then we see, that string length is greater (9) then maxNickLength-2, but also we've already appended something like "|0" into it, then
	// we could check it and wipe. Otherwise we wipe 2 last chars.
	if ( strlen(C->nick) > (C->maxNickLength - 2) ){
		if ( (C->nick[C->maxNickLength-3] == '|') && (C->nick[C->maxNickLength-2] >= '0') && (C->nick[C->maxNickLength-2] <= '9') )
			C->nick[C->maxNickLength-3] = '\0';
		else
			C->nick[C->maxNickLength-2] = '\0';
	}
	if ( C->nick[strlen(C->nick)-2] == '|' )
		C->nick[strlen(C->nick)-1] = append[1];
	else
		strncat(C->nick, append, 2);
	
	append[1]++;

	C = load_config(NICK_CHANGE, C->nick);			

	irc_cmd_nick(session, C->nick);
}

void event_numeric (irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count)
{
	char buf[24];
	sprintf (buf, "%d", event);
	
	if (event == 433){			// 433	Nickname is already in use
		nick_change(session);
	}

	dump_event (session, buf, origin, params, count);
}
static void event_notice (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	char nickBuf[1024];		// too big??
	configuration *C = load_config(RETURN_STRUCT,"");

	dump_event (session, event, origin, params, count);

	if ( !origin )
                return;
	else
		irc_target_get_nick (origin, nickBuf, 1024);	
		
	if ( strcasecmp (nickBuf, "nickserv") )
		return;
	if ( (strstr (params[1], "This nickname is registered") != NULL) || (strstr (params[1], "This nickname is owned by someone else") != NULL) ){
		irc_send_raw(session, "nickserv IDENTIFY %s", C->password);
	}
	else if( strstr (params[1], "Password accepted") != NULL )
		printf ("Nickserv authentication succeeded\n");

}

int makeTemplate(char * folder){
	FILE * templ;
	strcat (folder, "bot.conf");

	printf("Attention! This action will remove your current bot.conf file\n"
		"Where do you want to generate template?:\n"
		"(1) This folder\n"
		"(2) /etc/loperIRCLogBot/\n"
		"(A) Abort\n");
	do{
		switch (getchar()){
			case '1':  
				if ( (templ = fopen(folder,"w")) != NULL) {
					fprintf(templ, DEF_DEFAULT_TEMPLATE);
					fclose(templ);
					printf("Configuratation template created.\n");
					return 0;
				}
				else {
					printf("Unable to create configuration template file\n"
						"Check folder permissions\n");
					return 1;
				}
			case '2':
				if (createDir("/etc/loperIRCLogBot/","") != 0){
					printf("Unable to create diretory /etc/loperIRCLogBot\n"
						"Make sure that you have permissions to write there\n");
					return 1;
				}
				if ( (templ = fopen("/etc/loperIRCLogBot/bot.conf","w")) != NULL) {
					fprintf(templ, DEF_DEFAULT_TEMPLATE);
					fclose(templ);
					printf("Configuratation template created.\n");
					return 0;
				}
				else {
					printf("Unable to create configuration template file\n"
						"Check folder permissions\n");
					return 1;
				}
			case 'A': case 'a': 
				printf("No changes applied\n");                   
				return 0;
			case '\n':                                      //     ,__o
				break;                                  //   _-\_<, 
			default:                                        //  (*)/'(*)
			 	while ( getchar() != '\n' );		
		}
		
		printf("\033[A\033[2K");				// VT100 escape codes

	} while(1);
}

void reportSettingsIssues(int sum){
	char *setLst[] = {"server", 
			"channel", 
			"port", 
			"nick", 
			"username", 
			"realname", 
			"password", 
			"maxNickLenght", 
			"logPath", 
			"link", 
			"reJoin",
			"floodTimeOut"};
	int i;
	
	switch (sum){
		case -1:
			printf("Configuration file issue: value or attribute missed or redundant\n");
			break;
		case -2:
			printf ("Unable to open configuration files. Please make sure that they exist at one of the next folders\n"
				"/etc/loperIRCLogBot/bot.conf\n"
				"./bot.conf\n"
				"Please note, that configuration file shouldn't be lager then 15 lines.\n"
				"If files exists, please check that user have read permissions for configuraion file\n");
			break;
		case -3:
			printf("Configuration file issue: 'logPath' is not defined as absolute path\n");
			break;
		case -4:
			printf("Unable to allocate memory.\n");
			break;
		case -5:
			printf("Internal error.\n");
			break;
		default:
			printf("Configuration file issue. Next field(s) not found:\n");
			for (i=0; i<12; i++)
				if ((sum & (1<<i)) != (1<<i))
					printf("\t'%s'\n", setLst[i]);
	}
}

void silentMode(){
	// close stdin, redirect stdout & stderr
	if ( freopen ("output.txt", "a", stdout) == NULL )		// todo - add validation for errno? Write to syslog?
		exit(EXIT_FAILURE);
	setlinebuf(stdout);						// line buffer
	if ( freopen ("output.txt", "a", stderr) == NULL )		// todo - add validation for errno?
		exit(EXIT_FAILURE);
	setvbuf(stderr, NULL, _IONBF, 0);				// no buffering for srderr
	if ( freopen("/dev/null", "rb", stdin) == NULL )
		exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	configuration *config;
	
	irc_callbacks_t callbacks;	// The IRC callbacks structure
	irc_session_t * session;
	struct timespec ts;		// time structure for pselect();
	fd_set in_set, out_set;
	int maxfd = 0;
	 
	// ### daemon mode tails.
	// Define signals that we will block while using pselect(); See below. pselect set mask provided, then change it to default
	sigset_t blockedSignals;	// block all signals; to use in pselect()
	sigset_t normalSignals;		// define default mask
	sigfillset(&blockedSignals);
	sigdelset(&blockedSignals, SIGINT);
	
	sigfillset(&normalSignals);				// Not sure that it could make impact on anything
	sigprocmask(SIG_UNBLOCK, &normalSignals, NULL);
	// ###

	// Let's find out the path to executable file! It's needed, because when we create config file like ../executable -g 
	// it creates it on the same folder where user located at, not at the folder where the executable is.
	// Same for logs.
	char dest[PATH_MAX];
	struct stat info;
	if (readlink("/proc/self/exe", dest, PATH_MAX) == -1){
		printf("Unable to get the path to this executable\n");
		return 1;
	}
	else {
		dest[strlen(dest)-strlen(strrchr(dest,'/')+1)] = '\0';		// looks for the last "/" at the path etc.
	}
	// end detection
	if (argc == 2){
		if ( strcmp("--help", argv[1]) == 0 ){
			printf(DEF_HELP_MSG);
			return 0;
		}
		else if( (strcmp("-g", argv[1]) == 0) || (strcmp("--genconf", argv[1]) == 0)) {
			return makeTemplate(dest);
		}
		// deamon mode here
		else if( (strcmp("-d", argv[1]) == 0) || (strcmp("--daemon", argv[1]) == 0)) {
			daemonMode();
		}
		else if( (strcmp("-s", argv[1]) == 0) || (strcmp("--silent", argv[1]) == 0)) {
			silentMode();
		}
		else if ( (strcmp("-v", argv[1]) == 0) ||  (strcmp("--version", argv[1]) == 0)) {
			printf("loperIRCLogBot: "__CUR_VER__" - build "__TIME__" "__DATE__"\n");
			return 0;
		}
		else {
			printf("Incorrect arguments.\n"
				"Use --help for information.\n");
			return 0;
		}
	}
	else if (argc > 2){
		printf("Too many arguments.\n"
			"Use --help for information.\n");
		return 0;
	}
//	-================================================================================================-
	config = load_config(CONFIG_INIT, dest);
	if (config->status == 0) {
		printf("\t--------------------\n"
			"\t%s\n"
			"\t\tSETTINGS\n"
			"\t--------------------\n"
			"server          - %s\n"
			"channel         - %s\n"
			"port            - %d\n"
			"nick            - %s\n"
			"username        - %s\n"
			"realname        - %s\n"
			"password        - %s\n"
			"maxNickLength   - %d\n"
			"logPath         - %s\n"
			"link            - %s\n"
			"reJoin          - %s\n"
			"floodTimeOut    - %d\n"
			"\t--------\n", 
			printTimeStamp(),
			config->server, 
			config->channel, 
			config->port, 
			config->nick, 
			config->username, 
			config->realname, 
			config->password, 
			config->maxNickLength, 
			config->logPath, 
			config->link, 
			config->reJoin == 0?"No":"Yes",
			config->floodTimeOut);
		if ( createDir(config->logPath, "logs") != 0){				// set logs directory
			printf ("Unable to create directory for log files (%s)\n"
				"Please make sure that you have premissions to write in this directory\n", 
				config->logPath);
			return 1;
		}
		else {
			// Init callbacks
			memset ( &callbacks, 0, sizeof(callbacks) );
        		
			// Set up the mandatory events
			callbacks.event_connect = event_connect;
			callbacks.event_numeric = event_numeric;
			callbacks.event_join = event_join;
        		
			// Set up the rest of events
			
			callbacks.event_ctcp_req = event_ctcp_req;
			callbacks.event_notice = event_notice;
			callbacks.event_ctcp_rep = event_ctcp_rep;	// &  dump_event? Now no need to dump.	This event is triggered upon receipt of an CTCP response. 
									//Thus if you generate the CTCP message and the remote user responded, this event handler will be called.
			//callbacks.event_unknown = event_unknown;	// this event will be triggered when app recieve unknown request from server. 
        		
			callbacks.event_channel = dump_event;
			callbacks.event_nick = dump_event;
			callbacks.event_quit = dump_event;
			callbacks.event_part = dump_event;
			callbacks.event_mode = dump_event;
			callbacks.event_topic = dump_event;
			callbacks.event_kick = dump_event;
			callbacks.event_invite = dump_event;
			callbacks.event_umode = dump_event;
			callbacks.event_ctcp_action = dump_event;	// "ACTION" handler
        		
			// Now create the session
			while (1) {
				printf("%s Connecting...\n", printTimeStamp());
				session = irc_create_session( &callbacks );
				
	        		if ( !session ){
					printf("%s Unable to create session\n", printTimeStamp());
					// return 1; 							// give a try once again
				}
				else {
		                	irc_option_set( session, LIBIRC_OPTION_SSL_NO_VERIFY );
				
	        			// Connect to a regular IRC server
					if ( irc_connect (session, config->server, config->port, 0, config->nick, config->username, config->realname ) ){
						
						printf ("%s Could not connect: %s\n", printTimeStamp(), irc_strerror (irc_errno(session)));
						// return 1;						//give a try once again
						// TODO add a counter for few reply and die / do something with this
		                	}
					else {	

						is_alive(1, time(NULL), session);	//initialize is_alive internal start time
						printf("%s Connection established\n", printTimeStamp());
		
						///////////////////// irc_run() replacement  ///////////////////
						while ( irc_is_connected(session) )
						{
							if (reloadLogSig == 1)		//debugFunction
								logsReopen();
							if (terminateSig == 1)		//debugFunction
								killBySignalRecieved(session);

							ts.tv_sec = 0;
							ts.tv_nsec = 250000000;
		        
							// Init sets
							FD_ZERO (&in_set);
							FD_ZERO (&out_set);
		        
							// +1 chk
							if ( is_alive(0, time(NULL), session) != 0 ){
								printf("\n\t-----------------------------------\n\t"
									"%s PING timeout\n"
									"\t-----------------------------------\n", 
									printTimeStamp());
                                                                 break; // 1
							}
							// end +1 chk

							
							irc_add_select_descriptors (session, &in_set, &out_set, &maxfd);
							// Set mask to all signals, while neccessary for us are: SIGHUP, SIGTERM. Exclude SIGINT, 'cause no matter how this app fucks up in case of _exit(2).
							if ( pselect (maxfd + 1, &in_set, &out_set, 0, &ts, &blockedSignals) < 0 )		// and errno == EINTR !
							{
								printf ("%s Could not connect or I/O error: LIBIRC_ERR_TERMINATED\n", printTimeStamp());
								break; // 1
							}
					
							if ( irc_process_select_descriptors (session, &in_set, &out_set) ){
								printf ("%s Could not connect or I/O error\n", printTimeStamp());
								break;	// 1
							}
		
						}
						irc_disconnect(session);
						printf("%s Connection lost\n", printTimeStamp());
						//return 0;
						////////////////////////////////////////////////////////////////////////////
					}
				}
				printf ("%s Not connected: will retry in 5 sec\n", printTimeStamp());
				sleep (5);
			} // while end
		}
		return 0; // never happens
	}
	else {
		reportSettingsIssues(config->status);
		return 1;
	}
	return 0;
}
