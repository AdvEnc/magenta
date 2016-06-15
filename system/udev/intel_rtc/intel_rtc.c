// Copyright 2016 The Fuchsia Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/protocol/char.h>
#include <hw/inout.h>

#include <magenta/types.h>
#include <magenta/syscalls.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define RTC_IO_BASE 0x70
#define RTC_NUM_IO_REGISTERS 8

#define RTC_IDX_REG 0x70
#define RTC_DATA_REG 0x71

enum intel_rtc_registers {
    REG_SECONDS,
    REG_SECONDS_ALARM,
    REG_MINUTES,
    REG_MINUTES_ALARM,
    REG_HOURS,
    REG_HOURS_ALARM,
    REG_DAY_OF_WEEK,
    REG_DAY_OF_MONTH,
    REG_MONTH,
    REG_YEAR,
};


struct rtc_time {
    uint8_t seconds, minutes, hours;
};
static void read_time(struct rtc_time *t) {
    outp(RTC_IDX_REG, REG_SECONDS);
    t->seconds = inp(RTC_DATA_REG);
    outp(RTC_IDX_REG, REG_MINUTES);
    t->minutes = inp(RTC_DATA_REG);
    outp(RTC_IDX_REG, REG_HOURS);
    t->hours = inp(RTC_DATA_REG);
}

static bool rtc_time_eq(struct rtc_time *lhs, struct rtc_time *rhs) {
    return lhs->seconds == rhs->seconds &&
            lhs->minutes == rhs->minutes &&
            lhs->hours == rhs->hours;
}

// implement char protocol
static ssize_t intel_rtc_read(mx_device_t* dev, void* buf, size_t count) {
    struct rtc_time cur, last;

    read_time(&cur);
    do {
        last = cur;
        read_time(&cur);
    } while (!rtc_time_eq(&cur, &last));

    int n = snprintf(buf, count, "%02x:%02x:%02x", cur.hours, cur.minutes, cur.seconds);
    if (n < 0) {
        return ERR_GENERIC;
    }
    if ((unsigned int)n > count) {
        return ERR_NOT_ENOUGH_BUFFER;
    }
    return n;
}

static ssize_t intel_rtc_write(mx_device_t* dev, const void* buf, size_t count) {
    return ERR_NOT_SUPPORTED;
}

static mx_protocol_char_t intel_rtc_char_proto = {
    .read = intel_rtc_read,
    .write = intel_rtc_write,
};

// implement device protocol

static mx_status_t intel_rtc_open(mx_device_t* dev, uint32_t flags) {
    return NO_ERROR;
}

static mx_status_t intel_rtc_close(mx_device_t* dev) {
    return NO_ERROR;
}

static mx_status_t intel_rtc_release(mx_device_t* dev) {
    return NO_ERROR;
}

static mx_protocol_device_t intel_rtc_device_proto = {
    .get_protocol = device_base_get_protocol,
    .open = intel_rtc_open,
    .close = intel_rtc_close,
    .release = intel_rtc_release,
};

// implement driver object:
//
static mx_status_t intel_rtc_init(mx_driver_t* drv) {
#if ARCH_X86
    // TODO(teisenbe): This should be probed via the ACPI pseudo bus whenever it
    // exists.

    mx_status_t status = _magenta_mmap_device_io(RTC_IO_BASE, RTC_NUM_IO_REGISTERS);
    if (status != NO_ERROR) {
        return status;
    }

    mx_device_t* dev;
    status = device_create(&dev, drv, "rtc", &intel_rtc_device_proto);
    if (status != NO_ERROR) {
        return status;
    }

    dev->protocol_id = MX_PROTOCOL_CHAR;
    dev->protocol_ops = &intel_rtc_char_proto;
    status = device_add(dev, NULL);
    if (status != NO_ERROR) {
        free(dev);
        return status;
    }
    return NO_ERROR;
#else
    intel_rtc_device_proto = intel_rtc_device_proto;
    intel_rtc_char_proto = intel_rtc_char_proto;
    return ERR_NOT_SUPPORTED;
#endif
}

mx_driver_t _driver_intel_rtc BUILTIN_DRIVER = {
    .name = "intel_rtc",
    .ops = {
        .init = intel_rtc_init,
    },
};