/*Developed by Kristen Patterson & Kenny Steinfeldt
 * Date: November 13, 2017
 * file: sbd.c
 * Starting Point provided by: http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>/* vmalloc */
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>  /* HDIO_GETGEO partitioning*/
#include <linux/slab.h>	  /* kmmalloc */
#include <linux/fcntl.h>  /* File I/O */
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/kdev_t.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>

MODULE_LICENSE("GPL");

static int major_num = 0;
module_param(major_num, int, 0);
static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
static int nsectors = 1024;
module_param(nsectors, int, 0);
struct crypto_cipher *crypt_cipher;
static char *crypt_key = "passwordpassword";
module_param(crypt_key, charp, 0644);
static int crypt_keylen = 16;
module_param(crypt_keylen, int, 0644);

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE 512

/*
 * Our request queue.
 */
static struct request_queue *Queue;

/*
 * Device data
 */
static struct sbd_device {
	unsigned long size;		/* Device size in sectors*/
	spinlock_t lock;		/* Mutex */
	u8 *data;				/* Data Array */
	struct gendisk *gd; 	/* Gendisk struct */
} Device;

static void print_data(u8 *tmp, unsigned int length){
	int i;
	for(i = 0; i < length; i++)
		printk("%02x ", tmp[i]);
	printk("\n");
}

/*
 * Handle an I/O request.
 * param nsect - The number of sectors for reading and writing
 * param sector - Starting sector
 */
static void sbd_crypt_transfer(struct sbd_device *dev, sector_t sector,
		unsigned long nsect, char *buffer, int write) {
	unsigned long offset = sector * logical_block_size;
	unsigned long nbytes = nsect * logical_block_size;
	int i;
	u8 *sourc, dest;

	if ((offset + nbytes) > dev->size) {
		printk(KERN_NOTICE "sbd_crypt_transfer: Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}

	crypto_cipher_setkey(crypt_cipher, crypt_key, crypt_keylen);
	sourc = buffer;
	dest = dev->data + offset;
	if (write) {

		printk("sbd_crypt_transfer: Writing encrypted to file\n");

		for (i = 0; i < nbytes; i += crypto_cipher_blocksize(crypt_cipher)) {
			crypto_cipher_encrypt_one(crypt_cipher, dev->data + offset + i,	buffer + i);
		}

		printk("sbd_crypt_transfer: encrypted view\n");
		print_data(sourc, 15);
		
		printk("sbd_crypt_transfer: decrypted view\n");
		print_data(dest, 15);
	}
	else {
		printk("sbd_crypt_transfer: Reading decryption from file");

		for (i = 0; i < nbytes; i += crypto_cipher_blocksize(crypt_cipher)) {
			crypto_cipher_decrypt_one(crypt_cipher,	buffer + i, dev->data + offset + i);
		}

		printk("sbd_crypt_transfer: encrypted view\n");
		print_data(sourc, 15);

		printk("sbd_crypt_transfer: decrypted view\n");
		print_data(dest, 15);
	}
}

static void sbd_crypt_request(struct request_queue *q) {
	struct request *req;
	req = blk_fetch_request(q);

	while (req != NULL) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk ("\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}

		sbd_crypt_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
				bio_data(req->bio), rq_data_dir(req));

		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}
	}
}

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
int sbd_crypt_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

	size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;

	return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations sbd_crypt_ops = {
		.owner  = THIS_MODULE,
		.getgeo = sbd_crypt_getgeo
};

static int __init sbd_crypt_init(void) {
	printk("sbd_crypt_init: Initializing block\n");
	
	/*
	 * Set up our internal device.
	 */
	Device.size = nsectors * logical_block_size;
	spin_lock_init(&Device.lock);
	Device.data = vmalloc(Device.size);
	if (Device.data == NULL)
		return -ENOMEM;
	/*
	* Get a request queue.
	*/
	Queue = blk_init_queue(sbd_crypt_request, &Device.lock);
	if (Queue == NULL)
		goto out;
	blk_queue_logical_block_size(Queue, logical_block_size);
	/*
	* Get registered.
	*/
	major_num = register_blkdev(major_num, "sbd");
	if (major_num < 0) {
		printk(KERN_WARNING "sbd_crypt_init: unable to get major number\n");
		goto out;
	}
										
	/* allocing cipher */
	crypt_cipher = crypto_alloc_cipher("aes",0,0);
	if(!crypt_cipher){
		printk(KERN_WARNING "sbd_crypt_init: error with crypto creation\n");
		goto out;
	}
												
	crypto_cipher_setkey(crypt_cipher, crypt_key, crypt_keylen);
														
	/*
	* And the gendisk structure.
	*/
	Device.gd = alloc_disk(16);
	if (!Device.gd)
		goto out_unregister;
	Device.gd->major = major_num;
	Device.gd->first_minor = 0;
	Device.gd->fops = &sbd_crypt_ops;
	Device.gd->private_data = &Device;
	strcpy(Device.gd->disk_name, "sbd0");
	set_capacity(Device.gd, nsectors);
	Device.gd->queue = Queue;
	add_disk(Device.gd);
	
	printk("sbd_crypt_init: Device initialized\n");
	
	return 0;

out_unregister:
	unregister_blkdev(major_num, "sbd");
out:
	vfree(Device.data);
	return -ENOMEM;
}

static void __exit sbd_crypt_exit(void){
	printk("sbd_crypt_exit: freeing block device.\n");
	
	crypto_free_cipher(crypt_cipher);
	del_gendisk(Device.gd);
	put_disk(Device.gd);
	unregister_blkdev(major_num, "sbd");
	blk_cleanup_queue(Queue);
				
	printk("sbd_crypt_exit: device freed\n");
}

module_init(sbd_crypt_init);
module_exit(sbd_crypt_exit);

MODULE_AUTHOR("Kristen Patterson Kenny Steinfeldt");
MODULE_DESCRIPTION("Block Device Driver");
