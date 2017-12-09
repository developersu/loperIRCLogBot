#include <stdlib.h>	// something for load_config function

#define CONFIG_INIT	0	// defined for load_config()
#define RETURN_STRUCT	1	// defined for load_config()
#define NICK_CHANGE	2	// defined for load_config()

typedef struct conf_sruct {
        int status;
        char *server;					// !
        char *channel;					// !
        int port;
        char nick[129];					// max nick length is 128 + '\0'
        char *username;					// !
        char *realname;					// !
        char *password;					// !
	int maxNickLength;
	char *logPath;					// !
	char *link;					// !	should be enough to store link
	int reJoin;					// 	1 - yes, 0 - no
	int floodTimeOut;					
} configuration;

configuration * load_config(int run, char * nick_or_path) {	
/*
 * if run = CONFIG_INIT (0) we load config from the file
 * if run = RETURN_STRUCT (1) we return configuration structure stored in this function
 * if run = NICK_CHANGE (2) we overrite 'nick' at the configuration structure
 */
	int i,j;

	FILE *conf;
	
	char *st;				// WHAT THE FUCKING POINTER HERE? Dark side magic forces.
	char **sta[2];			
	static configuration C;			// returning structure
	
	char confFilePath[PATH_MAX];

	int checksum = 0;

	// added
	char tempChar;
	size_t countLinesInFile = 0;

	char **fArr = NULL;
	size_t eN = 0;

	// --*---
	if (run == CONFIG_INIT) {				// the first time call
		if ( (conf = fopen("/etc/loperIRCLogBot/bot.conf","r")) != NULL)
			printf("Using configuration file stored at /etc/loperIRCLogBot/bot.conf\n"
				"Prefere using the one, stored in this folder? Just rename or delete the one from /etc...\n");
		else {
			strcpy(confFilePath, nick_or_path);
			strcat(confFilePath, "bot.conf");		// add bot.conf to path-to-executable
			
			if ( (conf = fopen(confFilePath,"r")) != NULL) 
				printf("Using configuration file stored at %s\n"
					"Please note, configuration file also could be stored at '/etc/loperIRCLogBot/bot.conf' and have higher priority\n", confFilePath);
			else {
				C.status = -2;			//unable to open file = -2
				return &C;
			}
		}
		//	count a number of lines inside file
		while ((tempChar = getc(conf)) != EOF){
			if (tempChar == '\n')
				countLinesInFile++;	// this size should be used to define 2D array for using in getline();
		}
		rewind(conf);				// Move read pointer to the beggining of our file
		#ifdef DEBUG_LOG
		printf("\nFile have %zu lines\n", countLinesInFile);
		#endif
		//	end counting
		if ((fArr = (char **) malloc(countLinesInFile*sizeof(char *))) == NULL){	// allocate 2D array
			C.status = -4;
			return &C;
		}
			
		for (i = 0; i<countLinesInFile; i++){					// from 0 to end
//			fArr[i] = NULL;					// initialize by NULL to avoid undefined behavior. 
			if (getline(&fArr[i], &eN, conf) == -1){			// Error reading or EOF. Shouldn't be EOF.
				C.status = -5;
				return &C;
			}
			#ifdef DEBUG_LOG
			printf("Alloc 4 string #%d:\t%zu\n",i, eN);
			#endif
			eN = 0;
		}
		fclose(conf);

		#ifdef DEBUG_LOG
			printf("______Content of the file______\n");
			for (i = 0; i<countLinesInFile; i++)	// from 0 to end
				printf("%s", fArr[i]);
			printf("________________________________\n");
		#endif
		/////////////////////////////////////
		for (i=0;i<2;i++){									// TODO: check if we allocated memory or not: NULL if not
			if ((sta[i] = (char **) malloc (countLinesInFile * sizeof(char *))) == NULL){	// TODO: optimization needed. 1D array for example
				C.status = -4;	
				return &C;
			}
			for (j=0;j<countLinesInFile;j++){
				if ((sta[i][j] = (char *) malloc(sizeof(char *))) == NULL){		// STRLEN()
					C.status = -4;	
					return &C;
				}
				sta[i][j][0] = '\0';
			}
		}
		/////////////////////////////////////

		// Parsing configuration file			- IF STRING DOESN'T HAVE ":" - SKIP IT
		for (i=0;i<countLinesInFile;i++){			// Read all recieved fields
			if ( strstr(fArr[i], ":") != NULL ) {    
				st = strtok(fArr[i], ": \t\n");		// if we can't get ANY parameter at the string, that have ":" inside, then skip this string
				if (st != NULL){			
					sta[0][i] = (char *) realloc (sta[0][i], (strlen(st)+1) * sizeof(char *));
					#ifdef DEBUG_LOG
					printf("sta[0][%d] = %zu\n", i,strlen(st)+1 );
					#endif
					strcpy(sta[0][i],st);
	
					st = strtok(NULL, " \t\n");	// if we see anything before 'space' \t or \n

					if (st == NULL){		// if we had a first parameter in string, (no matter was it set before ":" or after) 
									//   and don't have the second one, 
						C.status = -1;		// then we ruin the chain and returning error. 'Status' of incorrect config-file = "-1"
						return &C;
					} 
					else {
						sta[1][i] = (char *) realloc (sta[1][i], (strlen(st)+1) * sizeof(char *));
						#ifdef DEBUG_LOG
						printf("sta[1][%d] = %zu\n", i,strlen(st)+1 );
						#endif
						strcpy(sta[1][i], st);					

						if ( strstr(sta[0][i], "link") != NULL ){	// if it's 'link', then add here everything user wrote
							st = strtok(NULL, "\n");
							if (st != NULL){
								sta[1][i] = (char *) realloc (sta[1][i], (strlen(sta[1][i])+2) * sizeof(char *) + (strlen(st)+1) * sizeof(char *));
								#ifdef DEBUG_LOG
								printf("sta[0][%d] = %zu\n", i,strlen(st)+1 );
								#endif
								strcat(sta[1][i], " ");
								strcat(sta[1][i], st);
							}
						}
					}
				}
			}
		}

		for (i=0;i<countLinesInFile;i++){
			if ( strstr(sta[0][i], "server") != NULL )
				checksum |= 1<<0;
			else if ( strstr(sta[0][i], "channel") != NULL )
				checksum |= 1<<1;
			else if ( strstr(sta[0][i], "port") != NULL )
				checksum |= 1<<2;
			else if ( strstr(sta[0][i], "nick") != NULL )
				checksum |= 1<<3;
			else if ( strstr(sta[0][i], "username") != NULL )
				checksum |= 1<<4;
			else if ( strstr(sta[0][i], "realname") != NULL )
				checksum |= 1<<5;
			else if ( strstr(sta[0][i], "password") != NULL )
				checksum |= 1<<6;
			else if ( strstr(sta[0][i], "maxNickLength") != NULL )
				checksum |= 1<<7;
			else if ( strstr(sta[0][i], "logPath") != NULL )
				checksum |= 1<<8;
			else if ( strstr(sta[0][i], "link") != NULL )
				checksum |= 1<<9;
			else if ( strstr(sta[0][i], "reJoin") != NULL )
				checksum |= 1<<10;
			else if ( strstr(sta[0][i], "floodTimeOut") != NULL )
				checksum |= 1<<11;
				
		}
		if (checksum != 0b111111111111){
			C.status = checksum;			// incorrect number of settings defined
			return &C;
		}
		else {
			// Format array for return in case we're all good
	
			C.status = 0;				//  OK = 0.
	
			for (i=0; i<countLinesInFile; i++){		// Good
				if ((strcmp(sta[0][i], "server")) == 0){
					if ((C.server = (char *) malloc (strlen(sta[1][i])+1 * sizeof(char *))) == NULL){	// NOTE: And never free();
						C.status = -4;	
						return &C;
					}
					strcpy(C.server, sta[1][i]);
					}
				else if ( (strcmp(sta[0][i], "port")) == 0)
					C.port = atoi(sta[1][i]);
				else if ( (strcmp(sta[0][i], "channel")) == 0){
					if ((C.channel = (char *) malloc (strlen(sta[1][i])+1 * sizeof(char *))) == NULL){	// NOTE: And never free();
						C.status = -4;	
						return &C;
					}
					strcpy(C.channel, sta[1][i]);
					}
				else if ( (strcmp(sta[0][i], "nick")) == 0)
					strcpy(C.nick, sta[1][i]);
				else if ( (strcmp(sta[0][i], "username")) == 0){
					if ((C.username = (char *) malloc (strlen(sta[1][i])+1 * sizeof(char *))) == NULL){	// NOTE: And never free();
						C.status = -4;	
						return &C;
					}
					strcpy(C.username, sta[1][i]);
					}
				else if ( (strcmp(sta[0][i], "realname")) == 0){
					if ((C.realname = (char *) malloc (strlen(sta[1][i])+1 * sizeof(char *))) == NULL){	// NOTE: And never free();
						C.status = -4;	
						return &C;
					}
					strcpy(C.realname, sta[1][i]);
					}
				else if ( (strcmp(sta[0][i], "password")) == 0){
					if ((C.password = (char *) malloc (strlen(sta[1][i])+1 * sizeof(char *))) == NULL){	// NOTE: And never free();
						C.status = -4;	
						return &C;
					}
					strcpy(C.password, sta[1][i]);
					}
				else if ( (strcmp(sta[0][i], "maxNickLength")) == 0){
					C.maxNickLength = atoi(sta[1][i]);
					if (C.maxNickLength > 128){
						C.maxNickLength = 128;			// now idiots could feel themselfs protected. Libircclient restriction IIRC set to 128 chars
					}
				}
				else if ( (strcmp(sta[0][i], "logPath")) == 0){
					if (strcmp(sta[1][i], "0") == 0){
						C.logPath = (char *) malloc (strlen(nick_or_path)+1 * sizeof(char *));	// NOTE: And never free();
						strcpy(C.logPath, nick_or_path);
					}
					else {
						if (sta[1][i][0] != '/'){
							C.status = -3;
							return &C;
						}
						else{
							if ((C.logPath = (char *) malloc (strlen(sta[1][i])+2 * sizeof(char *))) == NULL){	// NOTE: And never free(); +2 to include variant, when '/' is needed
								C.status = -4;	
								return &C;
							}
							strcpy(C.logPath, sta[1][i]);
							if ( C.logPath[strlen(C.logPath)] != '/' )
								strcat (C.logPath, "/");
						}
					}
				}
				else if ( (strcmp(sta[0][i], "link")) == 0){
					if((C.link = (char *) malloc (strlen(sta[1][i])+1 * sizeof(char *))) == NULL){	// NOTE: And never free();
						C.status = -4;	
						return &C;
					}
					strcpy(C.link, sta[1][i]);
				}
				else if ( (strcmp(sta[0][i], "reJoin")) == 0){
					if (strcmp(sta[1][i], "yes") == 0 || strcmp(sta[i][1], "Yes") == 0 )
						C.reJoin = 1;
					else
						C.reJoin = 0;
				}
				else if ( (strcmp(sta[0][i], "floodTimeOut")) == 0)
					C.floodTimeOut = atoi(sta[1][i]);
			}
			if (strlen(C.nick) > C.maxNickLength)
				C.nick[C.maxNickLength] = '\0';				// yeah, they will love it, before set nick name longer then 128 char =(
		}
		// ++++++++++++++++++++++++++++++++++++++++++++++
	
		#ifdef DEBUG_LOG
		printf("____Recieved keys____"
			"\n______________________\n");
		for (i=0;i<countLinesInFile; i++){
			printf("%s - %s\n", sta[0][i], sta[1][i]);
		}
		printf("______________________\n");
		#endif

		// Free allocated arrays
		for (i=0; i<countLinesInFile; i++)
			free(fArr[i]);
		free(fArr);
		for (i=0;i<2;i++){
			for (j=0;j<countLinesInFile;j++)
				free(sta[i][j]);
		}
		for (i=0;i<2;i++)
			free(sta[i]);
		//end

		return &C;
	}
	else if ( run == RETURN_STRUCT ){
		return &C;				// just return already loaded structure by request
	}
	else if ( run == NICK_CHANGE){				// save nick recieved
		strcpy(C.nick, nick_or_path);
		return &C;
	}
}
