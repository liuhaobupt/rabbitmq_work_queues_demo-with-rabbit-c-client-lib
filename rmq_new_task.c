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
	const char *queuename;
	const char *messagebody;

	if (argc < 6) {
		fprintf(stderr, "Usage: rmq_new_task host port exchange queuename messagebody\n");
		return 1;
	}

	hostname = argv[1];
	port = atoi(argv[2]);
	exchange = argv[3];
	queuename = argv[4];
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

/*	amqp_table_entry_t *arg = malloc(sizeof(amqp_table_entry_t));
	arg->key = amqp_cstring_bytes("x-ha-policy");
	arg->value.value.bytes = amqp_cstring_bytes("all");
	const amqp_table_t args = {1,arg};*/

	amqp_queue_declare(conn,channelid,amqp_cstring_bytes(queuename),0,1,0,0,amqp_empty_table);

	{
		amqp_basic_properties_t props;
		props._flags = AMQP_BASIC_DELIVERY_MODE_FLAG;
		/*props.content_type = amqp_cstring_bytes("text/plain");*/
		props.delivery_mode = 2; /* persistent delivery mode */
		die_on_error(amqp_basic_publish(conn,
										channelid,
										amqp_cstring_bytes(exchange),
										amqp_cstring_bytes(queuename),
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
