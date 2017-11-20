/***********************************************************************************
 * Author: Dmitry Isaenko                                                          *
 * License: GNU GPL v.3                                                            *
 * Version: 1.2                                                                    *
 * Site: https://developersu.blogspot.com/                                         *
 * 2017, Russia                                                                    *
 ***********************************************************************************/

#include "libircclient/libircclient.h"
#include "libircclient/libirc_rfcnumeric.h"
#include "stdio.h"
#include "string.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
// only to get PATH_MAX
#include <limits.h>

//define period before sending PING to server (5 min - 300).
#define TIME_TO_PING	300

// set if you want to turn mode +x on while connecting
#define X_MODE

#define NPING_DEBUG
#define NDEBUG
#define NDEBUG_LOG

#ifndef __TIME__
#define __TIME__ "-"
#endif
#ifndef __DATE__
#define __DATE__ "-"
#endif


// TODO normal deamon-mode functionality
// TODO make it multi-channel
// TODO-feachure make an ability to define log path for the bot.conf? Or move it to /etc/

typedef struct conf_sruct {
        int status;
        char server[2048];
        char channel[2048];
        int port;
        char nick[2048];
        char username[2048];
        char realname[2048];
        char password[2048];
	int maxNickLength;
	char logPath[PATH_MAX];				
	char link[2048];				// should be enough to store link
	int reJoin;					// 1 - yes, 0 - no
} configuration;


configuration load_config(int run, char * nick_or_path) {	
/*
 * if run = 0 we load config from the file
 * if run = 1 we return configuration structure stored in this function
 * if run = 2 we overrite 'nick' at the configuration structure
 */
	int i;

	FILE * conf;
	
	static char fileArray[15][2048];	// user setup stored here and avaliable by any call after initial (by passing 0 to function)
	char * st;
	char sta[15][2][2048];			// should be 3D
	static configuration C;			//returning structure
	
	char confFilePath[PATH_MAX];

	int checksum = 0;
	
	if (run == 0) {
	//	the first call
		strcpy(confFilePath, nick_or_path);
		strcat(confFilePath, "bot.conf");		// add bot.conf to path-to-executable

		if ( (conf = fopen(confFilePath,"r")) != NULL) {
			for (i=0;(!feof(conf));i++){
				fgets(fileArray[i], sizeof(fileArray[i]), conf);
				if (i>=14)		// 15 lines is max size for config file
					break;
			}

		fclose(conf);

		#ifdef DEBUG_LOG	
		printf("___Content of the file___\n");
		for (i=0;i<15; i++){
			printf("%s", fileArray[i]);
		}
		printf("______________________\n");
		#endif

		// Parsing configuration file			- IF STRING DOESN'T HAVE ":" - SKIP IT
		for (i=0;i<15;i++){
			if ( strstr(fileArray[i], ":") != NULL ) {    
				st = strtok(fileArray[i], ": \t\n");	// if we can't get ANY parameter at the string, that have ":" inside, then skip this string
				if (st != NULL){			
					strcpy(sta[i][0],st);
	
					st = strtok(NULL, ": \t\n");

					if (st == NULL){		// if we had a first parameter in string, (no matter was it set before ":" or after) and don't have the second one, 
						C.status = -1;		// then we ruin the chain and returning error. 'Status' of incorrect config-file = "-1"
						return C;
					} 
					else {
						strcpy(sta[i][1], st);					
	
						st = strtok(NULL, ": \t\n");	// generally to parse links
						while (st != NULL){
							strcat(sta[i][1], ":");
							strcat(sta[i][1], st);

							st = strtok(NULL, ": \t\n");
						}
					}
				}
			}
		}


		for (i=0;i<15;i++){
			if ( strstr(sta[i][0], "server") != NULL )
				checksum |= 1<<0;
			else if ( strstr(sta[i][0], "channel") != NULL )
				checksum |= 1<<1;
			else if ( strstr(sta[i][0], "port") != NULL )
				checksum |= 1<<2;
			else if ( strstr(sta[i][0], "nick") != NULL )
				checksum |= 1<<3;
			else if ( strstr(sta[i][0], "username") != NULL )
				checksum |= 1<<4;
			else if ( strstr(sta[i][0], "realname") != NULL )
				checksum |= 1<<5;
			else if ( strstr(sta[i][0], "password") != NULL )
				checksum |= 1<<6;
			else if ( strstr(sta[i][0], "maxNickLength") != NULL )
				checksum |= 1<<7;
			else if ( strstr(sta[i][0], "logPath") != NULL )
				checksum |= 1<<8;
			else if ( strstr(sta[i][0], "link") != NULL )
				checksum |= 1<<9;
			else if ( strstr(sta[i][0], "reJoin") != NULL )
				checksum |= 1<<10;
				
		}

		if (checksum != 0b11111111111){
			C.status = checksum;			// incorrect number of settings defined
			return C;
			}
		else {
			// Format array for return in case we're all good
	
			C.status = 0;				//  OK = 0.
	
			for (i=0; i<15; i++){
				if ((strcmp(sta[i][0], "server")) == 0)
					strcpy(C.server, sta[i][1]);
				else if ( (strcmp(sta[i][0], "port")) == 0)
					C.port = atoi(sta[i][1]);
				else if ( (strcmp(sta[i][0], "channel")) == 0)
					strcpy(C.channel, sta[i][1]);
				else if ( (strcmp(sta[i][0], "nick")) == 0)
					strcpy(C.nick, sta[i][1]);
				else if ( (strcmp(sta[i][0], "username")) == 0)
					strcpy(C.username, sta[i][1]);
				else if ( (strcmp(sta[i][0], "realname")) == 0)
					strcpy(C.realname, sta[i][1]);
				else if ( (strcmp(sta[i][0], "password")) == 0)
					strcpy(C.password, sta[i][1]);
				else if ( (strcmp(sta[i][0], "maxNickLength")) == 0){
					C.maxNickLength = atoi(sta[i][1]);
					if (C.maxNickLength > 128){
						C.maxNickLength = 128;			// now idiots could feel themselfs protected. Libircclient restriction IIRC set to 128 chars
					}
				}
				else if ( (strcmp(sta[i][0], "logPath")) == 0){
					if (strcmp(sta[i][1], "0") == 0)
						strcpy(C.logPath, nick_or_path);
					else {
						if (sta[i][1][0] != '/'){
							C.status = -3;
							return C;
						}
						else{
							strcpy(C.logPath, sta[i][1]);
							if ( C.logPath[strlen(C.logPath)] != '/' )
								strcat (C.logPath, "/");
						}
					}
				}
				else if ( (strcmp(sta[i][0], "link")) == 0){
					strcpy(C.link, "Logs: ");
					strcat(C.link, sta[i][1]);
				}
				else if ( (strcmp(sta[i][0], "reJoin")) == 0){
					if (strcmp(sta[i][1], "yes") == 0 || strcmp(sta[i][1], "Yes") == 0 )
						C.reJoin = 1;
					else
						C.reJoin = 0;
				}
			}
			if (strlen(C.nick) > C.maxNickLength)
				C.nick[C.maxNickLength] = '\0';				// yeah, they will love it, before set nick name longer then 128 char =(
		}
		// ++++++++++++++++++++++++++++++++++++++++++++++
	
		}
		else {

			C.status = -2;			//unable to open file = -2
			return C;
		}
	
		#ifdef DEBUG_LOG
		printf("___Recieved keys from the file___\n");
		for (i=0;i<15; i++){
			printf("%s - %s\n", sta[i][0], sta[i][1]);
		}
		printf("______________________\n");
		#endif

		return C;
	}
	else if ( run == 1 ){
		return C;				// just return already loaded structure by request
	}
	else if ( run == 2){				// save nick recieved
		strcpy(C.nick, nick_or_path);
		return C;
	}
		
	
}

int set_log_dir(char * logdir){
	struct stat st = {0};
	
	strcat(logdir, "logs");

	if (stat(logdir, &st) == -1) {
		if (mkdir(logdir, 0777) != 0 )
			return 1;
	}
	return 0;
}

// Function to print time stamps when writing to console
void printTimeStamp(){
	char timeStamp[22];
	time_t now = time(NULL);

	strftime(timeStamp, 22, "%d %b %Y %X ", localtime(&now));
	printf("%s", timeStamp);
}

// Mainly used to update is_alive
void  event_ctcp_rep(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count){	// handle own ctcp requests - get recieved data and do something with it
	configuration C = load_config(1, "");		//load config to get current nickname

	#ifdef PING_DEBUG
	printf("\nReply from my CTCP, where USER = %s\n", origin);
	#endif
	if (strncmp(origin, C.nick, strlen(C.nick)) == 0){		// if bot generated response, then it's ping for internal use. (Ok, documentation is 80% clear on this, it will work but in future should be checked)
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
	static int defaultTimeout = TIME_TO_PING;	
	static time_t timeSav;	
	static int pingRequestSent = 0;			// 0 - not sent, 1 - already sent
	configuration C = load_config(1, "");		//load config to get current nickname

	#ifdef PING_DEBUG
	printf("is_alive:ping = %d\n", pingRequestSent);
	#endif

	if (state == 0)	{	
		if ( ((long) timeGot - timeSav) > defaultTimeout ) {	// ok, it's time to send PING to yourself or probably we have problems
			if (pingRequestSent == 0){			// if we didn't sent PING request before, let's do it and change pingRequestSent
				#ifdef PING_DEBUG
				printf("PANIC - Request sent\n");
				printf("  stored time   = %ld\n",timeSav);
				printf("  recieved time = %ld\n",timeGot);
				#endif
				irc_cmd_ctcp_request(session, C.nick, "PING");
				pingRequestSent = 1;
				return 0;
			}
			else{						// we already sent PING
				if (((long) timeGot - timeSav) > (defaultTimeout+180)){	// we know, that the time is greater than stored one, so we add 3min(180) and if it's greater, then re-connect
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
		printf("defaultTimeout             	= %d\n", defaultTimeout);						//debug
		#endif
		return 0;
	}
	return 0;		// how it could be possible to come here? ok, whatever
}
void event_connect (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{	
	is_alive(1, time(NULL), session );				// this time really 'initial' timestamp written into the watchdog function

//	dump_event (session, event, origin, params, count);
	configuration C = load_config(1, "");
#ifdef X_MODE
	irc_cmd_user_mode (session, "+x");			// TODO clarify this shit
#endif
	irc_cmd_join (session, C.channel, 0);
}

void dump_event (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
#ifdef DEBUG
	char buf[512];
	int cnt;
#endif
	char nowTime[20];

	char nickBuf[128];
	char hostBuf[1024];

	time_t now = time(NULL);

#ifdef DEBUG
	buf[0] = '\0';
#endif
	FILE * fp;		// set FD for the file for logs
	
	configuration C = load_config(1, "");

#ifdef DEBUG
	for ( cnt = 0; cnt < count; cnt++ ) {
		if ( cnt )
			strcat (buf, "|");
		strcat (buf, params[cnt]);
	
	}
#endif
	// Set name for the log file

	irc_target_get_nick (origin, nickBuf, 128);
	irc_target_get_host (origin, hostBuf, 1024);	
	
	strftime(nowTime, 20, "logs/%Y-%m-%d.txt", localtime(&now));
	strcat(C.logPath, nowTime);		// now C.logPath have the correct name of the file

 	if ( (fp = fopen (C.logPath, "ab")) != NULL )
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
		!(strcmp(event,"QUIT"))    ? fprintf(fp,"%s << %s [%s] quit: %s \n",nowTime, nickBuf, hostBuf, params[0]): 
		!(strcmp(event,"NICK"))    ? fprintf(fp,"%s -!- %s [%s] changed nick to: %s \n",nowTime, nickBuf, hostBuf, params[0]): 0;

		if (!strcmp(event,"KICK")){
		// Reloading configuration in case our nick has been changed recently
			if ( !(strcmp(params[1], C.nick)) && C.reJoin == 1 ){
				event_connect (session, event, origin, params, count);
			}
		}
		else if (!strcmp(event, "CHANNEL"))
			if (strncmp(params[1], C.nick, strlen(C.nick)) == 0 )
				strlen(C.link) == 7 ? irc_cmd_msg(session, params[0], ";)") : irc_cmd_msg(session, params[0], C.link);

		fclose (fp);
	}
	else {
		printf("Unable to open/create log file: check folder permissions\n");			// and die, but it doesn't die.. well. Ok for now.
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
		irc_cmd_ctcp_reply(session, origin, "VERSION loperIRCLogBot v.1.2");
	}
	else if( strcmp (params[0], "SOURCE") == 0 ){
		dump_event (session, event, origin, params, count);
		irc_cmd_ctcp_reply(session, origin, "SOURCE loperIRCLogBot v.1.2");
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
		irc_cmd_ctcp_reply(session, origin, "CLIENTINFO loperIRCLogBot v.1.2 - Supported tags: VERSION, SOURCE, TIME, PING, CLIENTINFO");
	}
}

void event_join (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	dump_event (session, event, origin, params, count);
	irc_cmd_user_mode (session, "+i");
}

void nick_change(irc_session_t * session){		
	static char append[] = "|0";
	configuration C = load_config(1, "");
	
	if ( append[1] > '9')				// Refactoring request
		append[1] = '0';			// if all your nicknames with |0 |2 ... |9 are already occupied, we're fucked up. Really, there is no good solution in this code.
	
	// Let's say that we have string like 1234567|0, and maxNickLength = 10, then we see, that string length is greater (9) then maxNickLength-2, but also we've already appended something like "|0" into it, then
	// we could check it and wipe. Otherwise we wipe 2 last chars.
	if ( strlen(C.nick) > (C.maxNickLength - 2) ){
		if ( (C.nick[C.maxNickLength-3] == '|') && (C.nick[C.maxNickLength-2] >= '0') && (C.nick[C.maxNickLength-2] <= '9') )
			C.nick[C.maxNickLength-3] = '\0';
		else
			C.nick[C.maxNickLength-2] = '\0';
	}
	if ( C.nick[strlen(C.nick)-2] == '|' )
		C.nick[strlen(C.nick)-1] = append[1];
	else
		strncat(C.nick, append, 2);
	
	append[1]++;

	C = load_config(2, C.nick);			

	irc_cmd_nick(session, C.nick);
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
	configuration C = load_config(1,"");

	dump_event (session, event, origin, params, count);


	if ( !origin )
                return;
	else
		irc_target_get_nick (origin, nickBuf, 1024);	
		
	if ( strcasecmp (nickBuf, "nickserv") )
		return;
	if ( (strstr (params[1], "This nickname is registered") != NULL) || (strstr (params[1], "This nickname is owned by someone else") != NULL) ){
		irc_send_raw(session, "nickserv IDENTIFY %s", C.password);
	}
	else if( strstr (params[1], "Password accepted") != NULL )
		printf ("Nickserv authentication succeeded\n");

}

int make_template(char * dest){
	FILE * templ;
	strcat (dest, "bot.conf");

	if ( (templ = fopen(dest,"w")) != NULL) {
		fprintf(templ, "server:        \n");
		fprintf(templ, "channel:       \n");
		fprintf(templ, "port:          \n");
		fprintf(templ, "nick:          \n");
		fprintf(templ, "username:      \n");
		fprintf(templ, "realname:      \n");
		fprintf(templ, "password:      0\n");
		fprintf(templ, "maxNickLength: 30\n");
		fprintf(templ, "logPath:       0\n");
		fprintf(templ, "link:          0\n");
		fprintf(templ, "reJoin:        yes\n");
		fclose(templ);
		return 0;
	}
	else 
		return 1;
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
			"reJoin"};
	int i;
	
	switch (sum){
		case -1:
			printf("Configuration file issue: value or attribute missed or redundant\n");
			break;
		case -2:
			printf ("Unable to open configuration file\n");
			break;
		case -3:
			printf("Configuration file issue: 'logPath' is not defined as absolute path\n");
			break;
		default:
			printf("Configuration file issue. Next field(s) not found:\n");
			for (i=0; i<11; i++)
				if ((sum & (1<<i)) != (1<<i))
					printf("\t'%s'\n", setLst[i]);
	}
}

int main(int argc, char *argv[]) {
	configuration config;
	
	irc_callbacks_t callbacks;	// The IRC callbacks structure

	irc_session_t * session;

	struct timeval tv;
	fd_set in_set, out_set;
	int maxfd = 0;
	        
	// Let's find out the path to executable file!
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
			printf("Avaliable options:\n\n"
				"  -g, --genconf         Create configuration file template. Attention! It will overrite your existing configuration file.\n"
				"  -s, --silent          Silent mode. All program messages stores to the 'output.txt' file\n"
				"  -n, --nomessage       Don't print any messages anywhere\n"
				"  -v, --version         Application version\n\n"
				"      --help            Show this message and terminate application\n\n"
				"Configuration stores at the ./bot.conf. Please pay attention: maximum lenght of the file is 10 strings. In case if you don't use password for your nickname set it to 0.\n"
				"Log files stores at the \"./logs\" folder, if in \"bot.conf\" you defined '0' for this setting, otherwise it's stores at \"YOUR_PATH/logs\". Files have next format: YYY-MM-DD.txt\n"
				"Password for the nickname is taken form the configuration file. '0' stands for 'no password'\n"
				"'Link' is the link that returns when someone says \"bot_name something-something\". '0' stands for no link\n"
				"Be smart! Don't use too long nicknames. Usually they used should 30 characters long but may vary from server to server. Don't worry if your nickname shorter, but don't let it be greater then 128 characters.\n");
			return 0;
		}
		else if( (strcmp("-g", argv[1]) == 0) || (strcmp("--genconf", argv[1]) == 0)) {
			printf("Attention! This action will remove your current bot.conf file\nDo you really want to generate template?\n"
				"y/n: ");
			do{
				switch (getchar()){
					case 'y': case 'Y': 
						if (make_template(dest) == 0){
							printf("Configuratation template created.\n");
							return 0;
						}
						else {
							printf("Unable to create configuration template file\nCheck folder permissions\n");
							return 1;
						}
					case 'n': case 'N': 
						printf("No changes applied\n");                   
						return 0;
					case '\n':                                      //     ,__o
						break;                                  //   _-\_<, 
					default:                                        //  (*)/'(*)
					 	while ( getchar() != '\n' );		
				}
				
				printf("\033[A\033[2K");				// VT100 escape codes

			} while(1);
			return 0;
		}
		// in future define deamon mode here
		else if( (strcmp("-s", argv[1]) == 0) || (strcmp("--silent", argv[1]) == 0)) {
	 		if ( (freopen ("output.txt", "ab", stdout)) == NULL ){
				printf("Unable to redirect stdout to file\n");
				return 1;
			}
	 		if ( (freopen ("output.txt", "ab", stderr)) == NULL ){
				printf("Unable to redirect stderr to file\n");
				return 1;
			}
			freopen("/dev/null", "rb", stdin);

		}
		// end
		else if( (strcmp("-n", argv[1]) == 0) || (strcmp("--nomessage", argv[1]) == 0)) {
			freopen("/dev/null", "rb", stdout);	//doing it using guidelines form man daemon
			freopen("/dev/null", "rb", stdin);
			freopen("/dev/null", "rb", stderr);
		}
		else if ( (strcmp("-v", argv[1]) == 0) ||  (strcmp("--version", argv[1]) == 0)) {
			printf("loperLogBot: 1.2 - build %s %s\n", __TIME__, __DATE__);
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

//		-================================================================================================-
	config = load_config(0, dest);
	if (config.status == 0) {
		printf("\t--------\n"
			"\tSETTINGS\n"
			"\t--------\n"
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
			"\t--------\n", 
			config.server, 
			config.channel, 
			config.port, 
			config.nick, 
			config.username, 
			config.realname, 
			config.password, 
			config.maxNickLength, 
			config.logPath, 
			config.link, 
			config.reJoin == 0?"No":"Yes");
	
		if ( set_log_dir(config.logPath) != 0){				// set logs directory
			printf ("Unable to create directory for log files (%s)\n"
				"Please make sure that you have premissions to write in this directory\n", 
				config.logPath);
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
			callbacks.event_ctcp_rep = event_ctcp_rep;	// &  dump_event? Now no need to dump.	This event is triggered upon receipt of an CTCP response. Thus if you generate the CTCP message and the remote user responded, this event handler will be called.
//			callbacks.event_unknown = event_unknown;	// this event will be triggered when app recieve PING request from server. And any other unknown event, and this is buggy moment, and should be fixed. 
        		
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
				printTimeStamp();
				printf("Connecting...\n");
				session = irc_create_session( &callbacks );
				
	        		if ( !session ){
					printTimeStamp();
					printf("Unable to create session\n");
					// return 1; 							// give a try once again
				}
				else {
		                	irc_option_set( session, LIBIRC_OPTION_SSL_NO_VERIFY );
				
	        			// Connect to a regular IRC server
					if ( irc_connect (session, config.server, config.port, 0, config.nick, config.username, config.realname ) ){
						printTimeStamp();
						printf ("Could not connect: %s\n", irc_strerror (irc_errno(session)));
						// return 1;						//give a try once again
						// TODO add a counter for few reply and die / do something with this
		                	}
					else {	

						is_alive(1, time(NULL), session);	//initialize is_alive internal start time
						printTimeStamp();
						printf("Connection established\n");
		
						///////////////////// irc_run() replacement  ///////////////////
						while ( irc_is_connected(session) )
						{
							tv.tv_usec = 250000;
							tv.tv_sec = 0;
		        
							// Init sets
							FD_ZERO (&in_set);
							FD_ZERO (&out_set);
		        
							// +1 chk
							if ( is_alive(0, time(NULL), session) != 0 ){
								printf("\n\t-----------------------------------\n\t");
								printTimeStamp();
								printf("PING timeout\n"
									"\t-----------------------------------\n");
                                                                 break; // 1
							}
							// end +1 chk

							
							irc_add_select_descriptors (session, &in_set, &out_set, &maxfd);
		
							if ( select (maxfd + 1, &in_set, &out_set, 0, &tv) < 0 )
							{
								printTimeStamp();
								printf ("Could not connect or I/O error: LIBIRC_ERR_TERMINATED\n");
								break; // 1
							}
					
							if ( irc_process_select_descriptors (session, &in_set, &out_set) ){
								printTimeStamp();
								printf ("Could not connect or I/O error\n" );
								break;	// 1
							}
		
						}
						irc_disconnect(session);
						printTimeStamp();
						printf("Disconnected after successful connection\n");
						//return 0;
						////////////////////////////////////////////////////////////////////////////
					}
				}
				printTimeStamp();
				printf ("Not connected: will retry in 5 sec\n");
				sleep (5);
			} // while end
		}
		return 0; // never happens
	}
	else 
		reportSettingsIssues(config.status);

	return 0;
}
