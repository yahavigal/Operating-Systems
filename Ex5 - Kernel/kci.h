/*
 * kci.h
 *
 *  Created on: 28 Jan 2017
 *      Author: osboxes
 */

#ifndef KCI_H_
#define KCI_H_

#include <linux/ioctl.h>
#define MAJOR_NUM 245
#define MINOR_NUM 0
#define SUBDIR "kcikmod"
#define FILE "calls"
#define OLDLOG "/sys/kernel/debug/kcikmod/calls"
#define NEWLOG "/calls"
#define BUFSIZE 1024
#define LOGBUF 512

#define DEVICE_FILE_NAME "/dev/kci_dev"
#define IOCTL_SET_PID _IOW(MAJOR_NUM,0,int)
#define IOCTL_SET_FD _IOW(MAJOR_NUM,1,int)
#define IOCTL_CIPHER _IOW(MAJOR_NUM,2,int)
#define DEVICE_RANGE_NAME "kci_kmod"
#endif /* KCI_H_ */

