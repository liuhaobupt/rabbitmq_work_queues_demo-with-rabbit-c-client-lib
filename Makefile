all:rmq_worker rmq_new_task

rmq_worker:
	gcc -o rmq_worker rmq_worker.c utils.c unix/platform_utils.c -I. -lrabbitmq

rmq_new_task:
	gcc -o rmq_new_task rmq_new_task.c utils.c unix/platform_utils.c -I. -lrabbitmq

clean:
	rm rmq_worker rmq_new_task
