#ifndef _NIUSB6501_H_
#define _NIUSB6501_H_

#include <libusb-1.0/libusb.h>

extern size_t niusb6501_list_devices(struct libusb_device *devices[], size_t size, const char *serial);
extern struct libusb_device_handle *niusb6501_open_device(libusb_device *device);
extern int niusb6501_close_device(struct libusb_device_handle *handle);

extern int niusb6501_start_counter(struct libusb_device_handle *handle);
extern int niusb6501_stop_counter(struct libusb_device_handle *handle);
extern int niusb6501_read_counter(struct libusb_device_handle *handle, unsigned long *value);
extern int niusb6501_write_counter(struct libusb_device_handle *handle, unsigned long value);

extern int niusb6501_read_port(struct libusb_device_handle *handle, unsigned char port, unsigned char *value);
extern int niusb6501_write_port(struct libusb_device_handle *handle, unsigned char port, unsigned char value);

extern int niusb6501_set_io_mode(struct libusb_device_handle *handle, unsigned char port0, unsigned char port1, unsigned char port2);

#endif

