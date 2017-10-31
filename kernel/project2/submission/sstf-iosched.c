/* Developed by Kristen Patterson and Kenny Steinfeldt
 * Date: October 30, 2017
 * Name: sstf-iosched.c
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

//Main data type to store requests
struct sstf_data{
	struct list_head queue;
	int dir;
	sector_t head;
};

/* Name: init_sstf
 * Description: Initializes the queue
 * Parameters: Request queue, Elevator
 */
static int init_sstf(struct request_queue *queue, struct elevator_type *elev)
{
	struct sstf_data *data;
	struct elevator_queue *elevque;

	elevque = elevator_alloc(queue, elev);
	
	if(!elevque)
		return -ENOMEM;

	data = kmalloc_node(sizeof(*data), GFP_KERNEL, queue->node);
	
	if(!data){
		kobject_put(&elevque->kobj);
		return -ENOMEM;
	}

	data->head = 0;
	elevque->elevator_data = data;

	INIT_LIST_HEAD(&data->queue);

	spin_lock_irq(queue->queue_lock);
	queue->elevator = elevque;
	spin_unlock_irq(queue->queue_lock);
	
	return 0;
}

/* Name: add_req
 * Description: adds a request to the queue
 * Parameters: Request queue, request
 */
static void add_req(struct request_queue *queue, struct request *req)
{
    struct sstf_data *nd = queue->elevator->elevator_data;
    struct request *next, *prev;

    printk("LOOK: add_req() - Starting up.\n");
    
    struct request *iter;
    
    if (list_empty(&nd->queue)){  // list is empty, our work here is done.
	printk("add_req() - List is empty.\n");
        list_add(&req->queuelist, &nd->queue);
    } else { // find right place for request
	printk("add_req() - Searching for a place for the request.\n");
        next = list_entry(nd->queue.next, struct request, queuelist);
        prev = list_entry(nd->queue.prev, struct request, queuelist);
        
        if (blk_rq_pos(req) > blk_rq_pos(next)){
            while (blk_rq_pos(req) > blk_rq_pos(next)) {
                prev = next;
                next = list_entry(next->queuelist.next, struct request, queuelist);
            }
            
            list_add(&req->queuelist, &prev->queuelist);
        } else {
            while (blk_rq_pos(req) > blk_rq_pos(prev)) {
                next = prev;
                prev = list_entry(prev->queuelist.prev, struct request, queuelist);
            }
            
            list_add(&req->queuelist, &next->queuelist);
        }
        list_for_each_entry(iter, &nd->queue, queuelist) {
            printk("entry: %lu\n", (unsigned long) blk_rq_pos(iter));
        }
    }
}

/* Name: former_req
 * Description: stores former request in queue
 * Parameters: request queue, request
 */
static struct request *former_req(struct request_queue *queue, struct request *req)
{
	struct sstf_data *data = queue->elevator->elevator_data;

	if(req->queuelist.prev == &data->queue)
		return NULL;

	return list_entry(req->queuelist.prev, struct request, queuelist);
}

/* Name: latter_req
 * Description: stores latter request in queue
 * Parameters: request queue, request
 */
static struct request *latter_req(struct request_queue *queue, struct request *req)
{
	struct sstf_data *data = queue->elevator->elevator_data;

	if(req->queuelist.next == &data->queue)
		return NULL;

	return list_entry(req->queuelist.next, struct request, queuelist);
}

/* Name: merge_req
 * Description: merges requests
 * Parameters: request queue, request, next node
 */
static void merge_req(struct request_queue *queue, struct request *req, struct request *next)
{
	list_del_init(&next->queuelist);
}

/* Name: dispatch_req
 * Description: Main portion of code, runs the actual elevator
 * Parameters: request queue, force
 */
static int dispatch_req(struct request_queue *queue, int force)
{
    struct sstf_data *nd = queue->elevator->elevator_data;
    
    if  (!list_empty(&nd->queue)){
        struct request *req;
        req = list_entry(nd->queue.next, struct request, queuelist);
        list_del_init(&req->queuelist);
        elv_dispatch_sort(queue, req);
        return 1;
    }
    return 0;
}

/* Name: exit_sstf
 * Description: exits the queue
 * Parameters: elevator
 */
static void exit_sstf(struct elevator_queue *elev)
{
	struct sstf_data *data = elev->elevator_data;

	BUG_ON(!list_empty(&data->queue));
	kfree(data);
}

//http://elixir.free-electrons.com/linux/v4.9/source/include/linux/elevator.h
static struct elevator_type elevator_sstf = {
	.ops = {
		.elevator_init_fn = init_sstf,
		.elevator_add_req_fn = add_req,
		.elevator_former_req_fn = former_req,
		.elevator_latter_req_fn = latter_req,
		.elevator_merge_req_fn = merge_req,
		.elevator_dispatch_fn = dispatch_req,
		.elevator_exit_fn = exit_sstf,
	},
	.elevator_name = "LOOK",
	.elevator_owner = THIS_MODULE,
};

/* Name: elev_init
 * Description: Initializes the elevator
 * Parameters: N/A
 */
static int __init elev_init(void)
{
	return elv_register(&elevator_sstf);
}

/* Name: elev_exit
 * Description: exits out of the elevator
 * Parameters: N/A
 */
static void __exit elev_exit(void)
{
	elv_unregister(&elevator_sstf);
}

module_init(elev_init);
module_exit(elev_exit);

MODULE_AUTHOR("Kristen Patterson & Kenny Steinfeldt");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("IO Scheduler");
