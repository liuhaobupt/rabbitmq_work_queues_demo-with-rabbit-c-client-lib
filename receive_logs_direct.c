#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <assert.h>

#include "utils.h"

int main(int argc, const char **argv) {

	const char *hostname;
	int port;
	const char *exchange;
	const char *routingkey;
	const char *exchangetype = "direct";

	if (argc < 5) {
		fprintf(stderr, "Usage: receive_logs_direct host port exchange routingkeys...\n");
		return 1;
	}

	hostname = argv[1];
	port = atoi(argv[2]);
	exchange = argv[3];

	int sockfd;
	int channelid = 1;
	amqp_connection_state_t conn;
	conn = amqp_new_connection();

	die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
	amqp_set_sockfd(conn, sockfd);
	die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),"Logging in");
	amqp_channel_open(conn, channelid);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

	amqp_exchange_declare(conn,channelid,amqp_cstring_bytes(exchange),amqp_cstring_bytes(exchangetype),0,1,
						  amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn),"Declaring exchange");

	amqp_queue_declare_ok_t *r = amqp_queue_declare(conn,channelid,amqp_empty_bytes,0,0,1,0,amqp_empty_table);

	int i;
	for(i = 4;i < argc;i++)
	{
		routingkey = argv[i];
		amqp_queue_bind(conn,channelid,amqp_bytes_malloc_dup(r->queue),amqp_cstring_bytes(exchange),
						amqp_cstring_bytes(routingkey),amqp_empty_table);
	}

	amqp_basic_qos(conn,channelid,0,1,0);
	amqp_basic_consume(conn,channelid,amqp_bytes_malloc_dup(r->queue),amqp_empty_bytes,0,0,0,amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

	{
		amqp_frame_t frame;
		int result;
		amqp_basic_deliver_t *d;
		amqp_basic_properties_t *p;
		size_t body_target;
		size_t body_received;

		while (1) {
			amqp_maybe_release_buffers(conn);
			result = amqp_simple_wait_frame(conn, &frame);
			printf("Result %d\n", result);
			if (result < 0)
				break;

			printf("Frame type %d, channel %d\n", frame.frame_type, frame.channel);
			if (frame.frame_type != AMQP_FRAME_METHOD)
				continue;

			printf("Method %s\n", amqp_method_name(frame.payload.method.id));
			if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
				continue;

			d = (amqp_basic_deliver_t *) frame.payload.method.decoded;
			printf("Delivery %u, exchange %.*s routingkey %.*s\n",(unsigned) d->delivery_tag,
				(int) d->exchange.len, (char *) d->exchange.bytes,
				(int) d->routing_key.len, (char *) d->routing_key.bytes);

			result = amqp_simple_wait_frame(conn, &frame);
			if (result < 0)
				break;

			if (frame.frame_type != AMQP_FRAME_HEADER) {
				fprintf(stderr, "Expected header!");
				abort();
			}
			p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
			if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
				printf("Content-type: %.*s\n",
				(int) p->content_type.len, (char *) p->content_type.bytes);
			}

			body_target = frame.payload.properties.body_size;
			body_received = 0;

			int sleep_seconds = 0;
			while (body_received < body_target) {
				result = amqp_simple_wait_frame(conn, &frame);
				if (result < 0)
					break;

				if (frame.frame_type != AMQP_FRAME_BODY) {
					fprintf(stderr, "Expected body!");
					abort();
				}

				body_received += frame.payload.body_fragment.len;
				assert(body_received <= body_target);

				int i;
				for(i = 0; i<frame.payload.body_fragment.len; i++)
				{
					printf("%c",*((char*)frame.payload.body_fragment.bytes+i));
					if(*((char*)frame.payload.body_fragment.bytes+i) == '.')
						sleep_seconds++;
				}
				printf("\n");

			}

			if (body_received != body_target) {
				/* Can only happen when amqp_simple_wait_frame returns <= 0 */
				/* We break here to close the connection */
				break;
			}
			/* do something */
			sleep(sleep_seconds);

			amqp_basic_ack(conn,channelid,d->delivery_tag,0);
		}
	}

	die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
	die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
	die_on_error(amqp_destroy_connection(conn), "Ending connection");

	return 0;
}
