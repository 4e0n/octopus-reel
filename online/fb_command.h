/*      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
 *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
 *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            */

/* The constants and structures related with frontend-backend communication. */

#ifndef FB_COMMAND_H
#define FB_COMMAND_H

/* Commands sent to the acquisition backend from the frontend daemon. */
#define	ACQ_START		(0x0001)
#define	ACQ_STOP		(0x0002)
#define ACQ_CMD_F2B		(0x0003)
#define ACQ_CMD_B2F		(0x0004)
#define ACQ_ALERT		(0x0005) /* a.k.a. Problem in backend! */

#define	B2F_DATA_SYN		(0x1001) /* Xfer handshake req. from backend */
#define	F2B_DATA_ACK		(0x1002) /* Xfer handshake ack. from frntend */ 

#define	F2B_RESET_SYN		(0x1003) /* Invoke backend rst from frontend */
#define	B2F_RESET_ACK		(0x1004) /* Frontend approval of reset */

#define F2B_GET_BUF_SIZE	(0x1005) /* Buffer size alloc'd in backend */
#define B2F_PUT_BUF_SIZE	(0x1006)
#define F2B_GET_TOTAL_COUNT	(0x1007) /* N+2 */
#define B2F_PUT_TOTAL_COUNT	(0x1008)

#define F2B_TRIGTEST		(0x1009)

#define ALERT_DATA_LOSS		(0x0001) /* Buffer underrun ->
                                            Data is not being fetched fast
				            enough by the frontend.. */

/* Command Message for the frontend <-> backend communication. */
typedef struct _fb_command { unsigned short id; int iparam[4]; } fb_command;

#endif
