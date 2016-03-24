/*
 * libwebsockets-test-server - libwebsockets test implementation
 *
 * Copyright (C) 2010-2016 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The person who associated a work with this deed has dedicated
 * the work to the public domain by waiving all of his or her rights
 * to the work worldwide under copyright law, including all related
 * and neighboring rights, to the extent allowed by law. You can copy,
 * modify, distribute and perform the work, even for commercial purposes,
 * all without asking permission.
 *
 * The test apps are intended to be adapted for use in your code, which
 * may be proprietary.  So unlike the library itself, they are licensed
 * Public Domain.
 */

#include "test-server.h"
#include <pthread.h>

int close_testing;
int max_poll_elements;
int debug_level = 7;

#ifdef EXTERNAL_POLL
struct lws_pollfd *pollfds;
int *fd_lookup;
int count_pollfds;
#endif
volatile int force_exit = 0;
struct lws_context *context;

/*
 * This mutex lock protects code that changes or relies on wsi list outside of
 * the service thread.  The service thread will acquire it when changing the
 * wsi list and other threads should acquire it while dereferencing wsis or
 * calling apis like lws_callback_on_writable_all_protocol() which
 * use the wsi list and wsis from a different thread context.
 */
pthread_mutex_t lock_established_conns;

/* http server gets files from this path */
//#define LOCAL_RESOURCE_PATH INSTALL_DATADIR"/libwebsockets-test-server"
char *resource_path = "";//LOCAL_RESOURCE_PATH;

#define MAX_MESSAGE_QUEUE 512
#define PARAM_CHANNEL			"channel"
#define PARAM_MODE				"mode"


struct a_message {
	void *payload;
	size_t len;
};

static struct a_message ringbuffer[MAX_MESSAGE_QUEUE];
static int ringbuffer_head;


const char * get_mimetype(const char *file)
{
	int n = strlen(file);

	if (n < 5)
		return NULL;

	if (!strcmp(&file[n - 4], ".ico"))
		return "image/x-icon";

	if (!strcmp(&file[n - 4], ".png"))
		return "image/png";

	if (!strcmp(&file[n - 5], ".html"))
		return "text/html";

	return NULL;
}



int
callback_lws_mirror(struct lws *wsi, enum lws_callback_reasons reason,
		    void *user, void *in, size_t len)
{
#if 0
	struct per_session_data__lws_mirror *pss =
			(struct per_session_data__lws_mirror *)user;
	int n, m;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("%s: LWS_CALLBACK_ESTABLISHED\n", __func__);
		pss->ringbuffer_tail = ringbuffer_head;
		pss->wsi = wsi;
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		lwsl_notice("%s: mirror protocol cleaning up\n", __func__);
		for (n = 0; n < sizeof ringbuffer / sizeof ringbuffer[0]; n++)
			if (ringbuffer[n].payload)
				free(ringbuffer[n].payload);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (close_testing)
			break;
		while (pss->ringbuffer_tail != ringbuffer_head) {
			m = ringbuffer[pss->ringbuffer_tail].len;
			n = lws_write(wsi, (unsigned char *)
				   ringbuffer[pss->ringbuffer_tail].payload +
				   LWS_PRE, m, LWS_WRITE_TEXT);
			if (n < 0) {
				lwsl_err("ERROR %d writing to mirror socket\n", n);
				return -1;
			}
			if (n < m)
				lwsl_err("mirror partial write %d vs %d\n", n, m);

			if (pss->ringbuffer_tail == (MAX_MESSAGE_QUEUE - 1))
				pss->ringbuffer_tail = 0;
			else
				pss->ringbuffer_tail++;

			if (((ringbuffer_head - pss->ringbuffer_tail) &
			    (MAX_MESSAGE_QUEUE - 1)) == (MAX_MESSAGE_QUEUE - 15))
				lws_rx_flow_allow_all_protocol(lws_get_context(wsi),
					       lws_get_protocol(wsi));

			if (lws_send_pipe_choked(wsi)) {
				lws_callback_on_writable(wsi);
				break;
			}
		}
		break;

	case LWS_CALLBACK_RECEIVE:
		if (((ringbuffer_head - pss->ringbuffer_tail) &
		    (MAX_MESSAGE_QUEUE - 1)) == (MAX_MESSAGE_QUEUE - 1)) {
			lwsl_err("dropping!\n");
			goto choke;
		}

		if (ringbuffer[ringbuffer_head].payload)
			free(ringbuffer[ringbuffer_head].payload);

		ringbuffer[ringbuffer_head].payload = malloc(LWS_PRE + len);
		ringbuffer[ringbuffer_head].len = len;
		memcpy((char *)ringbuffer[ringbuffer_head].payload +
		       LWS_PRE, in, len);
		if (ringbuffer_head == (MAX_MESSAGE_QUEUE - 1))
			ringbuffer_head = 0;
		else
			ringbuffer_head++;

		if (((ringbuffer_head - pss->ringbuffer_tail) &
		    (MAX_MESSAGE_QUEUE - 1)) != (MAX_MESSAGE_QUEUE - 2))
			goto done;

choke:
		lwsl_debug("LWS_CALLBACK_RECEIVE: throttling %p\n", wsi);
		lws_rx_flow_control(wsi, 0);

done:
		lws_callback_on_writable_all_protocol(lws_get_context(wsi),
						      lws_get_protocol(wsi));
		break;

	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}
#endif
	return 0;
}


void
dump_handshake_info(struct lws *wsi)
{
	int n = 0, len;
	char buf[256];
	const unsigned char *c;

	do {
		c = lws_token_to_string(n);
		if (!c) {
			n++;
			continue;
		}

		len = lws_hdr_total_length(wsi, n);
		if (!len || len > sizeof(buf) - 1) {
			n++;
			continue;
		}

		lws_hdr_copy(wsi, buf, sizeof buf, n);
		buf[sizeof(buf) - 1] = '\0';

		fprintf(stderr, "    %s = %s\n", (char *)c, buf);
		n++;
	} while (c);
}


int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user,
		  void *in, size_t len)
{
	struct per_session_data__http *pss =
			(struct per_session_data__http *)user;
	#if 0
	unsigned char buffer[4096 + LWS_PRE];
	unsigned long amount, file_len, sent;
	char leaf_path[1024];
	const char *mimetype;
	char *other_headers;
	unsigned char *end;
	struct timeval tv;
	unsigned char *p;
	char buf[256];
	char b64[64];
	int n, m;
#ifdef EXTERNAL_POLL
	struct lws_pollargs *pa = (struct lws_pollargs *)in;
#endif

	switch (reason) {
	case LWS_CALLBACK_HTTP:

		if (debug_level & LLL_INFO) {
			dump_handshake_info(wsi);

			/* dump the individual URI Arg parameters */
			n = 0;
			while (lws_hdr_copy_fragment(wsi, buf, sizeof(buf),
						     WSI_TOKEN_HTTP_URI_ARGS, n) > 0) {
				lwsl_notice("URI Arg %d: %s\n", ++n, buf);
			}
		}

		{
			char name[100], rip[50];
			lws_get_peer_addresses(wsi, lws_get_socket_fd(wsi), name,
					       sizeof(name), rip, sizeof(rip));
			sprintf(buf, "%s (%s)", name, rip);
			lwsl_notice("HTTP connect from %s\n", buf);
		}

		if (len < 1) {
			lws_return_http_status(wsi,
						HTTP_STATUS_BAD_REQUEST, NULL);
			goto try_to_reuse;
		}

		/* this example server has no concept of directories */
		if (strchr((const char *)in + 1, '/')) {
			lws_return_http_status(wsi, HTTP_STATUS_FORBIDDEN, NULL);
			goto try_to_reuse;
		}

#ifdef LWS_WITH_CGI
		if (!strcmp(in, "/cgitest")) {
			static char *cmd[] = {
				"/bin/sh",
				"-c",
				INSTALL_DATADIR"/libwebsockets-test-server/lws-cgi-test.sh",
				NULL
			};

			lwsl_notice("%s: cgitest\n", __func__);
			n = lws_cgi(wsi, cmd, 5);
			if (n) {
				lwsl_err("%s: cgi failed\n");
				return -1;
			}
			p = buffer + LWS_PRE;
			end = p + sizeof(buffer) - LWS_PRE;

			if (lws_add_http_header_status(wsi, 200, &p, end))
				return 1;
			if (lws_add_http_header_by_token(wsi,
					WSI_TOKEN_HTTP_CONTENT_TYPE,
					(unsigned char *)"text/plain",
					10, &p, end))
				return 1;
			if (lws_add_http_header_by_token(wsi, WSI_TOKEN_CONNECTION,
					(unsigned char *)"close", 5, &p, end))
				return 1;
			if (lws_finalize_http_header(wsi, &p, end))
				return 1;
			n = lws_write(wsi, buffer + LWS_PRE,
				      p - (buffer + LWS_PRE),
				      LWS_WRITE_HTTP_HEADERS);
			break;
		}
#endif

		/* if a legal POST URL, let it continue and accept data */
		if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI))
			return 0;

		/* check for the "send a big file by hand" example case */

		if (!strcmp((const char *)in, "/leaf.jpg")) {
			if (strlen(resource_path) > sizeof(leaf_path) - 10)
				return -1;
			sprintf(leaf_path, "%s/leaf.jpg", resource_path);

			/* well, let's demonstrate how to send the hard way */

			p = buffer + LWS_PRE;
			end = p + sizeof(buffer) - LWS_PRE;

			pss->fd = lws_plat_file_open(wsi, leaf_path, &file_len,
						     LWS_O_RDONLY);

			if (pss->fd == LWS_INVALID_FILE) {
				lwsl_err("faild to open file %s\n", leaf_path);
				return -1;
			}

			/*
			 * we will send a big jpeg file, but it could be
			 * anything.  Set the Content-Type: appropriately
			 * so the browser knows what to do with it.
			 *
			 * Notice we use the APIs to build the header, which
			 * will do the right thing for HTTP 1/1.1 and HTTP2
			 * depending on what connection it happens to be working
			 * on
			 */
			if (lws_add_http_header_status(wsi, 200, &p, end))
				return 1;
			if (lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_SERVER,
				    	(unsigned char *)"libwebsockets",
					13, &p, end))
				return 1;
			if (lws_add_http_header_by_token(wsi,
					WSI_TOKEN_HTTP_CONTENT_TYPE,
				    	(unsigned char *)"image/jpeg",
					10, &p, end))
				return 1;
			if (lws_add_http_header_content_length(wsi,
							       file_len, &p,
							       end))
				return 1;
			if (lws_finalize_http_header(wsi, &p, end))
				return 1;

			/*
			 * send the http headers...
			 * this won't block since it's the first payload sent
			 * on the connection since it was established
			 * (too small for partial)
			 *
			 * Notice they are sent using LWS_WRITE_HTTP_HEADERS
			 * which also means you can't send body too in one step,
			 * this is mandated by changes in HTTP2
			 */

			*p = '\0';
			lwsl_info("%s\n", buffer + LWS_PRE);

			n = lws_write(wsi, buffer + LWS_PRE,
				      p - (buffer + LWS_PRE),
				      LWS_WRITE_HTTP_HEADERS);
			if (n < 0) {
				lws_plat_file_close(wsi, pss->fd);
				return -1;
			}
			/*
			 * book us a LWS_CALLBACK_HTTP_WRITEABLE callback
			 */
			lws_callback_on_writable(wsi);
			break;
		}

		/* if not, send a file the easy way */
		strcpy(buf, resource_path);
		if (strcmp(in, "/")) {
			if (*((const char *)in) != '/')
				strcat(buf, "/");
			strncat(buf, in, sizeof(buf) - strlen(resource_path));
		} else /* default file to serve */
			strcat(buf, "/index.html");
		buf[sizeof(buf) - 1] = '\0';

		/* refuse to serve files we don't understand */
		mimetype = get_mimetype(buf);
		if (!mimetype) {
			lwsl_err("Unknown mimetype for %s\n", buf);
			lws_return_http_status(wsi,
				      HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL);
			return -1;
		}

		/* demonstrates how to set a cookie on / */

		other_headers = leaf_path;
		p = (unsigned char *)leaf_path;
		if (!strcmp((const char *)in, "/") &&
			   !lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_COOKIE)) {
			/* this isn't very unguessable but it'll do for us */
			gettimeofday(&tv, NULL);
			n = sprintf(b64, "test=LWS_%u_%u_COOKIE;Max-Age=360000",
				(unsigned int)tv.tv_sec,
				(unsigned int)tv.tv_usec);

			if (lws_add_http_header_by_name(wsi,
				(unsigned char *)"set-cookie:",
				(unsigned char *)b64, n, &p,
				(unsigned char *)leaf_path + sizeof(leaf_path)))
				return 1;
		}
		if (lws_is_ssl(wsi) && lws_add_http_header_by_name(wsi,
						(unsigned char *)
						"Strict-Transport-Security:",
						(unsigned char *)
						"max-age=15768000 ; "
						"includeSubDomains", 36, &p,
						(unsigned char *)leaf_path +
							sizeof(leaf_path)))
			return 1;
		n = (char *)p - leaf_path;

		n = lws_serve_http_file(wsi, buf, mimetype, other_headers, n);
		if (n < 0 || ((n > 0) && lws_http_transaction_completed(wsi)))
			return -1; /* error or can't reuse connection: close the socket */

		/*
		 * notice that the sending of the file completes asynchronously,
		 * we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback when
		 * it's done
		 */
		break;

	case LWS_CALLBACK_HTTP_BODY:
		strncpy(buf, in, 20);
		buf[20] = '\0';
		if (len < 20)
			buf[len] = '\0';

		lwsl_notice("LWS_CALLBACK_HTTP_BODY: %s... len %d\n",
				(const char *)buf, (int)len);

		break;

	case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		lwsl_notice("LWS_CALLBACK_HTTP_BODY_COMPLETION\n");
		/* the whole of the sent body arrived, close or reuse the connection */
		lws_return_http_status(wsi, HTTP_STATUS_OK, NULL);
		goto try_to_reuse;

	case LWS_CALLBACK_HTTP_FILE_COMPLETION:
		goto try_to_reuse;

	case LWS_CALLBACK_HTTP_WRITEABLE:
		lwsl_info("LWS_CALLBACK_HTTP_WRITEABLE\n");

		if (pss->fd == LWS_INVALID_FILE)
			goto try_to_reuse;
#ifdef LWS_WITH_CGI
		if (pss->reason_bf) {
			lwsl_debug("%s: stdout\n", __func__);
			n = read(lws_get_socket_fd(pss->args.stdwsi[LWS_STDOUT]),
					buf + LWS_PRE, sizeof(buf) - LWS_PRE);
			//lwsl_notice("read %d (errno %d)\n", n, errno);
			if (n < 0 && errno != EAGAIN)
				return -1;
			if (n > 0) {
				m = lws_write(wsi, (unsigned char *)buf + LWS_PRE, n,
					      LWS_WRITE_HTTP);
				//lwsl_notice("write %d\n", m);
				if (m < 0)
					goto bail;
				pss->reason_bf = 0;
			}
			break;
		}
#endif
		/*
		 * we can send more of whatever it is we were sending
		 */
		sent = 0;
		do {
			/* we'd like the send this much */
			n = sizeof(buffer) - LWS_PRE;

			/* but if the peer told us he wants less, we can adapt */
			m = lws_get_peer_write_allowance(wsi);

			/* -1 means not using a protocol that has this info */
			if (m == 0)
				/* right now, peer can't handle anything */
				goto later;

			if (m != -1 && m < n)
				/* he couldn't handle that much */
				n = m;

			n = lws_plat_file_read(wsi, pss->fd,
					       &amount, buffer + LWS_PRE, n);
			/* problem reading, close conn */
			if (n < 0) {
				lwsl_err("problem reading file\n");
				goto bail;
			}
			n = (int)amount;
			/* sent it all, close conn */
			if (n == 0)
				goto penultimate;
			/*
			 * To support HTTP2, must take care about preamble space
			 *
			 * identification of when we send the last payload frame
			 * is handled by the library itself if you sent a
			 * content-length header
			 */
			m = lws_write(wsi, buffer + LWS_PRE, n, LWS_WRITE_HTTP);
			if (m < 0) {
				lwsl_err("write failed\n");
				/* write failed, close conn */
				goto bail;
			}
			if (m) /* while still active, extend timeout */
				lws_set_timeout(wsi, PENDING_TIMEOUT_HTTP_CONTENT, 5);
			sent += m;

		} while (!lws_send_pipe_choked(wsi) && (sent < 1024 * 1024));
later:
		lws_callback_on_writable(wsi);
		break;
penultimate:
		lws_plat_file_close(wsi, pss->fd);
		pss->fd = LWS_INVALID_FILE;
		goto try_to_reuse;

bail:
		lws_plat_file_close(wsi, pss->fd);

		return -1;

	/*
	 * callback for confirming to continue with client IP appear in
	 * protocol 0 callback since no websocket protocol has been agreed
	 * yet.  You can just ignore this if you won't filter on client IP
	 * since the default uhandled callback return is 0 meaning let the
	 * connection continue.
	 */
	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
		/* if we returned non-zero from here, we kill the connection */
		break;

#ifdef LWS_WITH_CGI
	/* CGI IO events (POLLIN/OUT) appear here our demo user code policy is
	 *
	 *  - POST data goes on subprocess stdin
	 *  - subprocess stdout goes on http via writeable callback
	 *  - subprocess stderr goes to the logs
	 */
	case LWS_CALLBACK_CGI:
		pss->args = *((struct lws_cgi_args *)in);
		//lwsl_notice("LWS_CALLBACK_CGI: ch %d\n", pss->args.ch);
		switch (pss->args.ch) { /* which of stdin/out/err ? */
		case LWS_STDIN:
			/* TBD stdin rx flow control */
			break;
		case LWS_STDOUT:
			pss->reason_bf |= 1 << pss->args.ch;
			/* when writing to MASTER would not block */
			lws_callback_on_writable(wsi);
			break;
		case LWS_STDERR:
			n = read(lws_get_socket_fd(pss->args.stdwsi[LWS_STDERR]),
					buf, 127);
			//lwsl_notice("stderr reads %d\n", n);
			if (n > 0) {
				if (buf[n - 1] != '\n')
					buf[n++] = '\n';
				buf[n] = '\0';
				lwsl_notice("CGI-stderr: %s\n", buf);
			}
			break;
		}
		break;

	case LWS_CALLBACK_CGI_TERMINATED:
		lwsl_notice("LWS_CALLBACK_CGI_TERMINATED\n");
		/* because we sent on openended http, close the connection */
		return -1;

	case LWS_CALLBACK_CGI_STDIN_DATA:  /* POST body for stdin */
		lwsl_notice("LWS_CALLBACK_CGI_STDIN_DATA\n");
		pss->args = *((struct lws_cgi_args *)in);
		n = write(lws_get_socket_fd(pss->args.stdwsi[LWS_STDIN]),
			  pss->args.data, pss->args.len);
		lwsl_notice("LWS_CALLBACK_CGI_STDIN_DATA: write says %d", n);
		if (n < pss->args.len)
			lwsl_notice("LWS_CALLBACK_CGI_STDIN_DATA: sent %d only %d went",
					n, pss->args.len);
		return n;
#endif

	/*
	 * callbacks for managing the external poll() array appear in
	 * protocol 0 callback
	 */

	case LWS_CALLBACK_LOCK_POLL:
		/*
		 * lock mutex to protect pollfd state
		 * called before any other POLL related callback
		 * if protecting wsi lifecycle change, len == 1
		 */
		test_server_lock(len);
		break;

	case LWS_CALLBACK_UNLOCK_POLL:
		/*
		 * unlock mutex to protect pollfd state when
		 * called after any other POLL related callback
		 * if protecting wsi lifecycle change, len == 1
		 */
		test_server_unlock(len);
		break;

#ifdef EXTERNAL_POLL
	case LWS_CALLBACK_ADD_POLL_FD:

		if (count_pollfds >= max_poll_elements) {
			lwsl_err("LWS_CALLBACK_ADD_POLL_FD: too many sockets to track\n");
			return 1;
		}

		fd_lookup[pa->fd] = count_pollfds;
		pollfds[count_pollfds].fd = pa->fd;
		pollfds[count_pollfds].events = pa->events;
		pollfds[count_pollfds++].revents = 0;
		break;

	case LWS_CALLBACK_DEL_POLL_FD:
		if (!--count_pollfds)
			break;
		m = fd_lookup[pa->fd];
		/* have the last guy take up the vacant slot */
		pollfds[m] = pollfds[count_pollfds];
		fd_lookup[pollfds[count_pollfds].fd] = m;
		break;

	case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
	        pollfds[fd_lookup[pa->fd]].events = pa->events;
		break;
#endif

	case LWS_CALLBACK_GET_THREAD_ID:
		/*
		 * if you will call "lws_callback_on_writable"
		 * from a different thread, return the caller thread ID
		 * here so lws can use this information to work out if it
		 * should signal the poll() loop to exit and restart early
		 */

		/* return pthread_getthreadid_np(); */

		break;

	default:
		break;
	}

	return 0;

	/* if we're on HTTP1.1 or 2.0, will keep the idle connection alive */
try_to_reuse:
	if (lws_http_transaction_completed(wsi))
		return -1;
#endif
	return 0;
}

/*
 * multithreaded version - protect wsi lifecycle changes in the library
 * these are called from protocol 0 callbacks
 */
 extern void setTrigger(void);
extern int sendData(int fp, int channelNum, unsigned char* buf);
extern int getQueueLength(int channel);
extern void setADModeClient(int nMode);
extern void setChannelClient(int nChannel);
extern void stopADReadThread();

int	callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason,
				void *user, void *in, size_t len)
	{
	
	//unsigned char *senddata= NULL;
		unsigned char buf[LWS_PRE + 2048*10*5];\
		struct per_session_data__dumb_increment *pss =
				(struct per_session_data__dumb_increment *)user;
		unsigned char *p = &buf[LWS_PRE];
		int n, m, result, dataSize;
	
		char* pCmd;
		char tempBuf[3]={0,};
		int nChannel = -1;
		int nMode = -1;
		
		switch (reason) {
	
		case LWS_CALLBACK_ESTABLISHED:
			lwsl_err("established\n");
			pss->number = 0;
			break;
	
		case LWS_CALLBACK_SERVER_WRITEABLE:
//			lwsl_err("writable\n");
			//buf_size = LWS_SEND_BUFFER_PRE_PADDING+2048*sizeof(char)+LWS_SEND_BUFFER_POST_PADDING;
			//senddata = (unsigned char*)malloc(buf_size);
			//memset(senddata, 0, buf_size);
			//dataSize = getQueueLength(4);

			dataSize = sendData(NULL, 4, p);
//			if(p != NULL)
//				{
//			lwsl_err("enable Data\n");
//				}
//			else
//				{
//			lwsl_err("Data NULL\n");				
//				}
			//n = sprintf((char *)p, "%d", pss->number++);
			if(dataSize > 0)
			{
				m = lws_write(wsi, p, dataSize*2048, LWS_WRITE_BINARY);
			
				if (m < n) {
					lwsl_err("ERROR %d writing to di socket\n", m);
					return -1;
				}
				else
					{
					lwsl_err("SUCCESS dataSize: %d ,  %dbyte writing to di socket\n", dataSize, m);
					}
				//free(senddata);
			}
			else
			{
				lwsl_err("Under 50 Count Data di socket\n");
			}
			if (close_testing && pss->number == 50) {
				lwsl_info("close tesing limit, closing\n");
				return -1;
			}
			break;
	
		case LWS_CALLBACK_RECEIVE:
			
			
			lwsl_err("receivced: %s\n", (char*)in);

			if (strcmp((const char *)in, "reset\n") == 0)
				pss->number = 0;

			if(strstr((const char *)in, "start") != NULL)
			{
				if( (pCmd = strstr((const char *)in, PARAM_CHANNEL)) != NULL)
				{
					strncpy(tempBuf, pCmd+strlen(PARAM_CHANNEL)+1, 1);
					nChannel = atoi(tempBuf);
					setChannelClient(nChannel);
					lwsl_err("recevied channel : %d\n", nChannel);
				}
					

				if( (pCmd = strstr((const char *)in, PARAM_MODE)) != NULL)
				{
					memset(tempBuf, 0, sizeof(tempBuf));
					strncpy(tempBuf, pCmd+strlen(PARAM_MODE)+1, 1);
					nMode = atoi(tempBuf);
					setADModeClient(nMode);
					lwsl_err("recevied nMode : %d\n", nMode);
					
					setTrigger();
				}
			}
			if(strstr((const char *)in, "stop") != NULL)
			{
				stopADReadThread();
				
				lwsl_err("recevied stop : %s\n", in);
			}
			if (strcmp((const char *)in, "closeme\n") == 0) {
				lwsl_notice("dumb_inc: closing as requested\n");
				lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY,
						 (unsigned char *)"seeya", 5);
				return -1;
			}
			break;
		/*
		 * this just demonstrates how to use the protocol filter. If you won't
		 * study and reject connections based on header content, you don't need
		 * to handle this callback
		 */
		case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
			
			lwsl_err("handshake\n");
			dump_handshake_info(wsi);
			/* you could return non-zero here and kill the connection */
			break;
	
		/*
		 * this just demonstrates how to handle
		 * LWS_CALLBACK_WS_PEER_INITIATED_CLOSE and extract the peer's close
		 * code and auxiliary data.  You can just not handle it if you don't
		 * have a use for this.
		 */
		case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
			lwsl_notice("LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: len %d\n",
					len);
			for (n = 0; n < (int)len; n++)
				lwsl_notice(" %d: 0x%02X\n", n,
						((unsigned char *)in)[n]);
			break;
	
		default:
			break;
		}
	
		return 0;
	}

void test_server_lock(int care)
{
	if (care)
		pthread_mutex_lock(&lock_established_conns);
}
void test_server_unlock(int care)
{
	if (care)
		pthread_mutex_unlock(&lock_established_conns);
}

/*
 * This demo server shows how to use libwebsockets for one or more
 * websocket protocols in the same server
 *
 * It defines the following websocket protocols:
 *
 *  dumb-increment-protocol:  once the socket is opened, an incrementing
 *				ascii string is sent down it every 50ms.
 *				If you send "reset\n" on the websocket, then
 *				the incrementing number is reset to 0.
 *
 *  lws-mirror-protocol: copies any received packet to every connection also
 *				using this protocol, including the sender
 */

enum demo_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_DUMB_INCREMENT,
	PROTOCOL_LWS_MIRROR,

	/* always last */
	DEMO_PROTOCOL_COUNT
};

/* list of supported protocols and callbacks */

static struct lws_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		sizeof (struct per_session_data__http),	/* per_session_data_size */
		0,			/* max frame size / rx buffer */
	},
	{
		"dumb-increment-protocol",
		callback_dumb_increment,
		sizeof(struct per_session_data__dumb_increment),
		65536,
	},
	{
		"lws-mirror-protocol",
		callback_lws_mirror,
		sizeof(struct per_session_data__lws_mirror),
		128,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

void *thread_dumb_increment(void *threadid)
{
	while (!force_exit) {
		/*
		 * this lock means wsi in the active list cannot
		 * disappear underneath us, because the code to add and remove
		 * them is protected by the same lock
		 */
		pthread_mutex_lock(&lock_established_conns);
		lws_callback_on_writable_all_protocol(context,
				&protocols[PROTOCOL_DUMB_INCREMENT]);
		pthread_mutex_unlock(&lock_established_conns);
		usleep(5000);
	}

	pthread_exit(NULL);
}

void *thread_service(void *threadid)
{
	while (lws_service_tsi(context, 1000, (int)(long)threadid) >= 0 && !force_exit)
		;

	pthread_exit(NULL);
}

void sighandler(int sig)
{
	force_exit = 1;
	lws_cancel_service(context);
}

static const struct lws_extension exts[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate; client_no_context_takeover; client_max_window_bits"
	},
	{
		"deflate-frame",
		lws_extension_callback_pm_deflate,
		"deflate_frame"
	},
	{ NULL, NULL, NULL /* terminator */ }
};

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "debug",	required_argument,	NULL, 'd' },
	{ "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "allow-non-ssl",	no_argument,	NULL, 'a' },
	{ "interface",  required_argument,	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },
	{ "libev",  no_argument,		NULL, 'e' },
	{ "threads",  required_argument,	NULL, 'j' },
#ifndef LWS_NO_DAEMONIZE
	{ "daemonize", 	no_argument,		NULL, 'D' },
#endif
	{ "resource_path", required_argument,	NULL, 'r' },
	{ NULL, 0, 0, 0 }
};

int testmain(void)
{
	struct lws_context_creation_info info;
	char interface_name[128] = "";
	const char *iface = NULL;
	pthread_t pthread_dumb, pthread_service[32];
	char cert_path[1024];
	char key_path[1024];
 	int threads = 1;
	int use_ssl = 0;
	void *retval;
	int opts = 0;
	int n = 0;
#ifndef _WIN32
	int syslog_options = LOG_PID | LOG_PERROR;
#endif
#ifndef LWS_NO_DAEMONIZE
 	int daemonize = 0;
#endif
	printf("start websocket server\n");
	/*
	 * take care to zero down the info struct, he contains random garbaage
	 * from the stack otherwise
	 */
	memset(&info, 0, sizeof info);
	info.port = 9000;

	pthread_mutex_init(&lock_established_conns, NULL);
#if 0
	while (n >= 0) {
		n = getopt_long(argc, argv, "eci:hsap:d:Dr:j:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
		case 'j':
			threads = atoi(optarg);
			if (threads > ARRAY_SIZE(pthread_service)) {
				lwsl_err("Max threads %d\n",
					 ARRAY_SIZE(pthread_service));
				return 1;
			}
			break;
		case 'e':
			opts |= LWS_SERVER_OPTION_LIBEV;
			break;
#ifndef LWS_NO_DAEMONIZE
		case 'D':
			daemonize = 1;
			#ifndef _WIN32
			syslog_options &= ~LOG_PERROR;
			#endif
			break;
#endif
		case 'd':
			debug_level = atoi(optarg);
			break;
		case 's':
			use_ssl = 1;
			break;
		case 'a':
			opts |= LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
			break;
		case 'p':
			info.port = atoi(port);
			break;
		case 'i':
			strncpy(interface_name, optarg, sizeof interface_name);
			interface_name[(sizeof interface_name) - 1] = '\0';
			iface = interface_name;
			break;
		case 'c':
			close_testing = 1;
			fprintf(stderr, " Close testing mode -- closes on "
					   "client after 50 dumb increments"
					   "and suppresses lws_mirror spam\n");
			break;
		case 'r':
			resource_path = optarg;
			printf("Setting resource path to \"%s\"\n", resource_path);
			break;
		case 'h':
			fprintf(stderr, "Usage: test-server "
					"[--port=<p>] [--ssl] "
					"[-d <log bitfield>] "
					"[--resource_path <path>]\n");
			exit(1);
		}
	}
#endif
#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
	/*
	 * normally lock path would be /var/lock/lwsts or similar, to
	 * simplify getting started without having to take care about
	 * permissions or running as root, set to /tmp/.lwsts-lock
	 */
	if (daemonize && lws_daemonize("/tmp/.lwsts-lock")) {
		fprintf(stderr, "Failed to daemonize\n");
		return 1;
	}
#endif

	signal(SIGINT, sighandler);

#ifndef _WIN32
	/* we will only try to log things according to our debug_level */
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("lwsts", syslog_options, LOG_DAEMON);
#endif

	/* tell the library what debug level to emit and to send it to syslog */
	lws_set_log_level(debug_level, lwsl_emit_syslog);
	lwsl_notice("libwebsockets test server pthreads - license LGPL2.1+SLE\n");
	lwsl_notice("(C) Copyright 2010-2016 Andy Green <andy@warmcat.com>\n");

	printf("Using resource path \"%s\"\n", resource_path);
#ifdef EXTERNAL_POLL
	max_poll_elements = getdtablesize();
	pollfds = malloc(max_poll_elements * sizeof (struct lws_pollfd));
	fd_lookup = malloc(max_poll_elements * sizeof (int));
	if (pollfds == NULL || fd_lookup == NULL) {
		lwsl_err("Out of memory pollfds=%d\n", max_poll_elements);
		return -1;
	}
#endif

	info.iface = iface;
	info.protocols = protocols;
	info.extensions = exts;

	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;

	if (use_ssl) {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(cert_path, "%s/libwebsockets-test-server.pem",
			resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
			resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
	}
	info.gid = -1;
	info.uid = -1;
	info.options = opts;
	info.count_threads = threads;
	info.extensions = exts;
	info.max_http_header_pool = 4;

	context = lws_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}

	/* start the dumb increment thread */
	lwsl_err("start Server\n");

	n = pthread_create(&pthread_dumb, NULL, thread_dumb_increment, 0);
	if (n) {
		lwsl_err("Unable to create dumb thread\n");
		goto done;
	}

	/*
	 * notice the actual number of threads may be capped by the library,
	 * so use lws_get_count_threads() to get the actual amount of threads
	 * initialized.
	 */

	for (n = 0; n < lws_get_count_threads(context); n++)
		if (pthread_create(&pthread_service[n], NULL, thread_service,
				   (void *)(long)n))
			lwsl_err("Failed to start service thread\n");

	/* wait for all the service threads to exit */

	for (n = 0; n < lws_get_count_threads(context); n++)
		pthread_join(pthread_service[n], &retval);

	/* wait for pthread_dumb to exit */
	pthread_join(pthread_dumb, &retval);

done:
	lws_context_destroy(context);
	pthread_mutex_destroy(&lock_established_conns);

	lwsl_notice("libwebsockets-test-server exited cleanly\n");

#ifndef _WIN32
	closelog();
#endif

	return 0;
}
