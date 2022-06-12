/* 3c509.c: A 3c509 EtherLink3 ethernet driver for linux. */
/*
	For ELKS by Helge Skrivervik (@mellvik) may/june 2022

	Adapted from Linux driver by Donald Becker et al.

	Parts Copyright 1994-2000 by Donald Becker and (1993) United States Government as represented by the
	Director, National Security Agency.
	This software may be used and distributed according to the terms of the GNU General Public License,
	incorporated herein by reference.

*/

#define ELKS

#include <arch/io.h>
#include <arch/ports.h>
#include <arch/segment.h>
#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>  
#include <linuxmt/sched.h>
#include <linuxmt/limits.h>
#include <linuxmt/mm.h>  
#include <linuxmt/debug.h> 
#include <linuxmt/netstat.h>
#include <netinet/in.h>

#ifdef EL3_DEBUG
static int el3_debug = EL3_DEBUG;
#else
static int el3_debug = 0;
#endif


/* To minimize the size of the driver source I only define operating
   constants if they are used several times.  You'll need the manual
   anyway if you want to understand driver details. */
/* Offsets from base I/O address. */
#define EL3_DATA 0x00
#define EL3_CMD 0x0eU
#define EL3_STATUS 0x0eU
#define	EEPROM_READ 0x80U

#define EL3WINDOW(win_num) outw(SelectWindow + (win_num), ioaddr + EL3_CMD)


/* The top five bits written to EL3_CMD are a command, the lower
   11 bits are the parameter, if applicable. */
enum c509cmd {
	TotalReset = 0<<11, SelectWindow = 1<<11, StartCoax = 2<<11,
	RxDisable = 3<<11, RxEnable = 4<<11, RxReset = 5<<11, RxDiscard = 8<<11,
	TxEnable = 9<<11, TxDisable = 10<<11, TxReset = 11<<11,
	FakeIntr = 12<<11, AckIntr = 13<<11, SetIntrEnb = 14<<11,
	IntStatusEnb = 15<<11, SetRxFilter = 16<<11, SetRxThreshold = 17<<11,
	SetTxThreshold = 18<<11, SetTxStart = 19<<11, StatsEnable = 21<<11,
	StatsDisable = 22<<11, StopCoax = 23<<11, PowerUp = 27<<11,
	PowerDown = 28<<11, PowerAuto = 29<<11};

enum c509status {
	IntLatch = 0x0001U, AdapterFailure = 0x0002U, TxComplete = 0x0004U,
	TxAvailable = 0x0008U, RxComplete = 0x0010U, RxEarly = 0x0020U,
	IntReq = 0x0040U, StatsFull = 0x0080U, CmdBusy = 0x1000U, };

static int active_imask = IntLatch|RxComplete|TxAvailable|TxComplete|StatsFull;
/* The SetRxFilter command accepts the following classes: */
enum RxFilter {
	RxStation = 1, RxMulticast = 2, RxBroadcast = 4, RxProm = 8 };

/* Register window 1 offsets, the window used in normal operation. */
#define TX_FIFO		0x00U
#define RX_FIFO		0x00U
#define RX_STATUS 	0x08U
#define TX_STATUS 	0x0BU
#define TX_FREE		0x0CU		/* Remaining free bytes in Tx buffer. */

#define WN0_CONF_CTRL	0x04U		/* Window 0: Configuration control register */
#define WN0_ADDR_CONF	0x06U		/* Window 0: Address configuration register */
#define WN0_IRQ		0x08U		/* Window 0: Set IRQ line in bits 12-15. */
#define WN4_MEDIA	0x0AU		/* Window 4: Various transcvr/media bits. */
#define	MEDIA_TP	0x00C0U		/* Enable link beat and jabber for 10baseT. */
#define WN4_NETDIAG	0x06U		/* Window 4: Net diagnostic */
#define FD_ENABLE	0x8000U		/* Enable full-duplex ("external loopback") */
#define ENABLE_ADAPTER	0x1		/* Enable adapter (W0-conf-ctrl reg) */

#define EEPROM_NODE_ADDR_0	0x0	/* Word */
#define EEPROM_NODE_ADDR_1	0x1	/* Word */
#define EEPROM_NODE_ADDR_2	0x2	/* Word */
#define EEPROM_PROD_ID		0x3	/* 0x9[0-f]50 */
#define EEPROM_MFG_ID		0x7	/* 0x6d50 */
#define EEPROM_ADDR_CFG		0x8	/* Base addr */
#define EEPROM_RESOURCE_CFG	0x9	/* IRQ. Bits 12-15 */

#define EP_ID_PORT_START 0x110  /* avoid 0x100 to avoid conflict with SB16 */
#define EP_ID_PORT_INC 0x10
#define EP_ID_PORT_END 0x200
#define EP_TAG_MAX		0x7 /* must be 2^n - 1 */

static int el3_isa_probe();
//static word_t read_eeprom(int, int);
static word_t id_read_eeprom(int);
void el3_drv_init();
static size_t el3_write(struct inode *, struct file *, char *, size_t);
static void el3_int(int, struct pt_regs *);
static size_t el3_read(struct inode *, struct file *, char *, size_t);
static void el3_release(struct inode *, struct file *);
static int el3_open(struct inode *, struct file *);
static int el3_ioctl(struct inode *, struct file *, unsigned int, unsigned int);
static int el3_select(struct inode *, struct file *, int);
static void el3_down();
void el3_sendpk(int, char *, int);
void el3_insw(int, char *, int);
extern void el3_mdelay(int);
static void update_stats();

//static int current_tag;
// static struct net_device *el3_devs[EL3_MAX_CARDS];

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 5;

#define pr_debug printk

int net_irq = EL3_IRQ;  /* default IRQ, changed by netirq= in /bootopts */
int net_port = EL3_PORT; /* default IO PORT, changed by netport= in /bootopts */

struct netif_stat netif_stat =
	{ 0, 0, 0, 0, 0, 0, {0x52, 0x54, 0x00, 0x12, 0x34, 0x57}};  /* QEMU default  + 1 */
static char *mac_addr = (char *)&netif_stat.mac_addr;
static int ioaddr;	// FIXME  remove later

// Static data
struct wait_queue rxwait;
struct wait_queue txwait;

static word_t el3_id_port;

static struct file_operations el3_fops =
{
    NULL,	 /* lseek */
    el3_read,
    el3_write,
    NULL,	 /* readdir */
    el3_select,
    el3_ioctl,
    el3_open,
    el3_release
};

void el3_drv_init( void ) {
	int err;

	ioaddr = net_port;		// temporary
	err = request_irq(net_irq, el3_int, INT_GENERIC);
	if (err) {
		printk("eth: Cannot allocate IRQ %d for EL3: %i\n",
			net_irq, err);
		return;
	}
	err = register_chrdev(ETH_MAJOR, "eth", &el3_fops);
	if (err) {
		printk("eth: device registration error: %i\n", err);
		return;
	}

	// May want to probe before requesting the IRQ & device, but
	// then we need a tiny probe routine, not the whole shebang.
	if (el3_isa_probe()) {
		//printk("not found\n");
		return;
	}
	netif_stat.if_status |= NETIF_FOUND;
	
}
static int el3_find_id_port ( void ) {

	for ( el3_id_port = EP_ID_PORT_START ;
	      el3_id_port < EP_ID_PORT_END ;
	      el3_id_port += EP_ID_PORT_INC ) {
		outb(0, el3_id_port);
		/* See if anything's listening */
		outb(0xff, el3_id_port);
		if (inb(el3_id_port) & 0x01) {
			/* Found a suitable port */
			//printk("el3: using ID port at %04x\n", el3_id_port);
			return 0;
		}
	}
	/* No id port available */
	printk("el3: no available ID port found\n");
	return -ENOENT;
}

/* Return 0 on success, 1 on error */
static int el3_isa_probe( void )
{
	short lrs_state = 0xff;
	int i;
	word_t *mac = (word_t *)mac_addr;

	/* ISA boards are detected by sending the ID sequence to the
	   ID_PORT.  We find cards past the first by setting the 'current_tag'
	   on cards as they are found.  Cards with their tag set will not
	   respond to subsequent ID sequences.
	   ELKS supports only one card, comment kept for future reference. HS */

	if (el3_find_id_port()) return 1;

	outb(0x00, el3_id_port);
	outb(0x00, el3_id_port);	// just wait ...
	for (i = 0; i < 255; i++) {
		outb(lrs_state, el3_id_port);
		lrs_state <<= 1;
		lrs_state = lrs_state & 0x100 ? lrs_state ^ 0xcf : lrs_state;
	}
	outb(0xd0, el3_id_port);	// Set tag

	outb(0xd0, el3_id_port);			// select tag (0)
	outb(0xe0 |(ioaddr >> 4), el3_id_port );	// Activate
	printk("eth: 3C509 @ IRQ %d, port %02x", net_irq, ioaddr);

	if ((i = id_read_eeprom(EEPROM_MFG_ID)) != 0x6d50) {
		printk(" - probe failed (wrong ID), got %04x\n", i);
		return 1;
	}
	/* Read in EEPROM data.
	   3Com got the byte order backwards in the EEPROM. */
	for (i = 0; i < 3; i++)
		mac[i] = htons(id_read_eeprom(i));
	printk(", MAC %02x", (mac_addr[0]&0xff));
	i = 1;
	while (i < 6) printk(":%02x", (mac_addr[i++]&0xff));

	int iobase = id_read_eeprom(EEPROM_ADDR_CFG);
	//int if_port = iobase >> 14;
	printk(" (HW conf: irq %d port %x)", (id_read_eeprom(9) >> 12), (0x200 + ((iobase & 0x1f) << 4)));
	printk("\n");
	
	return 0;

}

#if LATER
/* Read a word from the EEPROM using the regular EEPROM access register.
   Assume that we are in register window zero.
 */
static word_t read_eeprom(int addr, int index)
{
	outw(EEPROM_READ + index, addr + 10);
	/* Pause for at least 162 us. for the read to take place.
	   Some chips seem to require much longer */
	el3_mdelay(200);
	return inw(addr + 12);
}
#endif

/* Read a word from the EEPROM when in the ISA ID probe state. */
static word_t id_read_eeprom(int index)
{
	int bit, word = 0;

	/* Issue read command, and pause for at least 162 us. for it to complete.
	   Assume extra-fast 16Mhz bus. */
	outb(EEPROM_READ + index, el3_id_port);

	/* Pause for at least 162 us. for the read to take place. */
	/* Some chips seem to require much longer */
	el3_mdelay(400);

	for (bit = 15; bit >= 0; bit--)
		word = (word << 1) + (inb(el3_id_port) & 0x01);

	if (el3_debug > 3)
		pr_debug("  3c509 EEPROM word %d %04x.\n", index, word);

	return word;
}

static size_t el3_write(struct inode *inode, struct file *file, char *data, size_t len)
{
	int res;

	while (1) {
		/*
		 *	FIXME: May need to block other interrupts for the duration
		 */
		prepare_to_wait_interruptible(&txwait);
		if (len > MAX_PACKET_ETH) len = MAX_PACKET_ETH;
		if (len < 64) len = 64;
		//printk("T%d^", len);

		if (inw(ioaddr + TX_FREE) < (len + 4)) {
			// NO space in FIFO, wait
			if (file->f_flags & O_NONBLOCK) {
				res = -EAGAIN;
				break;
			}
			do_wait();
			if (current->signal) {
				res = -EINTR;
				break;
			}
		}
		//EXPERIMENTAL
		outw(SetIntrEnb | 0x0, ioaddr + EL3_CMD);	// Block interrupts
		
		el3_sendpk(ioaddr + TX_FIFO, data, len);

		/* Interrupt us when the FIFO has room for max-sized packet. */
		// FIXME: Unsure about this one ... Tune later
		outw(SetTxThreshold + 1536, ioaddr + EL3_CMD);
		//EXPERIMENTAL
		outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);	// Reenable interrupts

		outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
						/* may not be the right place to do this */
		// END EXPERIMENTAL

		/* Clear the Tx status stack. */
#if 0		// FIXME - check if this is OK to remove.
		{
			short tx_status;
			int i = 4;

			while (--i > 0	&& (tx_status = inb(ioaddr + TX_STATUS)) > 0) {
				//if (tx_status & 0x38) dev->stats.tx_aborted_errors++;
				if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
				if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
				outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
			}
		}
#endif
		// FIXME - need a better way to do this. Maybe trust interrupts ...
		while (inb(ioaddr + EL3_STATUS) & CmdBusy) 
			el3_mdelay(1);		// Wait - for now, to avoid collision

		res = len;
		//printk("%02x@", inb(ioaddr + TX_STATUS));
		break;
	}
	return res;
}

/* *****************************************************************************************************
	A note about ELKS, EL3 and Interrupts
	ELKS does not service network interrupts as they arrive. Instead, when the application calls the
	driver, the status is checked and a read initiated if data is ready - or a write is initiated if 
	the interface is ready. Otherwise the application is but to sleep, to be awakened by the wake_up
	calls from the driver. A more traditional approach is to act on the cause of an interrupt 
	- e.g. transfer an arrived packet into a buffer, immediately.
	The 3Com 3C509 family of NICs expect the latter, and the RxComplete and RxEarly interrupt status
	bits can only be reset by emptying the NIC's FIFO.
	In order to get this scheme to work with ELKS, we mask off the RxComplete interrupt immediately 
	after seeing it, and reenable it in the packet read routine when the FIFO is empty. Not optimal
	from a performance point of view, but it works and will do for now. 

	As to Transmits, the TxComplete interrupt from the 3c5xx NICs indicate a transmit error. 
	The TxAvailable interrupt signals the availability of enough space in the output FIFO to hold a
	packet of a predetermined size. Since ELKS (ktcp) limits the send to 512 bytes and we rarely experiment 
	with sizes above 1k, this driver sets the limit to 1040 bytes.
	There are definite performance improvements to be gained by tuning this. 
 *******************************************************************************************************/

static void el3_int(int irq, struct pt_regs *regs)
{
	unsigned int status;
	int i = max_interrupt_work;

	outw(SetIntrEnb | 0x0, ioaddr + EL3_CMD);	// Block interrupts
	//printk("I");

	while ((status = inw(ioaddr + EL3_STATUS)) & active_imask) {
		//printk("/%04x;", status);

		if (status & (RxEarly | RxComplete)) {
			//printk("r:%04x;", inw(ioaddr + RX_STATUS));
			wake_up(&rxwait);
			//outw(SetRxThreshold | 60, ioaddr + EL3_CMD);	// Reactivate before ack
			active_imask &= ~RxComplete;	// Disable RxComplete for now
		}

		if (status & TxAvailable) {
			//printk("t:%02x;", inb(ioaddr + TX_STATUS));
			if (el3_debug > 5)
				pr_debug("TxAvailable.\n");
			/* There's room in the FIFO for a full-sized packet. */
			outw(AckIntr | TxAvailable, ioaddr + EL3_CMD);
			wake_up(&txwait);
		}
		if (status & (AdapterFailure | StatsFull | TxComplete)) {
			//printk("F");
			/* Handle all uncommon interrupts. */
			if (status & StatsFull)			/* Empty statistics. */
				update_stats();
#if 0			// Not currently used
			if (status & RxEarly) {
				//el3_rx(dev);
				outw(AckIntr | RxEarly, ioaddr + EL3_CMD);
				printk("RxEarly int\n");
			}
#endif
			if (status & TxComplete) {		/* Really Tx error. */
				short tx_status;
				int i = 4;

				printk("el3: Transmit error, status %02x\n", inb(ioaddr + TX_STATUS));
				while (--i>0 && (tx_status = inb(ioaddr + TX_STATUS)) > 0) {
					//if (tx_status & 0x38) dev->stats.tx_aborted_errors++;
					if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
					if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
					outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
				}
			}
			// Receive overflow
			if (status & AdapterFailure) {
				printk("eth: NIC overflow, status %04x\n", status);
				/* Adapter failure requires Rx reset and reinit. */
				outw(RxReset, ioaddr + EL3_CMD);
				/* Set the Rx filter to the current state. */
				outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);

				outw(RxEnable, ioaddr + EL3_CMD); /* Re-enable the receiver. */
				outw(AckIntr | AdapterFailure, ioaddr + EL3_CMD);
			}
		}

		if (--i < 0) {	// This should not happen
			printk("eth: EL3 Infinite loop in interrupt, status %4.4x.\n",
				   status);

			/* Clear all interrupts. */
			outw(AckIntr | 0xFF, ioaddr + EL3_CMD);
			break;
		}
		/* General acknowledge the IRQ. */
		outw(AckIntr | IntReq | IntLatch, ioaddr + EL3_CMD); /* Ack IRQ */
	}

	outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);
	//printk("i%04x!", inw(ioaddr + EL3_STATUS));
	if (el3_debug > 4) {
		pr_debug("EL3: exiting interrupt, status %4.4x.\n",
			   inw(ioaddr + EL3_STATUS));
	}

	return;
}


#if LATER
static struct net_device_stats *
el3_get_stats(struct net_device *dev)
{
	struct el3_private *lp = netdev_priv(dev);
	unsigned long flags;

	/*
	 *	This is fast enough not to bother with disable IRQ
	 *	stuff.
	 */

	spin_lock_irqsave(&lp->lock, flags);
	update_stats(dev);
	spin_unlock_irqrestore(&lp->lock, flags);
	return &dev->stats;
}

#endif

/*  Update statistics.  We change to register window 6, so this should be run
	single-threaded if the device is active. This is expected to be a rare
	operation, and it's simpler for the rest of the driver to assume that
	window 1 is always valid rather than use a special window-state variable.
	*/
/* ELKS: Dummy for now */
static void update_stats( void )
{

	if (el3_debug > 5)
		pr_debug("   Updating the statistics.\n");

	/* Turn off statistics updates while reading. */
	outw(StatsDisable, ioaddr + EL3_CMD);

	/* Switch to the stats window, and read everything. */
	EL3WINDOW(6);
#if 0
	dev->stats.tx_carrier_errors 	+= inb(ioaddr + 0);
	dev->stats.tx_heartbeat_errors	+= inb(ioaddr + 1);
	/* Multiple collisions. */	   inb(ioaddr + 2);
	dev->stats.collisions		+= inb(ioaddr + 3);
	dev->stats.tx_window_errors	+= inb(ioaddr + 4);
	dev->stats.rx_fifo_errors	+= inb(ioaddr + 5);
	dev->stats.tx_packets		+= inb(ioaddr + 6);
	/* Rx packets	*/		   inb(ioaddr + 7);
	/* Tx deferrals */		   inb(ioaddr + 8);
#else
	{
	int i = 0;
	while (i++ < 9) inb(ioaddr + i);
	}
#endif
	inw(ioaddr + 10);	/* Total Rx and Tx octets. */
	inw(ioaddr + 12);

	/* Back to window 1, and turn statistics back on. */
	EL3WINDOW(1);
	outw(StatsEnable, ioaddr + EL3_CMD);
}


/*
 * Release (close) device
 */

static void el3_release(struct inode *inode, struct file *file)
{
	el3_down();

	netif_stat.if_status &= ~NETIF_IS_OPEN;

	/* Switching back to window 0 disables the IRQ. */
	//EL3WINDOW(0);
	/* But we explicitly zero the IRQ line select anyway. Don't do
	 * it on EISA cards, it prevents the module from getting an
	 * IRQ after unload+reload... */
	//outw(0x0f00, ioaddr + WN0_IRQ);

	return;
}


static size_t el3_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
	short rx_status;
	size_t res;

	while(1) {
		
		//printk("R");
		prepare_to_wait_interruptible(&rxwait);

		//if (!(rx_status & 0x7ff)) {
		if ((rx_status = inw(ioaddr + RX_STATUS)) & 0x8000) {
			if (filp->f_flags & O_NONBLOCK) {
				res = -EAGAIN;
				break;
			}
			do_wait();
			if (current->signal) {
				res = -EINTR;
				break;
			}
		}
		//printk("%d", rx_status&0x7ff);		// DEBUG
		// Early read is not implemented, thus only one read operation.
		// Assuming that we actually have data.
		res = -EIO;
		if (rx_status & 0x4000) { /* Error, update stats. */
			//short error = rx_status & 0x3800;

			outw(RxDiscard, ioaddr + EL3_CMD);
#ifdef LATER		// FIX stats later
			dev->stats.rx_errors++;

			switch (error) {
			case 0x0000:	dev->stats.rx_over_errors++; break;
			case 0x0800:	dev->stats.rx_length_errors++; break;
			case 0x1000:	dev->stats.rx_frame_errors++; break;
			case 0x1800:	dev->stats.rx_length_errors++; break;
			case 0x2000:	dev->stats.rx_frame_errors++; break;
			case 0x2800:	dev->stats.rx_crc_errors++; break;
			}
#endif
			inw(ioaddr + EL3_STATUS); 				/* Delay. */
			while (inw(ioaddr + EL3_STATUS) & 0x1000)
				pr_debug("	Waiting for 3c509 to discard packet, status %x.\n",
					  inw(ioaddr + EL3_STATUS) );
			res = -EIO;
		} else {
			short pkt_len = rx_status & 0x7ff;
			if (el3_debug > 4)
				pr_debug("Receiving packet size %d status %4.4x.\n",
					  pkt_len, rx_status);
			el3_insw(ioaddr + RX_FIFO, data, (pkt_len + 1) >> 1); //Word size

			outw(RxDiscard, ioaddr + EL3_CMD); /* Pop top Rx packet. */
			res = pkt_len;
			
		}
		break;
	}
	
	finish_wait(&rxwait);
	rx_status = inw(ioaddr + RX_STATUS);
	if (rx_status & 0x07ff)
		wake_up(&rxwait);	// more data
	else {
		active_imask |= RxComplete;	// activate 
		outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);
	}

	//printk("RR%d:", res);
	return res;
}

static void el3_down( void )
{

	/* Turn off statistics ASAP.  We update lp->stats below. */
	outw(StatsDisable, ioaddr + EL3_CMD);

	/* Disable the receiver and transmitter. */
	outw(RxDisable, ioaddr + EL3_CMD);
	outw(TxDisable, ioaddr + EL3_CMD);

	/* Disable link beat and jabber, if_port may change here next open(). */
	EL3WINDOW(4);
	outw(inw(ioaddr + WN4_MEDIA) & ~MEDIA_TP, ioaddr + WN4_MEDIA);

	outw(SetIntrEnb | 0x0, ioaddr + EL3_CMD);
}

/* Open el3 device: All initialization is done here, dev_init does the probe 
 * (verify existence) and pulls out the MAC address, that's it.
 */

static int el3_open(struct inode *inode, struct file *file)
{
	int i;

	if (!(netif_stat.if_status & NETIF_FOUND)) 
		return(-EINVAL);	// Does not exist 
	if (netif_stat.if_status & NETIF_IS_OPEN) {
		return(-EBUSY);		// Already open
	} 

	/* Activating the board - done already, repeat doesn't harm */
	outw(ENABLE_ADAPTER, ioaddr + WN0_CONF_CTRL);

	/* Set the IRQ line. */
	outw((net_irq << 12) | 0x0f00, ioaddr + WN0_IRQ);

	/* Set the station address in window 2 each time opened. */
	// FIXME: May not be required, done at init time ... 
	EL3WINDOW(2);

	// Set the MAC address
	for (i = 0; i < 6; i++)
		outb(mac_addr[i], ioaddr + i);
	outw(RxReset, ioaddr + EL3_CMD);
	outw(TxReset, ioaddr + EL3_CMD);	// FIXME: Join these two

	EL3WINDOW(1);
	for (i = 0; i < 31; i++)
		inb(ioaddr+TX_STATUS);		// Clear TX status stack

	outw(AckIntr | 0xff, ioaddr + EL3_CMD);	// Get rid of stray interrupts

	// The IntStatusEnb reg defines which interrupts will be visible,
	// The SetIntrEnb reg defines which of the visible interrupts will actually 
	// trigger an interrupt (the INTR mask). 
	outw(IntStatusEnb | 0xff, ioaddr + EL3_CMD); // Allow all status bits to be seen,
						     // May want to restrict this to the
						     // 'interesting' bits
	// Allow these interrupts for now
	outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);

	/* Accept b-cast and phys addr only. */
	outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);

	// Skip thinnet & AUI; we support TP only
	EL3WINDOW(4);
	outw(MEDIA_TP, ioaddr + WN4_MEDIA);
	el3_mdelay(1000);
	EL3WINDOW(1);
	
	outw(SetTxThreshold | 1040, ioaddr + EL3_CMD); /* Signal TxAvailable when this # of 
							* bytes is available */
	//outw(SetRxThreshold | 60, ioaddr + EL3_CMD);	// Receive interrupts when 60 bytes
							// are available
	outw(RxEnable, ioaddr + EL3_CMD); /* Enable the receiver. */
	outw(TxEnable, ioaddr + EL3_CMD); /* Enable transmitter. */

#if LATER	// This is tuning, fix later
	{
		int sw_info, net_diag;

		/* Combine secondary sw_info word (the adapter level) and primary
			sw_info word (duplex setting plus other useless bits) */
		EL3WINDOW(0);
		sw_info = (read_eeprom(ioaddr, 0x14) & 0x400f) |
			(read_eeprom(ioaddr, 0x0d) & 0xBff0);
		//printk("sw_info %04x ", sw_info);
		EL3WINDOW(4);
		//net_diag = inw(ioaddr + WN4_NETDIAG);
		//net_diag = (net_diag | FD_ENABLE); /* temporarily assume full-duplex will be set */
		//pr_info("%s: ", dev->name);

		switch (dev->if_port & 0x0c) {
			case 12:
				/* force full-duplex mode if 3c5x9b */
				if (sw_info & 0x000f) {
					pr_cont("Forcing 3c5x9b full-duplex mode");
					break;
				}
				fallthrough;
			case 8:
				/* set full-duplex mode based on eeprom config setting */
				if ((sw_info & 0x000f) && (sw_info & 0x8000)) {
					pr_cont("Setting 3c5x9b full-duplex mode (from EEPROM configuration bit)");
					break;
				}
				fallthrough;
			default:
				/* xcvr=(0 || 4) OR user has an old 3c5x9 non "B" model */
				pr_cont("Setting 3c5x9/3c5x9B half-duplex mode");
				net_diag = (net_diag & ~FD_ENABLE); /* disable full duplex */
		}

		//outw(net_diag, ioaddr + WN4_NETDIAG);
		//if (el3_debug > 3)
		//	pr_debug("%s: 3c5x9 net diag word is now: %4.4x.\n", dev->name, net_diag);
		/* Enable link beat and jabber check. */
		outw(inw(ioaddr + WN4_MEDIA) | MEDIA_TP, ioaddr + WN4_MEDIA);
	}

	/* Switch to the stats window, and clear all stats by reading. */
	outw(StatsDisable, ioaddr + EL3_CMD);
	EL3WINDOW(6);
	for (i = 0; i < 9; i++)
		inb(ioaddr + i);
	inw(ioaddr + 10);
	inw(ioaddr + 12);

	/* Switch to register set 1 for normal use. */
	EL3WINDOW(1);

	/* Accept b-cast and phys addr only. */
	outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);
	//outw(StatsEnable, ioaddr + EL3_CMD); /* Turn on statistics. */

	outw(RxEnable, ioaddr + EL3_CMD); /* Enable the receiver. */
	outw(TxEnable, ioaddr + EL3_CMD); /* Enable transmitter. */
	/* Allow status bits to be seen. */
	outw(IntStatusEnb | 0xff, ioaddr + EL3_CMD);
	/* Ack all pending events, and set active indicator mask. */
	outw(AckIntr | IntLatch | TxAvailable | RxEarly | IntReq,
		 ioaddr + EL3_CMD);
	outw(SetIntrEnb | IntLatch|TxAvailable|TxComplete|RxComplete|StatsFull,
		 ioaddr + EL3_CMD);
	
#endif
	netif_stat.if_status |= NETIF_IS_OPEN;
	return 0;
}

/*
 * I/O control
 */

static int el3_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned int arg)
{       
	int err = 0;
	byte_t *mac_addr = (byte_t *)&netif_stat.mac_addr;
	
	switch (cmd) {
		case IOCTL_ETH_ADDR_GET:
			verified_memcpy_tofs((byte_t *)arg, mac_addr, 6);
			break;

#if LATER       
		case IOCTL_ETH_ADDR_SET:
			verified_memcpy_fromfs(mac_addr, (byte_t *) arg, 6);
			ne2k_addr_set(mac_addr); 
			printk("eth: MAC address changed to %02x", mac_addr[0]);
			for (i = 1; i < 6; i++) printk(":%02x", mac_addr[i]&0xff);
			printk("\n");
			break;
#endif	  
		
		case IOCTL_ETH_GETSTAT:
			/* Return the entire netif_struct */
			verified_memcpy_tofs((char *)arg, &netif_stat, sizeof(netif_stat));
			break;
		
		default:
			err = -EINVAL;
	
	}
	return err;
}

/*
 * Test for readiness
 */

int el3_select(struct inode *inode, struct file *filp, int sel_type)
{       
	int res = 0;
	
	//printk("S:%d;",sel_type);
	switch (sel_type) {
		case SEL_OUT:
			// FIXME - may need a more accurate tx status test
			if (inw(ioaddr + TX_FREE) < MAX_PACKET_ETH) {
				select_wait(&txwait);
				break;
			}
			res = 1;
			break;
		
		case SEL_IN:
			// FIXME: Unsure whether this is the optimal
			// test for select()
			//if (!(inw(ioaddr+EL3_STATUS) & RxComplete)) {
			if (inw(ioaddr+RX_STATUS) & 0x8000) {
				select_wait(&rxwait);
				break;
			}
			res = 1;
			break;
		
		default:
			res = -EINVAL;
	}
	return res;
}

