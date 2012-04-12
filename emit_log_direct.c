#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "utils.h"

int main(int argc, const char **argv) {

	const char *hostname;
	int port;
	const char *exchange;
	const char *routingkey;
	const char *messagebody;
	const char *exchangetype = "direct";

	if (argc < 6) {
		fprintf(stderr, "Usage: emit_log_direct host port exchange routingkey messagebody\n");
		return 1;
	}

	hostname = argv[1];
	port = atoi(argv[2]);
	exchange = argv[3];
	routingkey = argv[4];
	messagebody = argv[5];

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

	{
		amqp_basic_properties_t props;
		props._flags = AMQP_BASIC_DELIVERY_MODE_FLAG;
		/*props.content_type = amqp_cstring_bytes("text/plain");*/
		props.delivery_mode = 2; /* persistent delivery mode */
		die_on_error(amqp_basic_publish(conn,
										channelid,
										amqp_cstring_bytes(exchange),
										amqp_cstring_bytes(routingkey),
										0,
										0,
										&props,
										amqp_cstring_bytes(messagebody)),
					"Publishing");
	}

	die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
	die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
	die_on_error(amqp_destroy_connection(conn), "Ending connection");
	return 0;
}
