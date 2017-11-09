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

/* Drive size */
static int nsectors = 1024;
module_param(nsectors, int, 0);

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

/* ***Note that the crypto key must be 16 characters long */
struct crypto_cipher *tfm;
static char *key = "1234567890123456";
module_param(key, charp, 0644);
static int keylen = 16;
module_param(keylen, int, 0644);

/*
 * Handle an I/O request.
 * param nsect - The number of sectors for reading and writing
 * param sector - Starting sector
 */
static void sbd_transfer(struct sbd_device *dev, sector_t sector,
		unsigned long nsect, char *buffer, int write) {
	unsigned long offset = sector * logical_block_size;
	unsigned long nbytes = nsect * logical_block_size;
	u8 *destination;
	u8 *source;

	if (write)
		printk("[ sbd.c: sbd_transfer() ] - WRITE Transferring Data\n");
	else
		printk("[ sbd.c: sbd_transfer() ] - READ Transferring Data\n");

	if ((offset + nbytes) > dev->size) {
		printk("[ sbd.c: sbd_transfer() ] - OFFSET Too Far %ld NBYTES: %ld\n", offset, nbytes);
		return;
	}

	if (crypto_cipher_setkey(tfm, key, keylen) == 0) {
		printk("[ sbd.c: sbd_transfer() ] - Crypto key is set and encrypted\n");
	} else {
		printk("[ sbd.c: sbd_transfer() ] - Crypto key was not able to be set\n");
	}

	int i;

	/*
	 * Essentially we are transferring (encrypting/decrypting) data one block at
	 * a time until we reach a specific length (n bytes)
	 * If we are writing, then we are transferring data from the block to the device
	 * If we are reading, then we are transferring data from the device to the block
	 */

	if (write) {

		printk("[ sbd.c: sbd_transfer() ] - Write %lu bytes to device data\n", nbytes);

		destination = dev->data + offset;
		source = buffer;

		for (i = 0; i < nbytes; i += crypto_cipher_blocksize(tfm)) {
			/* Use crypto cipher handler and tfm to encrypt data one block at a time*/
			crypto_cipher_encrypt_one(
					tfm,	 				/* Cipher handler */
					dev->data + offset + i,	/* Destination */
					buffer + i				/* Source */
					);
		}

		printk("[ sbd.c: sbd_transfer() ] - UNENCRYPTED DATA VIEW:\n");
		for (i = 0; i < 100; i++) {
			printk("%u", (unsigned) *destination++);
		}

		printk("\n[ sbd.c: sbd_transfer() ] - ENCRYPTED DATA VIEW:\n");
		for (i = 0; i < 100; i++) {
			printk("%u", (unsigned) *source++);
		}
		printk("\n");
	}
	else {
		printk("[ sbd.c: sbd_transfer() ] - Read %lu bytes to device data\n", nbytes);

		destination = dev->data + offset;
		source = buffer;

		for (i = 0; i < nbytes; i += crypto_cipher_blocksize(tfm)) {
			/* Use crypto cipher handler and tfm to decrypt data one block at a time*/
			crypto_cipher_decrypt_one(
					tfm,					/* Cipher handler */
					buffer + i,				/* Destination */
					dev->data + offset + i	/* Source */
					);
		}

		printk("[ sbd.c: sbd_transfer() ] - UNENCRYPTED DATA VIEW:\n");
		for (i = 0; i < 100; i++) {
			printk("%u", (unsigned) *destination++);
		}

		printk("\n[ sbd.c: sbd_transfer() ] - ENCRYPTED DATA VIEW:\n");
		for (i = 0; i < 100; i++) {
			printk("%u", (unsigned) *source++);
		}
		printk("\n");
	}

	printk("[ sbd.c: sbd_transfer() ] - Transfer and Encryption Completed\n");
}

static void sbd_request(struct request_queue *q) {
	struct request *req;

	/* Gets request from top of queue */
	req = blk_fetch_request(q);

	printk("[ sbd.c: sbd_request() ] - Fetching Requests\n");

	/* Iterate through queue */
	while (req != NULL) {

		/* Skip non-command requests */
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk ("[ sbd.c: sbd_transfer() ] - Skip non-command request\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}

		sbd_transfer(&Device, 					/* Device Data */
					blk_rq_pos(req),			/* Request Sector */
					blk_rq_cur_sectors(req),	/* Number of sectors */
					bio_data(req->bio), 				/* Buffer */
					rq_data_dir(req)); 			/* Write */

		printk("[ sbd.c: sbd_transfer() ] - Request Data Transfered\n");

		/* Get next request */
		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}

		printk("[ sbd.c: sbd_transfer() ] - Requests Completed!\n");
	}
}

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
int sbd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

	printk("[ sbd.c: sbd_getgeo() ] - Start Partitioning\n");

	/* We have no real geometry, of course, so make something up. */
	size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;

	printk("[ sbd.c: sbd_getgeo() ] - Finish Partitioning\n");

	return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations sbd_ops = {
		.owner  = THIS_MODULE,
		.getgeo = sbd_getgeo
};

static int __init sbd_init(void) {
	/* File I/O */
	mm_segment_t oldfs;
	struct file *filp = NULL;
	unsigned long long offset = 0;
	ssize_t size;

	printk("[ sbd.c: sbd_init() ] - Start Initialize\n");

	/* Register block device */
	major_num = register_blkdev(major_num, "sbd");

	/* Check if block device is busy */
	if (major_num < 0) {
		printk("[ sbd.c: sbd_init() ] - Unable to register Block Device\n");
		return -EBUSY;
	}

	/* Set up device and set the device to all 0s */
	memset(&Device, 0, sizeof(struct sbd_device));
	Device.size = nsectors * logical_block_size;
	Device.data = vmalloc(Device.size);

	memset(Device.data, 0, Device.size);

	/* Check if device data is allocated */
	if (Device.data == NULL) {
		printk("[ sbd.c: sbd_init() ] - Unable to allocate Device.data\n");
		unregister_blkdev(major_num, "sbd");
		return -ENOMEM;
	}

	printk("[ sbd.c: sbd_init() ] - Device Size: %ld\n", Device.size);

	/* Copy device data from a file. Create it too if it does not exist */
	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open("/Data", O_RDONLY | O_CREAT, S_IRWXUGO);

	printk("[ sbd.c: sbd_init() ] - Attempt to open /Data\n");

	/* Check any errors or NULL from reading */
	if (IS_ERR(filp)) {
		printk("[ sbd.c: sbd_init() ] - Unable to open /Data\n");
		set_fs(oldfs);
	} else {

		/* Read File */
		size = vfs_read(filp, Device.data, Device.size, &offset);
		printk("[ sbd.c: sbd_init() ] File output - size: %d, offset: %llu\n", size, offset);

		/* Closed File */
		set_fs(oldfs);
		filp_close(filp, 0);
		printk("[ sbd.c: sbd_init() ] - Close file\n");
	}

	/* Initialize spin_lock */
	spin_lock_init(&Device.lock);

	/* Initialize queue and call sbd_request when requests come in */
	Queue = blk_init_queue(sbd_request, &Device.lock);
	if (Queue == NULL) {
		printk("[ sbd.c: sbd_init() ] - Unable to initialize queue\n");
		unregister_blkdev(major_num, "sbd");
		vfree(Device.data);
		return -ENOMEM;
	}

	/* Set logical_block_size for queue */
	blk_queue_logical_block_size(Queue, logical_block_size);

	/* Initialize gendisk */
	Device.gd = alloc_disk(16);
	if (!Device.gd) {
		printk("[ sbd.c: sbd_init() ] - Unable to allocate gendisk\n");
		unregister_blkdev(major_num, "sbd");
		vfree(Device.data);
		return -ENOMEM;
	}

	/* Initialize cypto and set key
	 * ctrypto_alloc_cipher are: crypto driver name, type, and mask
	 */
	tfm = crypto_alloc_cipher("aes", 0, 0);

	if (IS_ERR(tfm))
		printk("[ sbd.c: sbd_init() ] - Unable to allocate cipher\n");
	else
		printk("[ sbd.c: sbd_init() ] - Allocated cipher\n");

	/* Crypto debugging print statements */
	printk("[ sbd.c: sbd_init() ] - Block Cipher Size: %u\n", crypto_cipher_blocksize(tfm));
	printk("[ sbd.c: sbd_init() ] - Crypto key: %s\n", key);
	printk("[ sbd.c: sbd_init() ] - Key Length: %d\n", keylen);

	/* And The gendisk structure */
	Device.gd->major = major_num;
	Device.gd->first_minor = 0;
	Device.gd->fops = &sbd_ops;
	Device.gd->private_data = &Device;
	strcpy(Device.gd->disk_name, "sbd0");
	set_capacity(Device.gd, nsectors);
	Device.gd->queue = Queue;

	/* Register partition in Device.gd with kernel */
	add_disk(Device.gd);

	printk("[ sbd.c: sbd_init() ] - Successful initialization\n");

	return 0;
}

static void __exit sbd_exit(void)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	ssize_t size;
	unsigned long long offset = 0;

	printk("[ sbd.c: sbd_exit() ] - Exiting\n");

	/* First write data to file */
	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open("/Data", O_WRONLY | O_TRUNC | O_CREAT, S_IRWXUGO);

	if (IS_ERR(filp)) {
		printk("[ sbd.c: sbd_exit() ] - Unable to open file\n");
		set_fs(oldfs);
	} else {
		printk("[ sbd.c: sbd_exit() ] - Opened file\n");

		/* Write the bytes to file */
		size = vfs_write(filp, Device.data, Device.size, &offset);
		printk("[ sbd.c: sbd_exit() ] - Wrote to file: %d Offset: %llu.\n", size, offset);

		/* Close File */
		set_fs(oldfs);
		filp_close(filp, 0);
		printk("[ sbd.c: sbd_exit() ] - Closed file\n");
	}

	del_gendisk(Device.gd);
	put_disk(Device.gd);
	unregister_blkdev(major_num, "sbd");
	blk_cleanup_queue(Queue);
	vfree(Device.data);

	crypto_free_cipher(tfm);

	printk("[ sbd.c: sbd_exit() ] - Module Exited\n");
}

module_init(sbd_init);
module_exit(sbd_exit);

MODULE_AUTHOR("Kristen Patterson Kenny Steinfeldt");
MODULE_DESCRIPTION("Block Device Driver");
