#ifndef __PRINTF__
#define __PRINTF__
#include<linux/device.h>

#define __RMI_DEBUG__
#ifdef __RMI_DEBUG__

#define dbg_printk(fmt,args...)   \
 printk(KERN_DEBUG"Robin->%s_%d:"fmt,__FUNCTION__,__LINE__,##args)
#else
#define dbg_printk(fmt,args...)
#endif

#define err_printk(fmt,args...)   \
printk(KERN_ERR"Robin->%s_%d:"fmt,__FUNCTION__,__LINE__,##args)
#endif
