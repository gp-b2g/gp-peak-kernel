/*
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM cpu_hotplug

#if !defined(_TRACE_EVENT_CPU_HOTPLUG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_EVENT_CPU_HOTPLUG_H

#include <linux/tracepoint.h>

TRACE_EVENT(cpu_online,
	TP_PROTO(unsigned int on),

	TP_ARGS(on),

	TP_STRUCT__entry(
		__field(	unsigned int,	on)
	),

	TP_fast_assign(
		__entry->on	= on;
	),

	TP_printk("turn cpu %s", __entry->on ? "on" : "off")
);

#endif

/* This part must be outside protection */
#include <trace/define_trace.h>

