#include <stdio.h>
#include "_queue.h"

struct eventList{
	struct list_head signal_queue;
};

struct event{
	struct list_head node;
	int    val;
};

int main(){
	struct eventList eL;
	struct event eN;
	eN.val = 8888;
	init_queue_head(&eL.signal_queue);
	queue_add_tail(&eN.node, &eL.signal_queue);
	struct list_head *cur = queue_get_first(&eL.signal_queue);
	struct event * tmp = (struct event *)container_of(cur, struct event, node);
	printf("%d\n", tmp->val);

	cur = queue_get_first(&eL.signal_queue);
	tmp = (struct event *)container_of(cur, struct event, node);
	printf("%d\n", tmp->val);

	cur = queue_get_first(&eL.signal_queue);
	queue_del_first(&eL.signal_queue);
	tmp = (struct event *)container_of(cur, struct event, node);
	printf("%d\n", tmp->val);

	printf("%d\n", queue_empty(&eL.signal_queue));
	cur = queue_get_first(&eL.signal_queue);
	tmp = (struct event *)container_of(cur, struct event, node);
	printf("%d\n", tmp->val);
}
