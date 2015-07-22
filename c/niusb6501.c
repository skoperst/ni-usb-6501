#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "niusb6501.h"

#define NI_VENDOR_ID	0x3923
#define USB_6501_PRODUCT_ID	0x718a

#define EP_IN	0x81
#define EP_OUT	0x01

#define TIMEOUT 1000

#define PACKET_HEADER_LEN	4
#define DATA_HEADER_LEN		4

#define PROTOCOL_ERROR		(-EPROTO)
#define BUFFER_TOO_SMALL	(-ENOSPC)

#if 0
#define DEBUG(args...) fprintf(stderr , ## args);
#else
#define DEBUG(args...)
#endif

//#define DUMP_BUFFERS

libusb_context *ctx; //a libusb session

#ifdef DUMP_BUFFERS
void dump_buffer(size_t len, const void *buffer)
{
    size_t i;

    printf("%02x: ", len);
    for(i = 0; i < len; i++)
        printf("%02x ", ((unsigned char *) buffer)[i]);
    printf("\n");
}
#endif

int niusb6501_is_success(size_t len, const void *buffer)
{
    static const char success_packet[8] = "\x00\x08\x01\x00\x00\x00\x00\x02";

    if(len != sizeof success_packet)
        return 0;
    return !memcmp(buffer, success_packet, sizeof success_packet);
}

int niusb6501_packet_matches(size_t len, const void *buffer, size_t expect_len, const void *expect, const void *mask)
{
    size_t i;
    unsigned char b, e, m;
    if(len != expect_len)
        return 0;
    for(i = 0; i < len; i++)
    {
        b = ((unsigned char *) buffer)[i];
        e = ((unsigned char *) expect)[i];
        m = ((unsigned char *) mask)[i];

        if((b & m) != (e & m))
            return 0;
    }
    return 1;
}

int niusb6501_send_request(struct libusb_device_handle *usb_handle, unsigned char cmd, size_t request_len, const void *request, size_t *result_len, void *result)
{
    unsigned char buffer[256];
    int status, actualLength;

    if(request_len > 255-(PACKET_HEADER_LEN+DATA_HEADER_LEN))
    {
        DEBUG("request too long (%d > %d bytes)\n", request_len, 255-(PACKET_HEADER_LEN+DATA_HEADER_LEN));
        return -EINVAL;
    }

    buffer[0] = 0x00;
    buffer[1] = 0x01;
    buffer[2] = 0x00;
    buffer[3] = request_len+PACKET_HEADER_LEN+DATA_HEADER_LEN;
    buffer[4] = 0x00;
    buffer[5] = request_len+DATA_HEADER_LEN;
    buffer[6] = 0x01;
    buffer[7] = cmd;

    memcpy(&buffer[PACKET_HEADER_LEN+DATA_HEADER_LEN], request, request_len);
#ifdef DUMP_BUFFERS
    dump_buffer(request_len+PACKET_HEADER_LEN+DATA_HEADER_LEN, buffer);
#endif

    /* send command */
    DEBUG("send command\n");

    status = libusb_bulk_transfer(
                usb_handle,
                EP_OUT,
                buffer,
                request_len+PACKET_HEADER_LEN+DATA_HEADER_LEN,
                &actualLength,
                TIMEOUT
                );
    if (status < 0)
        return status;

    /* read result */
    DEBUG("read result\n");
    status = libusb_bulk_transfer(
                usb_handle,
                EP_IN,
                buffer,
                sizeof buffer,
                &actualLength,
                TIMEOUT
                );
    if(status < 0)
        return status;
#ifdef DUMP_BUFFERS
    dump_buffer(actualLength, buffer);
#endif
    if(actualLength < PACKET_HEADER_LEN)
        return PROTOCOL_ERROR;
    if(actualLength-PACKET_HEADER_LEN > *result_len)
        return BUFFER_TOO_SMALL;
    *result_len = actualLength-PACKET_HEADER_LEN;
    memcpy(result, &buffer[PACKET_HEADER_LEN], actualLength-PACKET_HEADER_LEN);

    return 0;
}

/*
 * niusb6501_list_devices
 *
 * DESCRIPTION:
 * Lists all NI USB-6501 devices found on the system.
 *
 * PARAMS:
 * devices	pointer to array of struct usb_device *, where the list
 *		of devices will be stored
 * size		maximum number of entries in devices
 * serial   the requested device serial number in string format pass NULL if you want to retrive all devices present on the system
 * RETURNS:
 * number of devices that have been detected
 */

size_t niusb6501_list_devices(libusb_device *devices[], size_t size, const char *serialID)
{
    struct libusb_device_handle *deviceHandle = NULL;
    struct libusb_device_descriptor desc;
    struct libusb_device **list;
    struct libusb_device *device;
    char serial[9];

    ssize_t cnt = 0, count = 0;
    ssize_t i = 0;
    int err = 0;

    if(libusb_init(&ctx) != 0) {
        ctx = NULL;
        DEBUG(stderr, "LIBUSB init failed\n");
        return -1;
    } else {
        libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
    }

    // discover devices
    cnt = libusb_get_device_list(NULL, &list);

    if (cnt < 0) {
        perror("No USB devices found on bus.");
        return -1;
    }

    for (i = 0; i < cnt; i++) {
        device = list[i];
        err = libusb_get_device_descriptor(device, &desc);
        if (err < 0) goto out;
        if (desc.idVendor == NI_VENDOR_ID && desc.idProduct == USB_6501_PRODUCT_ID) {
            err = libusb_open(device, &deviceHandle);
            if (err < 0) {
                deviceHandle = NULL;
                continue;
            }
            err = libusb_kernel_driver_active(deviceHandle, 0);
            if (err == 1) {
                /*
                 * device active by another driver. (like HID).  This can be dangerous,
                 * as we don't know if the kenel has claimed the interface or another
                 * process.  No easy way to tell at this moment, but HID devices won't
                 * work otherwise.
                 */
                libusb_detach_kernel_driver(deviceHandle, 0);
            }
            err = libusb_claim_interface(deviceHandle, 0);
            if (err < 0) {
                printf("error claiming interface 0.\n");
                libusb_close(deviceHandle);
                deviceHandle = NULL;
                continue;
            }

            /* Check to see if serialnumber  match if it was specified */
            if (serialID != NULL) {
                libusb_get_string_descriptor_ascii(deviceHandle, desc.iSerialNumber, (unsigned char *) serial, 8);
                serial[9] = '\0';
                if (strcmp(serialID, serial) == 0) {
                    if (count < size) {
                        devices[count++] = device;
                        return 1;
                    }
                }
            } else {
                if (count < size) {
                    devices[count++] = device;
                }
            }
            libusb_close(deviceHandle);
            deviceHandle = NULL;
        }
    }

    //libusb_free_device_list(list,1);
    return count;

out:
    libusb_free_device_list(list,1);
    if (deviceHandle != NULL)
        libusb_close(deviceHandle);
    libusb_exit(NULL);
    return -1;
}

/*
 * niusb6501_open_device
 *
 * DESCRIPTION:
 * Opens an NI USB-6501 device.
 *
 * PARAMS:
 * device	pointer to a struct usb_device
 *
 * RETURNS:
 * handle for the device on success
 * NULL on error
 */

struct libusb_device_handle *niusb6501_open_device(struct libusb_device *device)
{
    struct libusb_device_handle *handle = NULL;
    if (libusb_open(device, &handle) != LIBUSB_SUCCESS)
        return NULL;
    return handle;
}

/*
 * niusb6501_close_device
 *
 * DESCRIPTION:
 * Closes an NI USB-6501 device that has been opened with niusb6501_open_device.
 *
 * PARAMS:
 * handle	handle of the device
 *
 */

void niusb6501_close_device(struct libusb_device_handle *handle)
{
    libusb_close(handle);
}

/*
 * niusb6501_start_counter
 *
 * DESCRIPTION:
 * Starts the counter.
 *
 * RETURNS:
 * 0 on success
 * <0 on error
 */

int niusb6501_start_counter(libusb_device_handle *handle)
{
    unsigned char result[8];
    size_t result_len = sizeof result;
    static const unsigned char request[4] = "\x02\x20\x00\x00";
    int status;

    status = niusb6501_send_request(handle, 0x09, sizeof request, request, &result_len, result);
    if(status < 0)
        return status;
    if(!niusb6501_is_success(result_len, result))
        return PROTOCOL_ERROR;
    return 0;
}

/*
 * niusb6501_stop_counter
 *
 * DESCRIPTION:
 * Stops the counter.
 *
 * RETURNS:
 * 0 on success
 * <0 on error
 */

int niusb6501_stop_counter(struct libusb_device_handle *handle)
{
    unsigned char result[8];
    size_t result_len = sizeof result;
    static const unsigned char request[4] = "\x02\x20\x00\x00";
    int status;

    status = niusb6501_send_request(handle, 0x0c, sizeof request, request, &result_len, result);
    if(status < 0)
        return status;
    if(!niusb6501_is_success(result_len, result))
        return PROTOCOL_ERROR;
    return 0;
}

/*
 * niusb6501_read_port
 *
 * DESCRIPTION:
 * Reads a port.
 *
 * PARAMS:
 * port		number of the port
 * value	pointer to a variable to receive the value
 *
 * RETURNS:
 * 0 on success
 * <0 on error
 */

int niusb6501_read_port(struct libusb_device_handle *handle, unsigned char port, unsigned char *value)
{
    unsigned char result[12];
    size_t result_len = sizeof result;
    unsigned char request[8] = "\x02\x10\x00\x00\x00\x03\x00\x00";
    int status;

    request[6] = port;

    status = niusb6501_send_request(handle, 0x0e, sizeof request, request, &result_len, result);
    if(status < 0)
        return status;
    if(!niusb6501_packet_matches(result_len, result, 12, "\x00\x0c\x01\x00\x00\x00\x00\x02\x00\x03\x00\x00", "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\xff"))
        return PROTOCOL_ERROR;

    *value = result[10];

    return 0;
}

/*
 * niusb6501_read_counter
 *
 * DESCRIPTION:
 * Reads the counter.
 *
 * PARAMS:
 * value	pointer to a variable to receive the value
 *
 * RETURNS:
 * 0 on success
 * <0 on error
 */

int niusb6501_read_counter(struct libusb_device_handle *handle, unsigned long *value)
{
    unsigned char result[12];
    size_t result_len = sizeof result;
    unsigned char request[4] = "\x02\x20\x00\x00";
    int status;

    status = niusb6501_send_request(handle, 0x0e, sizeof request, request, &result_len, result);
    if(status < 0)
        return status;

    if(!niusb6501_packet_matches(result_len, result, 12, "\x00\x0c\x01\x00\x00\x00\x00\x02\x00\x00\x00\x00", "\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00"))
        return PROTOCOL_ERROR;

    *value = (result[8] << 24) | (result[9] << 16) | (result[10] << 8) | result[11];

    return 0;
}

/*
 * niusb6501_write_port
 *
 * DESCRIPTION:
 * Writes a port.
 *
 * PARAMS:
 * port		number of the port
 * value	value to write to the port
 *
 * RETURNS:
 * 0 on success
 * <0 on error
 */

int niusb6501_write_port(struct libusb_device_handle *handle, unsigned char port, unsigned char value)
{
    unsigned char result[8];
    size_t result_len = sizeof result;
    unsigned char request[12] = "\x02\x10\x00\x00\x00\x03\x00\x00\x03\x00\x00\x00";
    int status;

    request[6] = port;
    request[9] = value;

    status = niusb6501_send_request(handle, 0x0f, sizeof request, request, &result_len, result);
    if(status < 0)
        return status;
    if(!niusb6501_is_success(result_len, result))
        return PROTOCOL_ERROR;
    return 0;
}

/*
 * niusb6501_write_counter
 *
 * DESCRIPTION:
 * Writes the counter.
 *
 * PARAMS:
 * value	value to write to the counter
 *
 * RETURNS:
 * 0 on success
 * <0 on error
 */

int niusb6501_write_counter(struct libusb_device_handle *handle, unsigned long value)
{
    unsigned char result[8];
    size_t result_len = sizeof result;
    unsigned char request[8] = "\x02\x20\x00\x00\x00\x00\x00\x00";
    int status;

    request[4] = (value >> 24) & 0xff;
    request[5] = (value >> 16) & 0xff;
    request[6] = (value >> 8)  & 0xff;
    request[7] = value         & 0xff;

    status = niusb6501_send_request(handle, 0x0f, sizeof request, request, &result_len, result);
    if(status < 0)
        return status;
    if(!niusb6501_is_success(result_len, result))
        return PROTOCOL_ERROR;
    return 0;
}

/*
 * niusb6501_set_io_mode
 *
 * DESCRIPTION:
 * Sets the I/O mode for the ports.
 *
 * PARAMS:
 * port0-port2	bit masks for the ports
 *		0 = input
 *		1 = output
 *
 * RETURNS:
 * 0 on success
 * <0 on error
 */

int niusb6501_set_io_mode(struct libusb_device_handle *handle, unsigned char port0, unsigned char port1, unsigned char port2)
{
    unsigned char result[8];
    size_t result_len = sizeof result;
    unsigned char request[16] = "\x02\x10\x00\x00\x00\x05\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00";
    int status;

    request[6] = port0;
    request[7] = port1;
    request[8] = port2;

    status = niusb6501_send_request(handle, 0x12, sizeof request, request, &result_len, result);
    if(status < 0)
        return status;
    if(!niusb6501_is_success(result_len, result))
        return PROTOCOL_ERROR;
    return 0;
}

