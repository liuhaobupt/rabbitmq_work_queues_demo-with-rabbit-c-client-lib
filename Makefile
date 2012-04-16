all:rmq_worker rmq_new_task emit_log_direct receive_logs_direct

rmq_worker:rmq_worker.c
	gcc -o rmq_worker -Wall rmq_worker.c utils.c unix/platform_utils.c -I. -lrabbitmq

rmq_new_task:rmq_new_task.c
	gcc -o rmq_new_task -Wall rmq_new_task.c utils.c unix/platform_utils.c -I. -lrabbitmq

emit_log_direct:emit_log_direct.c
	gcc -o emit_log_direct -Wall emit_log_direct.c utils.c unix/platform_utils.c -I. -lrabbitmq

receive_logs_direct:receive_logs_direct.c
	gcc -o receive_logs_direct -Wall receive_logs_direct.c utils.c unix/platform_utils.c -I. -lrabbitmq

clean:
	rm rmq_worker rmq_new_task emit_log_direct receive_logs_direct
