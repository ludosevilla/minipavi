/*
 * Softmodemminipavi for Asterisk
 *
 * 2024 Jean-arthur Silve <contact@minipavi.fr>
 *
 * Simulates a V23 modem for French Minitel comunications for use with MiniPavi.
 * The V23 modem on the other end is connected to the specified server using a simple TCP connection (like Telnet).
 *
 * Mainly based on softmodem.c by Christian Groeger <code@proquari.at>
 
 * Removed some useless options for V23 French Minitel for easier configuration
 * Added some specific stuffs for Minipavi.
 *
 * Based on app_fax.c by Dmitry Andrianov <asterisk@dima.spb.ru>
 * and Steve Underwood <steveu@coppice.org>
 *
 * Parity options added 2018 Rob O'Donnell
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 *
 */

/*** MODULEINFO
	<depend>spandsp</depend>
	<support_level>extended</support_level>
***/

/* Needed for spandsp headers */
#define ASTMM_LIBC ASTMM_IGNORE
#include "asterisk.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <pthread.h>
#include <errno.h>
#include <tiffio.h>
#include <time.h>


/* For TDD stuff */
#define SPANDSP_EXPOSE_INTERNAL_STRUCTURES

#include <spandsp.h>
#ifdef HAVE_SPANDSP_EXPOSE_H
#include <spandsp/expose.h>
#endif
#include <spandsp/v22bis.h>
#include <spandsp/v18.h>

/* For TDD stuff */
#define SPANDSP_EXPOSE_INTERNAL_STRUCTURES
#include <spandsp/version.h>
#include <spandsp/logging.h>
#include <spandsp/fsk.h>
#include <spandsp/async.h>
#include <spandsp/tone_generate.h>


#include "asterisk/file.h"
#include "asterisk/module.h"
#include "asterisk/channel.h"
#include "asterisk/strings.h"
#include "asterisk/lock.h"
#include "asterisk/app.h"
#include "asterisk/pbx.h"
#include "asterisk/format_cache.h"
#include "asterisk/logger.h"
#include "asterisk/utils.h"
#include "asterisk/dsp.h"
#include "asterisk/manager.h"

/*** DOCUMENTATION
	<application name="SoftmodemMinipavi" language="en_US">
		<synopsis>
			Softmodem for MiniPavi that connects the caller to a MiniPavi telnet server (TCP port).
		</synopsis>
		<syntax>
			<parameter name="hostname" required="false">
				<para>Hostname. Default is 127.0.0.1</para>
			</parameter>
			<parameter name="port" required="false">
				<para>Port. Default is 23 (default Telnet port).</para>
			</parameter>
			<parameter name="callerinfo" required="false">
				<para>Callerinfo (should be filled in the dialplan: ${CDR(src)} ).</para>
			</parameter>
			<parameter name="starturl" required="false">
				<para>Service url to be called at start of the call.</para>
			</parameter>
			<parameter name="pce" required="false">
				<para>PCE activated</para>
			</parameter>
			
			<parameter name="options">
				<optionlist>
					<option name="f">
						<para>Flip the mode from answering to originating (1200/75 or 75/1200 bds).</para>
					</option>
					<option name="r">
						<para>RX cutoff (dBi, float, default: -35)</para>
					</option>
					<option name="t">
						<para>tx power (dBi, float, default: -28)</para>
					</option>
				</optionlist>
			</parameter>
		</syntax>
		<description>
			<para>Simulates a V23 modem for French Minitel comunications for use with MiniPavi. The modem on the other end is connected to the specified server using a simple TCP connection (like Telnet).</para>
		</description>
	</application>
 ***/

static const char app[] = "SoftmodemMinipavi";

enum {
	OPT_RX_CUTOFF =      (1 << 0),
	OPT_TX_POWER =       (1 << 1),
	OPT_FLIP_MODE =      (1 << 12),
};

enum {
	OPT_ARG_RX_CUTOFF,
	OPT_ARG_TX_POWER,
	/* Must be the last element */
	OPT_ARG_ARRAY_SIZE,
};

AST_APP_OPTIONS(additional_options, BEGIN_OPTIONS
	AST_APP_OPTION_ARG('r', OPT_RX_CUTOFF, OPT_ARG_RX_CUTOFF),
	AST_APP_OPTION_ARG('t', OPT_TX_POWER, OPT_ARG_TX_POWER),
	AST_APP_OPTION('f', OPT_FLIP_MODE),
END_OPTIONS );

#define MAX_SAMPLES 240


typedef struct {
	struct ast_channel *chan;
	const char *host;	//telnetd host+port
	int port;
	const char *callerinfo;	
	const char *starturl;		
	int pce;
	float txpower;
	float rxcutoff;
	int databits;
	int stopbits;
	volatile int finished;
	int	paritytype;
	unsigned int flipmode:1;
} modem_session;

#define MODEM_BITBUFFER_SIZE 16
typedef struct {
	int answertone;		/* terminal is active (=sends data) */
	int nulsent;		/* we sent a NULL as very first character (at least the DBT03 expects this) */
} connection_state;

typedef struct {
	int sock;
	int bitbuffer[MODEM_BITBUFFER_SIZE];
	int writepos;
	int readpos;
	int fill;
	connection_state *state;
	modem_session *session;
} modem_data;


/*! \brief This is called by spandsp whenever it filters a new bit from the line */
static void modem_put_bit(void *user_data, int bit)
{
	int stop, stop2, i;
	modem_data *rx = (modem_data*) user_data;

	int databits = rx->session->databits;
	int stopbits = rx->session->stopbits;
	int paritybits = 0;

	
	if (rx->session->paritytype) {
		paritybits = 1;
	}

	/* modem recognized us and starts responding through sending its pilot signal */
	if (rx->state->answertone <= 0) {
		if (bit == SIG_STATUS_CARRIER_UP) {
			ast_log(LOG_NOTICE, "ANSWERTONE A %d",bit);
			rx->state->answertone = 0;
		} else if (bit == 1 && rx->state->answertone == 0) {
			ast_log(LOG_NOTICE, "ANSWERTONE B %d",bit);
			rx->state->answertone = 1;
		}
	} else if (bit != 1 && bit != 0) {
		/* ignore other spandsp-stuff */
		ast_debug(1, "Bit is %d? Ignoring!\n", bit);
	} else {
		/* insert bit into our bitbuffer */
		rx->bitbuffer[rx->writepos] = bit;
		rx->writepos++;
		if (rx->writepos >= MODEM_BITBUFFER_SIZE) {
			rx->writepos = 0;
		}
		if (rx->fill < MODEM_BITBUFFER_SIZE) {
			rx->fill++;
		} else {
			/* our bitbuffer is full, this probably won't happen */
			ast_debug(3, "full buffer!\n");
			rx->readpos++;
			if (rx->readpos >= MODEM_BITBUFFER_SIZE) {
				rx->readpos = 0;
			}
		}

		/* full byte = 1 startbit + databits + paritybits + stopbits */
		while (rx->fill >= (1 + databits + paritybits + stopbits)) {
			if (rx->bitbuffer[rx->readpos] == 0) { /* check for startbit */
				stop = (rx->readpos + 1 + paritybits + databits) % MODEM_BITBUFFER_SIZE;
				stop2 = (rx->readpos + 2 + paritybits + databits) % MODEM_BITBUFFER_SIZE;
				if ((rx->bitbuffer[stop] == 1) && (stopbits == 1 || (stopbits == 2 && rx->bitbuffer[stop2] == 1))) { /* check for stopbit -> valid framing */
					char byte = 0;
					for (i = 0; i < databits; i++) {	/* generate byte */
						if (rx->bitbuffer[(rx->readpos + 1 + i) % MODEM_BITBUFFER_SIZE]) {
							byte |= (1 << i);
						}
					}

					if (!paritybits || (paritybits && (rx->bitbuffer[(rx->readpos + databits + 1) % MODEM_BITBUFFER_SIZE] == ((rx->session->paritytype == 2) ^ __builtin_parity(byte))))) {
						ast_debug(7, "send: %d, %c\n", rx->sock, byte);
						send(rx->sock, &byte, 1, 0);
						
					} /* else invalid parity, ignore byte */
					rx->readpos= (rx->readpos + 10) % MODEM_BITBUFFER_SIZE; /* XXX Why does this increment by 10? */
					rx->fill -= 10;
				} else { /* no valid framing (no stopbit), remove first bit and maybe try again */
					rx->fill--;
					rx->readpos++;
					rx->readpos %= MODEM_BITBUFFER_SIZE;
				}
			} else { /* no valid framing either (no startbit) */
				rx->fill--;
				rx->readpos++;
				rx->readpos %= MODEM_BITBUFFER_SIZE;
			}
		}
	}
}



/*! \brief spandsp asks us for a bit to send onto the line */
static int modem_get_bit(void *user_data)
{
	modem_data *tx = (modem_data*) user_data;
	char byte = 0;
	int i, rc;

	int databits = tx->session->databits;
	int stopbits = tx->session->stopbits;
	int paritybits = 0;

	if (tx->session->paritytype) {
		paritybits = 1;
	}


	/* no new data in send (bit)buffer,
	 * either we just picked up the line, the terminal started to respond,
	 * than we check for new data on the socket
	 * or there's no new data, so we send 1s (mark) */
	if (tx->writepos == tx->readpos) {
		if (tx->state->nulsent > 0) {	/* connection is established, look for data on socket */
			rc = recv(tx->sock, &byte, 1, 0);

			if (rc > 0) {
				/* new data on socket, we put that byte into our bitbuffer */
				for (i = 0; i < (databits + paritybits + stopbits); i++) {
					if (paritybits && (i == databits) ) {
						tx->bitbuffer[tx->writepos] = (tx->session->paritytype == 2) ^ __builtin_parity( byte);
					} else if (i >= databits) {
						tx->bitbuffer[tx->writepos] = 1;	/* stopbits */
					} else { /* databits */

						if (byte & (1 << i)) {
							tx->bitbuffer[tx->writepos] = 1;
						} else {
							tx->bitbuffer[tx->writepos] = 0;
						}
					}
					tx->writepos++;
					if (tx->writepos >= MODEM_BITBUFFER_SIZE) {
						tx->writepos = 0;
					}
				}
				return 0; /* return startbit immediately */
			} else if (rc == 0) {
				ast_log(LOG_WARNING, "Socket seems closed. Will hangup.\n");
				tx->session->finished = 1;
			}
		} else {
			int res;
			/* check if socket was closed before connection was terminated */
			res = recv(tx->sock, &byte, 1, MSG_PEEK);

			if (res == 0) {
				tx->session->finished = 1;
				return 1;
			}
			if (tx->state->answertone > 0) {
				tx->state->nulsent = 1;
				return 1;
			}
		}

		/* no new data on socket, NULL-byte already sent, send mark-frequency */
		return 1;
	} else {
		/* there still is data in the bitbuffer, so we just send that out */
		i = tx->bitbuffer[tx->readpos];
		tx->readpos++;
		if (tx->readpos >= MODEM_BITBUFFER_SIZE) {
			tx->readpos = 0;
		}
		return i;
	}
}

static void *modem_generator_alloc(struct ast_channel *chan, void *params)
{
	return params;
}

static int fsk_generator_generate(struct ast_channel *chan, void *data, int len, int samples)
{
	fsk_tx_state_t *tx = (fsk_tx_state_t*) data;
	uint8_t buffer[AST_FRIENDLY_OFFSET + MAX_SAMPLES * sizeof(uint16_t)];
	int16_t *buf = (int16_t *) (buffer + AST_FRIENDLY_OFFSET);

	struct ast_frame outf = {
		.frametype = AST_FRAME_VOICE,
		.subclass.format = ast_format_slin,
		.src = __FUNCTION__,
	};

	if (samples > MAX_SAMPLES) {
		ast_log(LOG_WARNING, "Only generating %d samples, where %d requested\n", MAX_SAMPLES, samples);
		samples = MAX_SAMPLES;
	}

	if ((len = fsk_tx(tx, buf, samples)) > 0) {
		outf.samples = len;
		AST_FRAME_SET_BUFFER(&outf, buffer, AST_FRIENDLY_OFFSET, len * sizeof(int16_t));

		if (ast_write(chan, &outf) < 0) {
			ast_log(LOG_WARNING, "Failed to write frame to %s: %s\n", ast_channel_name(chan), strerror(errno));
			return -1;
		}
	}

	return 0;
}


struct ast_generator fsk_generator = {
	alloc:		modem_generator_alloc,
	generate: 	fsk_generator_generate,
};


static int softmodemminipavi_communicate(modem_session *s)
{
	int res = -1;
	struct ast_format *original_read_fmt;
	struct ast_format *original_write_fmt;

	modem_data rxdata, txdata;

	struct ast_frame *inf = NULL;

	fsk_tx_state_t *modem_tx = NULL;
	fsk_rx_state_t *modem_rx = NULL;

	int sock;
	struct sockaddr_in server;
	struct hostent *hp;
	struct ast_hostent ahp;
	connection_state state;


	/* Used for TDD only */
	
	
	char bufcid[100] = " ";

	original_read_fmt = ast_channel_readformat(s->chan);
	if (original_read_fmt != ast_format_slin) {
		res = ast_set_read_format(s->chan, ast_format_slin);
		if (res < 0) {
			ast_log(LOG_NOTICE, "Unable to set to linear read mode on %s\n", ast_channel_name(s->chan));
			return res;
		}
	}

	original_write_fmt = ast_channel_writeformat(s->chan);
	if (original_write_fmt != ast_format_slin) {
		res = ast_set_write_format(s->chan, ast_format_slin);
		if (res < 0) {
			ast_log(LOG_NOTICE, "Unable to set to linear write mode on %s\n", ast_channel_name(s->chan));
			return res;
		}
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		ast_log(LOG_ERROR, "Could not create socket: %s\n", strerror(errno));
		return res;
	}

	server.sin_family = AF_INET;
	hp = ast_gethostbyname(s->host, &ahp);
	memcpy((char *) &server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_port = htons(s->port);

	/* If the connect takes a while, we should autoservice the channel */
	ast_autoservice_start(s->chan);
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
		ast_log(LOG_ERROR, "Cannot connect to remote host: '%s': %s\n", s->host, strerror(errno));
		ast_autoservice_stop(s->chan);
		return res;
	}
	
	ast_log(LOG_NOTICE, "CONNECTED to remote host: '%s port %d'\n", s->host,s->port);
	ast_autoservice_stop(s->chan);
	

	if (!strstr( s->callerinfo, "TO " )) {
		// Call received
		ast_log(LOG_NOTICE, ">Appel ENTRANT - CALLFROM %s (PCE=%d) STARTURL %s",s->callerinfo,s->pce,s->starturl);
		// First, send a 3 seconds 2100Hz carrier + 75ms silence
		ast_tonepair(s->chan, 2100, 2100, 3000, 500);	
		usleep(75000);
		sprintf(bufcid,"CALLFROM %s\nSTARTURL %s\nPCE %d\n",s->callerinfo,s->starturl,s->pce);	// Send caller number ans service url to Minipavi 
	} else {
		// Outbound call 
		ast_log(LOG_NOTICE, ">Appel SORTANT");
		sprintf(bufcid,"CALLTO %s\nPID %s\n\n",&s->callerinfo[3],s->starturl);	// Send called number and namle of local unix socket for communication with initiating process
	}
	fcntl(sock, F_SETFL, O_NONBLOCK);
	send(sock, bufcid,strlen(bufcid), 0);		

	state.answertone = -1; /* no carrier yet */
	state.nulsent = 0;

	rxdata.sock = sock;
	rxdata.writepos = 0;
	rxdata.readpos = 0;
	rxdata.fill = 0;
	rxdata.state = &state;
	rxdata.session = s;


	txdata.sock = sock;
	txdata.writepos = 0;
	txdata.readpos = 0;
	txdata.fill = 0;
	txdata.state = &state;
	txdata.session = s;


	/* initialise spandsp-stuff, give it our callback functions */
	if (s->flipmode) {
		modem_tx = fsk_tx_init(NULL, &preset_fsk_specs[FSK_V23CH2], modem_get_bit, &txdata);
		modem_rx = fsk_rx_init(NULL, &preset_fsk_specs[FSK_V23CH1], FSK_FRAME_MODE_SYNC, modem_put_bit, &rxdata);
	} else {
		modem_tx = fsk_tx_init(NULL, &preset_fsk_specs[FSK_V23CH1], modem_get_bit, &txdata);
		modem_rx = fsk_rx_init(NULL, &preset_fsk_specs[FSK_V23CH2], FSK_FRAME_MODE_SYNC, modem_put_bit, &rxdata);
	}
	fsk_tx_power (modem_tx, s->txpower);
	fsk_rx_set_signal_cutoff(modem_rx, s->rxcutoff);

	ast_activate_generator(s->chan, &fsk_generator, modem_tx);

	while (!s->finished) {
		res = ast_waitfor(s->chan, 20);
		if (res < 0) {
			break;
		} else if (res > 0) {
			res = 0;
		}

		inf = ast_read(s->chan);
		if (!inf) {
			ast_debug(1, "Channel hangup\n");
			if (recv(sock, &res, 1, MSG_PEEK) != 0) {
				ast_debug(1, "Closing socket\n");
				ast_log(LOG_NOTICE, "Fermeture Socket");
				close(sock);
			}
			res = -1;
			break;
		}

		/* Check the frame type. Format also must be checked because there is a chance
		   that a frame in old format was already queued before we set chanel format
		   to slinear so it will still be received by ast_read */
		   
		if (inf->frametype == AST_FRAME_VOICE && inf->subclass.format == ast_format_slin) {
			if (fsk_rx(modem_rx, inf->data.ptr, inf->samples) < 0) {
				/* I know fsk_rx never returns errors. The check here is for good style only */
				ast_log(LOG_NOTICE, "softmodem returned error\n");
				res = -1;
				break;
			}
		}

		ast_frfree(inf);
		inf = NULL;
	}



	if (original_write_fmt != ast_format_slin) {
		if (ast_set_write_format(s->chan, original_write_fmt) < 0) {
			ast_log(LOG_NOTICE, "Unable to restore write format on '%s'\n", ast_channel_name(s->chan));
		}
	}

	if (original_read_fmt != ast_format_slin) {
		if (ast_set_read_format(s->chan, original_read_fmt) < 0) {
			ast_log(LOG_NOTICE, "Unable to restore read format on '%s'\n", ast_channel_name(s->chan));
		}
	}

	return res;
}

static int softmodemminipavi_exec(struct ast_channel *chan, const char *data)
{
	int res = 0;
	char *parse;
	modem_session session;
	struct ast_flags options = { 0, };
	char *option_args[OPT_ARG_ARRAY_SIZE];

	AST_DECLARE_APP_ARGS(args,
		AST_APP_ARG(host);
		AST_APP_ARG(port);
		AST_APP_ARG(callerinfo);
		AST_APP_ARG(starturl);
		AST_APP_ARG(pce);
		AST_APP_ARG(options);
	);

	if (!chan) {
		ast_log(LOG_ERROR, "Channel is NULL.\n");
		return -1;
	}

	/* answer channel if not already answered */
	res = ast_auto_answer(chan);
	if (res) {
		ast_log(LOG_WARNING, "Could not answer channel '%s'\n", ast_channel_name(chan));
		return res;
	}

	/* Set defaults */
	session.chan = chan;
	session.finished = 0;
	session.rxcutoff = -35.0f;
	session.txpower = -28.0f;
	

	session.databits = 7;
	session.stopbits = 1;
	session.paritytype = 1;

	

	parse = ast_strdupa(S_OR(data, ""));
	AST_STANDARD_APP_ARGS(args, parse);

	if (args.host) {
		session.host = ast_strip(args.host); /* if there are spaces in the hostname, we crash at the memcpy after hp = ast_gethostbyname(s->host, &ahp); */
		if (strcmp(session.host, args.host)) {
			ast_log(LOG_WARNING, "Please avoid spaces in the hostname: '%s'\n", args.host);
		}
	} else {
		session.host = "localhost";
	}

	if (args.port) {
		session.port = atoi(args.port);
		if ((session.port < 0) || (session.port > 65535)) {
			ast_log(LOG_ERROR, "Please specify a valid port number.\n");
			return -1;
		}
	} else {
		session.port = 23;
	}

	if (args.callerinfo) {
		session.callerinfo = ast_strip(args.callerinfo);
	} else {
		session.callerinfo = "?";
	}

	if (args.starturl) {
		session.starturl = ast_strip(args.starturl);
	} else {
		session.starturl = "";
	}

	if (args.pce) {
		session.pce = atoi(args.pce);
	} else {
		session.pce = 0;
	}


	if (args.options) {
		ast_app_parse_options(additional_options, &options, option_args, args.options);

		if (ast_test_flag(&options, OPT_RX_CUTOFF) && !ast_strlen_zero(option_args[OPT_ARG_RX_CUTOFF])) {
			session.rxcutoff = atof(option_args[OPT_ARG_RX_CUTOFF]);
		}

		if (ast_test_flag(&options, OPT_TX_POWER) && !ast_strlen_zero(option_args[OPT_ARG_TX_POWER])) {
			session.txpower = atof(option_args[OPT_ARG_TX_POWER]);
		}
		session.flipmode = ast_test_flag(&options, OPT_FLIP_MODE) ? 1 : 0;
	}

	res = softmodemminipavi_communicate(&session);
	return res;
}

static int unload_module(void)
{
	return ast_unregister_application(app);
}

static int load_module(void)
{
	return ast_register_application_xml(app, softmodemminipavi_exec);
}

AST_MODULE_INFO_STANDARD_EXTENDED(ASTERISK_GPL_KEY, "SoftmodemMinipavi (V23)");
