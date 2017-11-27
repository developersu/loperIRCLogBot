// Define period before sending PING to server (5 min - 300).
#define TIME_TO_SEND_PING	300	// how often we send pings to server
#define TIME_TO_WAIT_PING	180	// after we sent request to server, how much time should we wait for response before re-connect?

// Set if you want to turn mode +x on while connecting
#define X_MODE

// Current version of the program
#define __CUR_VER__ "1.3"

#define NPING_DEBUG
#define NDEBUG
#define NDEBUG_LOG

#ifndef __TIME__
#define __TIME__ "-"
#endif
#ifndef __DATE__
#define __DATE__ "-"
#endif

#define DEF_HELP_MSG \
"Avaliable options:\n\n \
  -d, --daemon          Start application as daemon (experimental)\n \
  -g, --genconf         Create configuration file template. Attention! It will overrite your existing configuration file.\n \
  -s, --silent          Silent mode. All program messages stores to the 'output.txt' file\n \
  -v, --version         Application version\n\n \
      --help            Show this message and terminate application\n\n \
Configuration stores at the ./bot.conf. Please pay attention: maximum lenght of the file is 10 strings. In case if you don't use password for your nickname set it to 0.\n \
Log files stores at the './logs' folder, if in 'bot.conf' you defined '0' for this setting, otherwise it's stores at 'YOUR_PATH/logs'. Files have next format: YYY-MM-DD.txt\n \
Password for the nickname is taken form the configuration file. '0' stands for 'no password'\n \
'Link' is the link that returns when someone says 'bot_name something-something'. '0' stands for no link\n \
Be smart! Don't use too long nicknames. Usually they used should 30 characters long but may vary from server to server. Don't worry if your nickname shorter, but don't let it be greater then 128 characters.\n"

#define DEF_DEFAULT_TEMPLATE \
"server:        \n\
channel:       \n\
port:          \n\
nick:          \n\
username:      \n\
realname:      \n\
password:      0\n\
maxNickLength: 30\n\
logPath:       0\n\
link:          0\n\
reJoin:        yes\n\
floodTimeOut:  10\n"
