/*
 * Automatically generated by make menuconfig: don't edit.
 */

#define AUTOCONF_INCLUDED

/*
 * Kernel & Hardware
 */


/*
 * Hardware
 */

#define CONFIG_ARCH_IBMPC 1
#undef CONFIG_ARCH_SIBO
#undef CONFIG_ARCH_8018X
#define CONFIG_PC_AUTO 1
#undef CONFIG_PC_XT
#undef CONFIG_PC_AT
#undef CONFIG_PC_MCA
#define CONFIG_IBMPC_CLONE 1
#undef CONFIG_IBMPC_COMPAQ
#define CONFIG_CPU_8086 1
#undef CONFIG_CPU_80186
#undef CONFIG_CPU_80286
#define CONFIG_HW_VGA 1
#define CONFIG_HW_SERIAL_FIFO 1
#define CONFIG_HW_KEYBOARD_BIOS 1
#define CONFIG_HW_VIDEO_HOC 1
#define CONFIG_APM 1
#define CONFIG_ARCH_PC98 1

/*
 * Kernel settings
 */

#define CONFIG_SMALL_KERNEL 1
#undef CONFIG_BOOTOPTS
#undef CONFIG_STRACE
#undef CONFIG_IDLE_HALT
#undef CONFIG_ROMCODE
#define CONFIG_EXPERIMENTAL 1
#define CONFIG_FARTEXT_KERNEL 1

/*
 * Networking Support
 */

#undef CONFIG_SOCKET

/*
 * Filesystem Support
 */

#define CONFIG_MINIX_FS 1
#undef CONFIG_ROMFS_FS
#define CONFIG_FS_FAT 1
#define CONFIG_FS_DEV 1
#undef CONFIG_ROOT_READONLY
#undef CONFIG_FS_RO
#define CONFIG_FULL_VFS 1
#define CONFIG_32BIT_INODES 1
#define CONFIG_FS_EXTERNAL_BUFFER 1
#define CONFIG_FS_NR_EXT_BUFFERS (64)
#define CONFIG_PIPE 1
#define CONFIG_EXEC_COMPRESS 1
#define CONFIG_EXEC_MMODEL 1
#define CONFIG_EXEC_MMODEL 1

/*
 * Drivers
 */


/*
 * Block device drivers
 */

#define CONFIG_BLK_DEV_BIOS 1
#define CONFIG_BLK_DEV_BFD 1
#undef CONFIG_BLK_DEV_BFD_HARD
#undef CONFIG_TRACK_CACHE
#undef CONFIG_BLK_DEV_BHD
#undef CONFIG_IDE_PROBE
#undef CONFIG_BLK_DEV_FD
#undef CONFIG_BLK_DEV_HD
#undef CONFIG_DMA
#define CONFIG_BLK_DEV_RAM 1
#define CONFIG_RAMDISK_SEGMENT 0x0
#define CONFIG_RAMDISK_SECTORS (128)
#define CONFIG_BLK_DEV_CHAR 1

/*
 * Character device drivers
 */

#undef CONFIG_CONSOLE_DIRECT
#undef CONFIG_CONSOLE_BIOS
#define CONFIG_CONSOLE_HEADLESS 1
#undef CONFIG_KEYBOARD_SCANCODE
#undef CONFIG_CONSOLE_SERIAL
#undef CONFIG_CHAR_DEV_RS
#define CONFIG_CHAR_DEV_LP 1
#undef CONFIG_CHAR_DEV_CGATEXT
#define CONFIG_CHAR_DEV_MEM 1
#undef CONFIG_CHAR_DEV_MEM_PORT_READ
#undef CONFIG_CHAR_DEV_MEM_PORT_WRITE
#define CONFIG_PSEUDO_TTY 1
#undef CONFIG_DEV_META

/*
 * Network device drivers
 */

#undef CONFIG_ETH

/*
 * Userland
 */

#undef CONFIG_APPS_BY_IMAGESZ
#define CONFIG_APP_ASH 1
#undef CONFIG_APP_SASH
#undef CONFIG_APP_BUSYELKS
#undef CONFIG_APP_CGATEXT
#define CONFIG_APP_SCREEN 1
#undef CONFIG_APP_CRON
#define CONFIG_APP_DISK_UTILS 1
#define CONFIG_APP_FILE_UTILS 1
#define CONFIG_APP_SH_UTILS 1
#define CONFIG_APP_SYS_UTILS 1
#define CONFIG_APP_ELVIS 1
#define CONFIG_APP_MINIX1 1
#define CONFIG_APP_MINIX2 1
#define CONFIG_APP_MINIX3 1
#define CONFIG_APP_MISC_UTILS 1
#undef CONFIG_APP_MTOOLS
#undef CONFIG_APP_NANOX
#define CONFIG_APP_OTHER 1
#undef CONFIG_APP_BYACC
#undef CONFIG_APP_M4
#undef CONFIG_APP_TEST

/*
 * Target image
 */

#undef CONFIG_IMG_MINIX
#define CONFIG_IMG_FAT 1
#undef CONFIG_IMG_RAW
#undef CONFIG_IMG_ROM
#undef CONFIG_IMG_FD2880
/*
#define CONFIG_IMG_FD1440 1
*/
#undef CONFIG_IMG_FD1440
#define CONFIG_IMG_FD1232
#undef CONFIG_IMG_FD1200
#undef CONFIG_IMG_FD720
#undef CONFIG_IMG_FD360
#undef CONFIG_IMG_HD
#define CONFIG_IMG_DEV 1
#define CONFIG_IMG_BOOT 1
#undef CONFIG_IMG_EXTRA_IMAGES