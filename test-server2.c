#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test-server.h"
 unsigned char *buf = NULL;

 extern void setTrigger(void);
extern unsigned char* sendData(int fp, int channelNum);

int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user,
		  void *in, size_t len)
{
    return 0;
}

struct a_message {
    void* payload;
    size_t len;
};
static struct a_message messageRecieved;

int callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)

{

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
            lwsl_notice("connection established\n");
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            lwsl_notice("callback received\n");
			fflush(stdout);
            //if (messageRecieved.len <= 0)
            //break;
			buf = NULL;

             //buf = sendData(NULL, 4);// (unsigned char*) malloc(LWS_SEND_BUFFER_PRE_PADDING + messageRecieved.len + LWS_SEND_BUFFER_POST_PADDING);
//
      //     int i;
      //      for (i=0; i < messageRecieved.len; i++) {
      //          buf[i+LWS_SEND_BUFFER_PRE_PADDING] = ((unsigned char *)messageRecieved.payload)[i+LWS_SEND_BUFFER_PRE_PADDING];
       //     }

          //  lwsl_notice("received data: %s, replying: %.*s\n", (unsigned char *) messageRecieved.payload + LWS_SEND_BUFFER_PRE_PADDING, (int) messageRecieved.len, buf + LWS_SEND_BUFFER_PRE_PADDING);
		if(buf != NULL)
			{
           lws_write(wsi, buf, 2048, LWS_WRITE_BINARY);
		   free(buf);
			}
		else
		      lws_write(wsi, "aaa", 3, LWS_WRITE_TEXT);

	
	usleep(100000);
lws_callback_on_writable_all_protocol(lws_get_context(wsi),
					  lws_get_protocol(wsi));
		
//            free(buf);

            break;

        case LWS_CALLBACK_RECEIVE:{
			//setTrigger();
/*
            if(messageRecieved.payload)
                free(messageRecieved.payload);

            messageRecieved.payload = malloc(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING);
            messageRecieved.len = len;
            memcpy((unsigned char *)messageRecieved.payload + LWS_SEND_BUFFER_PRE_PADDING,in,len);

            lwsl_notice("messageRecieved.payload : %s \n messageRecieved.len : %d\n",(unsigned char *)messageRecieved.payload+LWS_SEND_BUFFER_PRE_PADDING,(int) messageRecieved.len);
            */
			lws_callback_on_writable_all_protocol(lws_get_context(wsi),
								  lws_get_protocol(wsi));
			usleep(100000);
            break;
        }
        default:
            break;
        }

    return 0;
}


static struct lws_protocols protocols[] = {
    /* first protocol must always be HTTP handler */
    {
        "http-only", // name
        callback_http, // callback
        0 // per_session_data_size
    },
    {
        "dumb-increment-protocol", // protocol name - very important!
        callback_dumb_increment, // callback
        0 // we don't use any per session data

    },
    {
        NULL, NULL, 0 /* End of list */
    }
};
int testmain(void) {
    // server url will be http://localhost:9000
    int port = 9000;
    const char *interface = NULL;
    struct lws_context *context;
    // we're not using ssl
    const char *cert_path = NULL;
    const char *key_path = NULL;
    // no special options
    int opts = 0;
    struct lws_context_creation_info info;

    memset(&info,0,sizeof(info));
    info.port=port;
    info.iface=interface;
    info.protocols = protocols;
    info.extensions = NULL; // libwebsocket_get_initernal_extensions();
    info.ssl_cert_filepath=NULL;
    info.ssl_private_key_filepath=NULL;
    info.gid=-1;
    info.uid=-1;
    info.options=opts;

    // create libwebsocket context representing this server
    context = lws_create_context(&info);

    if (context == NULL) {
        fprintf(stderr, "libwebsocket init failed\n");
        return -1;
    }

    printf("starting server...\n");

    // infinite loop, to end this server send SIGTERM. (CTRL+C)
    while (1) {
        lws_service(context, 50);
        // libwebsocket_service will process all waiting events with their
        // callback functions and then wait 50 ms.
        // (this is a single threaded webserver and this will keep our server
        // from generating load while there are not requests to process)
    }

   lws_context_destroy(context);

    return 0;
}

