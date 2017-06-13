#include <stdio.h>
#include <usb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>


#define VENDOR_ID	0x3923
#define PRODUCT_ID	0x7272
#define PRODUCT_ID_FIRMWARELOAD	0x7269

struct libusb_context *ctx; //a libusb session

char *load_firmware(char *path)
{
	FILE *f = fopen(path, "rb");
	
	if (f == NULL){
		printf("Unable to open file: %s \n",path);
		return NULL;
	}
	
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  //same as rewind(f);

	printf("malocing.. \n");
	char *string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;
	return string;
	
}

void char_to_bin(unsigned char value, char *buffer)
{
	int i;

	for(i = 0; i < 8; i++)
		buffer[7-i] = value & (1 << i) ? '1' : '0';
	buffer[8] = '\0';
}

void print_usage(const char *progname)
{
	fprintf(stderr,	"Usage: %s [mode]\n"
			"-c\tcounter demo\n"
			"-r\tport read demo\n"
			"-w\tport write demo\n"
			"-f\tfirmware load demo\n"
			"-h\tshow help\n",
	        progname);
}

/*
size_t list_devices(struct usb_device *devices[], size_t size,int vid,int pid)
{
	struct usb_device *usb_dev;
	struct usb_bus *usb_bus;
	size_t count = 0;

	usb_find_busses();
	usb_find_devices();

	for(usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next)
	{
		for(usb_dev = usb_bus->devices; usb_dev; usb_dev = usb_dev->next)
		{
			printf("USB device found: %x:%x \n",usb_dev->descriptor.idVendor,usb_dev->descriptor.idProduct);
			if ((usb_dev->descriptor.idVendor == vid) &&
			    (usb_dev->descriptor.idProduct == pid))
			{
				if(count < size)
					devices[count++] = usb_dev;
			}
		}
	}

	return count;
}
*/


int get_ni6218_io_usb(struct libusb_device_handle **handle)
{
	struct libusb_device *dev;
	struct libusb_device **list;
	//struct libusb_device *device;
	struct libusb_device_descriptor desc;
	//struct libusb_device_handle *deviceHandle = NULL;
	
	int cnt = 0;
	int i = 0;
	int res = 0;
	
	cnt = libusb_get_device_list(NULL,&list);
	if (cnt < 0){
		printf("No USB Devices found! \n");
		return -1;
	}
	
	for (i = 0; i< cnt; i++){
		dev = list[i];
		res = libusb_get_device_descriptor(dev,&desc);
		if (res < 0){
			libusb_free_device_list(list,1);
			return -1;
		}
		
		if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID){
			printf("Found NI 6218 IO device \n");
			libusb_open(dev,handle);
			libusb_free_device_list(list,1);
			return 0; 
		}
	}
	
	libusb_free_device_list(list,1);
	return -1;
}

int get_ni6218_fwloader_usb(struct libusb_device_handle **handle)
{
	
	struct libusb_device *dev;
	struct libusb_device **list;
	//struct libusb_device *device;
	struct libusb_device_descriptor desc;
	//struct libusb_device_handle *deviceHandle = NULL;
	
	int cnt = 0;
	int i = 0;
	int res = 0;
	
	cnt = libusb_get_device_list(NULL,&list);
	if (cnt < 0){
		printf("No USB Devices found! \n");
		return -1;
	}
	
	for (i = 0; i< cnt; i++){
		dev = list[i];
		res = libusb_get_device_descriptor(dev,&desc);
		if (res < 0){
			libusb_free_device_list(list,1);
			return -1;
		}
		
		if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID_FIRMWARELOAD){
			printf("Found NI 6218 FW Download device \n");
			libusb_open(dev,handle);
			libusb_free_device_list(list,1);
			return 0; 
		}
	}
	
	libusb_free_device_list(list,1);
	return -1;
	
}
void refresh_usb_system()
{
	libusb_exit(NULL);
	libusb_init(&ctx);
}

int ni6218_send_firmware(libusb_device_handle *handle)
{
	int res = 0;
	//struct libusb_device_handle *handle;
	
	printf("------------------------\n");
	printf("Found USB NI 6128 FW device\n");
	
	//res = libusb_open(dev,&handle);
	//if (res < 0){
	//	fprintf(stderr, "Unable to open the USB device: %d\n", res);
	//	return res;
	//}
	
	
	char buf[512];
	buf[0] = 0;
	buf[1] = 1;
	buf[2] = 2;
	buf[3] = 3;
	//int usb_control_msg(usb_dev_handle *dev, int requesttype, int request,int value, int index, char *bytes, int size, int timeout);
	//1. First of all we need GET DESCRIPTOR Request
	
	//re = libusb_control_transfer
	//res = usb_get_descriptor_by_endpoint(handle,0x80,0x3,0x0,buf,4);
	//if (res < 0){
	////	printf("usb_get_descriptor_by_endpoint failed: %d \n",res);
	//	return -1;
	//}
	
	//get descriptor
	libusb_control_transfer(handle,LIBUSB_ENDPOINT_IN | (0x80 & 0xff), LIBUSB_REQUEST_GET_DESCRIPTOR,(0x03 << 8) | 0x3, 0x03, buf, 4, 1000);

	//2. GET INTERFACE
	libusb_control_transfer(handle,0x81,USB_REQ_GET_INTERFACE,0x0,0,buf,1,1000);
	
	//Load firmware:
	char *firmware1 = load_firmware("../firmware/NIUSBMXB621x.bin");
	char *firmware2 = load_firmware("../firmware/NIUSBMXB621x1.bin");
	//dump_buffer(strlen(firmware1),firmware1);
	//3. Start Firmware Upload
	buf[0] = 1;
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0xe600,0,buf,1,1000);
	
	
	//4.Sending first block (512 bytes)
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0,0,firmware1,512,1000);
	//4.Sending second block (512 bytes)
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x200,0,(firmware1+512),512,1000);
	//5.Fifth and final block
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x400,0,(firmware1+1024),260,1000);
	
	//Now verifying the firmware:
	libusb_control_transfer(handle,0xc0,160,0x0,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x200,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x400,0,buf,260,1000);
	
	buf[0] = 0;
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0xe600,0,buf,1,1000);
	
	libusb_control_transfer(handle,0xc0,176,0x0000,0,buf,2,1000);
	
	printf("RESPONSE: %x %x \n", buf[0],buf[1]);
	
	//Now firmware2:
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4000,0,(firmware2 + 0x4000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4200,0,(firmware2 + 0x4200),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4400,0,(firmware2 + 0x4400),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4600,0,(firmware2 + 0x4600),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4800,0,(firmware2 + 0x4800),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4A00,0,(firmware2 + 0x4A00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4C00,0,(firmware2 + 0x4C00),43,1000);
	
	//Verify
	libusb_control_transfer(handle,0xc0,165,0x4000,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,165,0x4200,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,165,0x4400,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,165,0x4600,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,165,0x4800,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,165,0x4A00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,165,0x4C00,0,buf,43,1000);
	
	
	buf[0] = 1;
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0xe600,0,buf,1,1000);
	
	//Now firmware3:
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x0000,0,(firmware2 + 0x0000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x0200,0,(firmware2 + 0x0200),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x0400,0,(firmware2 + 0x0400),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x0600,0,(firmware2 + 0x0600),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x0800,0,(firmware2 + 0x0800),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x0A00,0,(firmware2 + 0x0A00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x0C00,0,(firmware2 + 0x0C00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x0E00,0,(firmware2 + 0x0E00),512,1000);
	
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x1000,0,(firmware2 + 0x1000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x1200,0,(firmware2 + 0x1200),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x1400,0,(firmware2 + 0x1400),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x1600,0,(firmware2 + 0x1600),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x1800,0,(firmware2 + 0x1800),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x1A00,0,(firmware2 + 0x1A00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x1C00,0,(firmware2 + 0x1C00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x1E00,0,(firmware2 + 0x1E00),512,1000);
	
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x2000,0,(firmware2 + 0x2000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x2200,0,(firmware2 + 0x2200),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x2400,0,(firmware2 + 0x2400),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x2600,0,(firmware2 + 0x2600),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x2800,0,(firmware2 + 0x2800),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x2A00,0,(firmware2 + 0x2A00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x2C00,0,(firmware2 + 0x2C00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x2E00,0,(firmware2 + 0x2E00),512,1000);
	
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x3000,0,(firmware2 + 0x3000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x3200,0,(firmware2 + 0x3200),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x3400,0,(firmware2 + 0x3400),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x3600,0,(firmware2 + 0x3600),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x3800,0,(firmware2 + 0x3800),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x3A00,0,(firmware2 + 0x3A00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x3C00,0,(firmware2 + 0x3C00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x3E00,0,(firmware2 + 0x3E00),512,1000);


	//Verify
	libusb_control_transfer(handle,0xc0,160,0x0000,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x0200,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x0400,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x0600,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x0800,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x0A00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x0C00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x0E00,0,buf,512,1000);
	
	libusb_control_transfer(handle,0xc0,160,0x1000,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x1200,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x1400,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x1600,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x1800,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x1A00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x1C00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x1E00,0,buf,512,1000);
	
	libusb_control_transfer(handle,0xc0,160,0x2000,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x2200,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x2400,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x2600,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x2800,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x2A00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x2C00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x2E00,0,buf,512,1000);
	
	libusb_control_transfer(handle,0xc0,160,0x3000,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x3200,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x3400,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x3600,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x3800,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x3A00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x3C00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,160,0x3E00,0,buf,512,1000);
	
	//Now the grand finale
	buf[0] = 0;
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0xE600,0,buf,1,1000);
	
	printf("YESS!!! \n");
	libusb_close(handle);
}

int main(int argc, char **argv)
{
	int res = 0;
	struct libusb_device_handle *handle;
	
	//printf("UDEV: %d \n",USE_UDEV);
	
	res = libusb_init(&ctx);
	if (res != 0){
		fprintf(stderr,"Could not initialize libusb! \n");
		return -1;
	}
	
	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

	res = get_ni6218_fwloader_usb(&handle);
	
	if (res == 0){
		printf("Found NI USB FW, sending firmware \n");
		ni6218_send_firmware(handle);
		sleep(8);
		refresh_usb_system();
	}else{
		printf("Could not find NI USB FW device, maybe its already in IO state \n");
	}
	
	printf("Checking for NI USB 6218 \n");
	res = get_ni6218_io_usb(&handle);
	if (res < 0){
		fprintf(stderr,"Could not find NI USB 6128 \n");
		return ENODEV;
	}
	
	
	
	
	
	
	//usb_set_configuration(handle,dev->config->bConfigurationValue);
	//usb_control_msg(handle,USB_TYPE_VENDOR,160,0xe600,0,buf,1,400);
	//usb_control_msg(handle,USB_TYPE_VENDOR,160,0xe600,0,buf,0,400);
	
//	usb_alloc_urb(40,NULL);
	
	//printf("NI USB-6218 Opened successfully \n");

	//printf("NI USB-6218 demo - press CTRL-C to quit\n\n");


	//libusb_close(handle);
	libusb_exit(NULL);
	return 0;
}
