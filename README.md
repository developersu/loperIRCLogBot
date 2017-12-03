# loperIRCLogBot

loperIRCLogBot is an IRC logging bot made for writing log-files. It could connect to one server and log one channel.
When someone says bot's name at the chat, the URL to logs web-storage could be published (or any message defined in configuration file).

## Description

Works for linux. Need to be tested on other UNIX-like OS.

Key feachures and limitations:
* Could work as a daemon.
* Store configuration file in current folder (./bot.conf)  or on /etc/loperIRCLogBot/bot.conf
* Only one channel and server supported for now.
* User can define folder to store files.
* Reconnect in ~6 minutes. Maybe earlier. 
* Dumb flood-control
* Authorization only as: /nickserv IDENTIFY [password] 

## License

Source code spreads under the GNU General Public License v.3. You can find it in LICENSE file or just visit www.gnu.org (it should be there for sure). 
It also uses  libircclient writted by George Yunaev, that is stored at ./src/libircclient-1.9. Libircclient is licensed under Lesser General Public License version 3 or higher. 

## Build & Deploy

```
$ make
$ sudo make install
```
## Uninstall
```
$ sudo make uninstall
```

NOTES: 
I believe you could change Makefile to let this app work with libircclien as a shared library. Now it's statically linked.
If you have bot.conf file at /etc/loperIRCLogBot folder it won't be deleted. So remember to delete it manually if needed.

## Configuration

Application settings should be set on /etc/loperIRCLogBot/bot.conf OR ./bin/bot.conf file. You could generate template by passing -g option: './loperIRCLogBot -g'. 
Configuration file contains 15 lines, so make sure that you didn't define any sensetive information below these 15 lines.
Here is an example:
```
server:          irc.example.com
channel:         #channel
port:            6667
nick:            awesome_nick_here
username:        usrname
realname:        realname
password:        0 
maxNickLength:   30
logPath:         0 
link:            http://localhost:8080/logs/
reJoin:          yes
floodTimeOut:    10
```

Let's go deeper:

* 'server' should be set as described. NO "irc://" or stuff like this allowed, no slashes and backslashes.
* 'channel' is the channel to log. Should be started by '#' symbol
* 'port' is server port.
* 'nick' is nick. Restricted by 'maxNickLength'. It means, that your nick shouldn't be greater then 'maxNickLength'.
* 'username' is username that defined as second parameter at your nick-on-server. Like nick!~[USERNAME]@ip_adress.
* 'realname' is realname. Don't ask, just set here something, ok?
* 'password' is a password for the nick. It goes to server as '/nickserv IDENTIFY [password]'. If '0' then no password needed. 
* 'maxNickLength' is a maximum length of the nick. Various servers restrict the length of the nick. '30' should be enough. '128' is maximum, but I'm not sure that you have to set '128' in here, because if server doesn't support nicknames with such lenght, you could face to unforseen behavior inside the application.
* 'logPath' is a path to 'logs' directory, where all your logs will be stored. If '0' then they will be stored at executable file folder. Pay attention, that logPath should be defined as full path. Not like '~/logs' or '../../logs'.
* 'link' is a link, that will be provided to anyone on the channel, who starts his/her message by your nick.
* 'reJoin'. If set to 'yes', then bot will be re-connect to channel after kick. If 'no' then it won't.
* 'floodTimeOut' is a time in seconds before answering on next request sent to us. 

## Options & work modes

When you start application, you could pass next options:
   -d, --daemon          Start application as daemon. Writes to /var/log/loperIRCLogBot.log and syslog. Stores PID number at /var/run/loperIRCLogBot.pid
                         Also can interrupt correctly on SIGTERM, not so smooth on SIGINT and reopen log file directory on SIGHUP (used to handle logrotation).
   -g, --genconf         Create configuration file template. Attention! It will overrite your existing configuration file.
   -s, --silent          Silent mode. All program messages stores to the 'output.txt' file
   -v, --version         Application version
       --help            Show this message and terminate application
