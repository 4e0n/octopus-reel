#ifndef TCPCOMMAND_H
#define TCPCOMMAND_H

const int CMD_ACQINFO=0x0001;
const QString CMD_ACQINFO_S="ACQINFO";
const int CMD_TRIGGER=0x0002;
const QString CMD_TRIGGER_S="TRIGGER";
const int CMD_AMPSYNC=0x0003;
const QString CMD_AMPSYNC_S="AMPSYNC";
const int CMD_STATUS=0x0004;
const QString CMD_STATUS_S="STATUS";

const int CMD_DISCONNECT=0x000f;
const QString CMD_DISCONNECT_S="DISCONNECT";

const int CMD_GETCONF=0x1001;
const QString CMD_GETCONF_S="GETCONF";
const int CMD_GETCHAN=0x1002;
const QString CMD_GETCHAN_S="GETCHAN";

const int CMD_REBOOT=0xfffe;
const QString CMD_REBOOT_S="REBOOT";
const int CMD_SHUTDOWN=0xffff;
const QString CMD_SHUTDOWN_S="SHUTDOWN";

#endif
