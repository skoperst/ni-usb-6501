#include <stdio.h>
#include <usb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>


#define VENDOR_ID	0x3923
#define PRODUCT_ID	0x7272
#define PRODUCT_ID_FIRMWARELOAD	0x7269

#define PACKET_HEADER_LEN	4
#define DATA_HEADER_LEN		4

#define PROTOCOL_ERROR		(-EPROTO)
#define BUFFER_TOO_SMALL	(-ENOSPC)

#define EP_IN_ANALOG_PORT 0x88
#define EP_IN	0x81
#define EP_OUT	0x01
#define EP_OUT_FPGA 0x02


#define TIMEOUT 7000

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

void dump_buffer(size_t len, const void *buffer)
{
    size_t i;

    printf("LEN: %02zu: ", len);
    for(i = 0; i < len; i++)
        printf("%02x ", ((unsigned char *) buffer)[i]);
    printf("\n");
}

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
			res = libusb_open(dev,handle);
			if (res < 0){
				printf("Error opening NI 6218 IO device \n");
				libusb_free_device_list(list,1);
				return -1;
			}
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
			printf("Found NI 6218 FW Download device. Opening it \n");
			res = libusb_open(dev,handle);
			if (res < 0){
				printf("Error opening usb device! \n");
				libusb_free_device_list(list,1);
				return -1;
				
			}
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
	char *firmware2 = load_firmware("../firmware/NIUSBMXB621x0.bin");
	char *firmware3 = load_firmware("../firmware/NIUSBMXB621x1.bin");
	//dump_buffer(strlen(firmware1),firmware1);
	//3. Start Firmware Upload
	buf[0] = 1;
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0xe600,0,buf,1,1000);
	
	
	//4.Sending first block (512 bytes)
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0,0,firmware1,512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,160,0x200,0,(firmware1+512),512,1000);
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
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x4000,0,(firmware2 + 0x4000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x4200,0,(firmware2 + 0x4200),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x4400,0,(firmware2 + 0x4400),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x4600,0,(firmware2 + 0x4600),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x4800,0,(firmware2 + 0x4800),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x4A00,0,(firmware2 + 0x4A00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x4C00,0,(firmware2 + 0x4C00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x4E00,0,(firmware2 + 0x4E00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x5000,0,(firmware2 + 0x5000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x5200,0,(firmware2 + 0x5200),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x5400,0,(firmware2 + 0x5400),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x5600,0,(firmware2 + 0x5600),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x5800,0,(firmware2 + 0x5800),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x5A00,0,(firmware2 + 0x5A00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x5C00,0,(firmware2 + 0x5C00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x5E00,0,(firmware2 + 0x5E00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x6000,0,(firmware2 + 0x6000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,164,0x6200,0,(firmware2 + 0x6200),75,1000);
	
	//Verify
	libusb_control_transfer(handle,0xc0,164,0x4000,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x4200,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x4400,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x4600,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x4800,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x4A00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x4C00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x4E00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x5000,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x5200,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x5400,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x5600,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x5800,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x5A00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x5C00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x5E00,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x6000,0,buf,512,1000);
	libusb_control_transfer(handle,0xc0,164,0x6200,0,buf,75,1000);
	
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4000,0,(firmware3 + 0x4000),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4200,0,(firmware3 + 0x4200),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4400,0,(firmware3 + 0x4400),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4600,0,(firmware3 + 0x4600),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4800,0,(firmware3 + 0x4800),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4A00,0,(firmware3 + 0x4A00),512,1000);
	libusb_control_transfer(handle,USB_TYPE_VENDOR,165,0x4C00,0,(firmware3 + 0x4C00),43,1000);
	
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
	
	printf("YESS!!! closing handle: %p\n",(void *)handle);
	libusb_close(handle);
	printf("Handl closed \n");
}

int ni6218_send_FPGA0_Program(libusb_device_handle *handle)
{
	int res = 0;
	int actual_length;
	char *fpga_prog = load_firmware("../firmware/NIUSBMXB621xFPGA0.bin");
	printf("Sending FPGA0 Program \n");
	
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x0000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x4000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x8000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0xC000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x10000,0x3256,&actual_length,TIMEOUT);
	
	
}

int ni6218_send_FPGA1_Program(libusb_device_handle *handle)
{
	int res = 0;
	int actual_length;
	char *fpga_prog = load_firmware("../firmware/NIUSBMXB621xFPGA1.bin");
	printf("Sending FPGA1 Program \n");
	
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x0000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x4000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x8000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0xC000,0x4000,&actual_length,TIMEOUT);
	
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x10000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x14000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x18000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x1C000,0x4000,&actual_length,TIMEOUT);
	
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x20000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x24000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x28000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x2C000,0x4000,&actual_length,TIMEOUT);
	
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x30000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x34000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x38000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x3C000,0x4000,&actual_length,TIMEOUT);
	
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x40000,0x4000,&actual_length,TIMEOUT);
	libusb_bulk_transfer(handle,EP_OUT_FPGA,fpga_prog + 0x44000,5248,&actual_length,TIMEOUT);

	
	
}

int niusb6218_send_request(struct libusb_device_handle *usb_handle, unsigned char cmd, size_t request_len, const void *request, size_t *result_len, void *result)
{
    unsigned char buffer[8192];
    int status, actualLength;

    if(request_len > 255-(PACKET_HEADER_LEN+DATA_HEADER_LEN))
    {
        printf("request too long (%zu > %d bytes)\n", request_len, 255-(PACKET_HEADER_LEN+DATA_HEADER_LEN));
        return -EINVAL;
    }

    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = request_len+PACKET_HEADER_LEN+DATA_HEADER_LEN;
    buffer[4] = 0x00;
    buffer[5] = request_len+DATA_HEADER_LEN;
    buffer[6] = 0x01;
    buffer[7] = cmd;

    memcpy(&buffer[PACKET_HEADER_LEN+DATA_HEADER_LEN], request, request_len);
    

    /* send command */
    printf("sending command handle:%p buf:%p >>\n",(void *)usb_handle,(void *)buffer);
	dump_buffer(request_len+PACKET_HEADER_LEN+DATA_HEADER_LEN, buffer);
	
	
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
    printf("read result<<\n");
    status = libusb_bulk_transfer(
                usb_handle,
                EP_IN,
                buffer,
                8192,
                &actualLength,
                TIMEOUT
                );
                
    if(status < 0)
        return status;
   // dump_buffer(actualLength, buffer);
    if(actualLength < PACKET_HEADER_LEN)
        return PROTOCOL_ERROR;
    if(actualLength-PACKET_HEADER_LEN > *result_len)
        return BUFFER_TOO_SMALL;
   // *result_len = actualLength-PACKET_HEADER_LEN;
   // memcpy(result, &buffer[PACKET_HEADER_LEN], actualLength-PACKET_HEADER_LEN);
    return 0;
}

void niusb6218_read_analog(struct libusb_device_handle *handle,int size,int timeout)
{
	int actual_length;
	int status;
	char buffer[8192];
	
    printf("Listeneing to analog value read \n");
    status = libusb_bulk_transfer(
                handle,
                EP_IN_ANALOG_PORT,
                buffer,
                size,
                &actual_length,
                timeout
                );

	printf("Done listeneing to analog value \n");
}

void* niusb6218_listen_analog_port_proc(void* args)
{
	struct libusb_device_handle *handle = args;
	niusb6218_read_analog(handle,8192,TIMEOUT);
	
	printf("Finishing thread.. \n");
}

int main(int argc, char **argv)
{
	int res = 0;
	pthread_t ainterface_thread;
	struct libusb_device_handle *handle;
	char req_buf[512];
	char res_buf[512];
	size_t *res_buf_sz = malloc(sizeof(size_t));
	
	*res_buf_sz = 512;
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
		sleep(4);
		refresh_usb_system();
		sleep(1);
		
	}else{
		printf("Could not find NI USB FW device, maybe its already in IO state \n");
	}
	
	printf("Checking for NI USB 6218 \n");
	res = get_ni6218_io_usb(&handle);
	if (res < 0){
		fprintf(stderr,"Could not find NI USB 6128 \n");
		return ENODEV;
	}
	
	//get descriptor
	libusb_control_transfer(handle,LIBUSB_ENDPOINT_IN | (0x80 & 0xff), LIBUSB_REQUEST_GET_DESCRIPTOR,(0x03 << 8) | 0x3, 0x0409, req_buf, 255, TIMEOUT);
	libusb_control_transfer(handle,0x81,USB_REQ_GET_INTERFACE,0x0,0,req_buf,1,1000);
	//get descriptor - Serial
	libusb_control_transfer(handle,LIBUSB_ENDPOINT_IN | (0x80 & 0xff), LIBUSB_REQUEST_GET_DESCRIPTOR,(0x03 << 8) | 0x0, 0x00, req_buf, 4, TIMEOUT);
	
	libusb_control_transfer(handle,LIBUSB_ENDPOINT_IN | (0x80 & 0xff), LIBUSB_REQUEST_GET_DESCRIPTOR,(0x03 << 8) | 0x3, 0x0409, req_buf, 255, TIMEOUT);
	
	req_buf[0] = req_buf[1] = req_buf[2] = req_buf[3] = 0x00;
	res = niusb6218_send_request(handle,0x00,4,req_buf,res_buf_sz,res_buf);
	
	res_buf_sz = malloc(sizeof(size_t));
	req_buf[0] = 0x03;
	req_buf[1] = 0xFE;
	req_buf[2] = req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x08,4,req_buf,res_buf_sz,res_buf);
	
	ni6218_send_FPGA0_Program(handle);
	
	req_buf[0] = 0x03; req_buf[1] = 0xFE; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x09,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0xFE; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x09,4,req_buf,res_buf_sz,res_buf);
	
	usleep(100000);
	
	//BOOTUP the FPGA prgram
	req_buf[0] = 0x03; req_buf[1] = 0x01; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x04; req_buf[6] = 0x54; req_buf[7] = 0x80;
	req_buf[8] = 0x02; req_buf[9] = 0x00; req_buf[10] = 0x00; req_buf[11] = 0x00;
	
	niusb6218_send_request(handle,0x08,12,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0xff; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x08,4,req_buf,res_buf_sz,res_buf);
	
	
	ni6218_send_FPGA1_Program(handle);
	
	req_buf[0] = 0x03; req_buf[1] = 0xFF; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x09,4,req_buf,res_buf_sz,res_buf);

	req_buf[0] = 0x04; req_buf[1] = 0x01; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x09,4,req_buf,res_buf_sz,res_buf);

	req_buf[0] = 0x04; req_buf[1] = 0x00; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x09,4,req_buf,res_buf_sz,res_buf);


	printf("Done initializing NI USB6218 firmware + FPGA1/FPGA2 \n");
	
	req_buf[0] = 0x03; req_buf[1] = 0x13; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0D,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x13; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0E,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x10; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0A,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x11; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0B,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x15; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0D,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x11; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0C,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x11; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0C,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x10; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0B,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x13; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0F,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x15; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x09,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x05; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x00; req_buf[6] = 0x00; req_buf[7] = 0x18;
	req_buf[8] = 0x00; req_buf[9] = 0x00; req_buf[10] = 0x00; req_buf[11] = 0x02;
	niusb6218_send_request(handle,0x08,12,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x05; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x00; req_buf[6] = 0x01; req_buf[7] = 0xa0;
	req_buf[8] = 0x00; req_buf[9] = 0x00; req_buf[10] = 0x00; req_buf[11] = 0x11;
	niusb6218_send_request(handle,0x08,12,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x05; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x00; req_buf[6] = 0x01; req_buf[7] = 0xe4;
	req_buf[8] = 0x00; req_buf[9] = 0x00; req_buf[10] = 0x00; req_buf[11] = 0x8;
	niusb6218_send_request(handle,0x08,12,req_buf,res_buf_sz,res_buf);	
	
	req_buf[0] = 0x03; req_buf[1] = 0x02; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x32; req_buf[6] = 0x00; req_buf[7] = 0x00;
	niusb6218_send_request(handle,0x0A,8,req_buf,res_buf_sz,res_buf);	
		
	req_buf[0] = 0x03; req_buf[1] = 0x16; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x03; req_buf[5] = 0x01; req_buf[6] = 0x03; req_buf[7] = 0x01;
	niusb6218_send_request(handle,0x08,8,req_buf,res_buf_sz,res_buf);	
		
	req_buf[0] = 0x03; req_buf[1] = 0x15; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x00; req_buf[6] = 0x00; req_buf[7] = 0x10;
	req_buf[8] = 0x80; req_buf[9] = 0x00; req_buf[10] = 0x00; req_buf[11] = 0x00;
	niusb6218_send_request(handle,0x08,12,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x12; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0B,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x12; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x04; req_buf[6] = 0x40; req_buf[7] = 0x55;
	niusb6218_send_request(handle,0x0A,8,req_buf,res_buf_sz,res_buf);	
	
	req_buf[0] = 0x03; req_buf[1] = 0x12; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0C,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x12; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0C,4,req_buf,res_buf_sz,res_buf);
	
	//The 100-mark
	req_buf[0] = 0x03; req_buf[1] = 0x13; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x00; req_buf[6] = 0x00; req_buf[7] = 0x03;
	req_buf[8] = 0x00; req_buf[9] = 0x00; req_buf[10] = 0x01; req_buf[11] = 0x18;
	req_buf[12] = 0x00; req_buf[13] = 0x00; req_buf[14] = 0x00; req_buf[15] = 0x02;
	req_buf[16] = 0x01; req_buf[17] = 0x31; req_buf[18] = 0x2D; req_buf[19] = 0x00;
	req_buf[20] = 0xFF; req_buf[21] = 0xFF; req_buf[22] = 0xFF; req_buf[23] = 0xFF;
	req_buf[24] = 0xFF; req_buf[25] = 0xFF; req_buf[26] = 0xFF; req_buf[27] = 0xFF;		
	niusb6218_send_request(handle,0x10,28,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x13; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x01; req_buf[6] = 0x00; req_buf[7] = 0x00;
	req_buf[8] = 0x00; req_buf[9] = 0x01; req_buf[10] = 0x00; req_buf[11] = 0x00; 
	req_buf[12] = 0x01; req_buf[13] = 0x01; req_buf[14] = 0x13; req_buf[15] = 0x00; 
	req_buf[16] = 0x00; req_buf[17] = 0x01; req_buf[18] = 0x1C; req_buf[19] = 0x00;
	req_buf[20] = 0x00; req_buf[21] = 0x00; req_buf[22] = 0x01; req_buf[23] = 0x00;
	req_buf[24] = 0x00; req_buf[25] = 0x00; req_buf[26] = 0x00; req_buf[27] = 0x01;		
	req_buf[28] = 0x01; req_buf[29] = 0x00; req_buf[30] = 0x00; req_buf[31] = 0x00;
	
	req_buf[32] = 0x01; req_buf[33] = 0x01; req_buf[34] = 0x00; req_buf[35] = 0x00;
	req_buf[36] = 0x01; req_buf[37] = 0x00; req_buf[38] = 0x00; req_buf[39] = 0x00;
	niusb6218_send_request(handle,0x11,40,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x10; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x08,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x11; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x00; req_buf[6] = 0x00; req_buf[7] = 0x02;
	niusb6218_send_request(handle,0x09,8,req_buf,res_buf_sz,res_buf);	
	
	niusb6218_read_analog(handle,512,100);
	
	
	//BULK IN FROM EP-8
	//GET STATUS.. 
	
	printf("Starting IO Loop.. \n");
	req_buf[0] = 0x03; req_buf[1] = 0x11; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0D,4,req_buf,res_buf_sz,res_buf);
	
	//Now we should start listening to EP-88
	pthread_create(&ainterface_thread, NULL, niusb6218_listen_analog_port_proc, handle); // error checking??
	usleep(2000);
	
	req_buf[0] = 0x03; req_buf[1] = 0x13; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0A,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x15; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0C,4,req_buf,res_buf_sz,res_buf);

	req_buf[0] = 0x03; req_buf[1] = 0x11; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0A,4,req_buf,res_buf_sz,res_buf);

	req_buf[0] = 0x03; req_buf[1] = 0x10; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x09,4,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x13; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x00; req_buf[5] = 0x00; req_buf[6] = 0x00; req_buf[7] = 0x00;
	niusb6218_send_request(handle,0x0B,8,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x01; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x7F; req_buf[5] = 0xFF; req_buf[6] = 0xFF; req_buf[7] = 0xFF;
	req_buf[8] = 0x08; req_buf[9] = 0x00; req_buf[10] = 0x00; req_buf[11] = 0x00;
	niusb6218_send_request(handle,0x08,12,req_buf,res_buf_sz,res_buf);
	
	req_buf[0] = 0x03; req_buf[1] = 0x13; req_buf[2] = 0x00; req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0C,4,req_buf,res_buf_sz,res_buf);
	
	printf("Starting listen-value-read loop .. \n");
	req_buf[0] = 0x03; req_buf[1] = 0x01; req_buf[2] = 0x00; req_buf[3] = 0x00;
	req_buf[4] = 0x7F; req_buf[5] = 0xFF; req_buf[6] = 0xFF; req_buf[7] = 0xFD;
	req_buf[8] = 0x80; req_buf[9] = 0x00; req_buf[10] = 0x00; req_buf[11] = 0x00;
	niusb6218_send_request(handle,0x09,12,req_buf,res_buf_sz,res_buf);
	
	//EP-8 fires again
	
	sleep(10);
	
	/*
	
	
		//2. GET INTERFACE
	req_buf[0] = 0x00;
	libusb_control_transfer(handle,0x81,USB_REQ_GET_INTERFACE,0x0,0,req_buf,1,1000);
	
	res_buf_sz = malloc(sizeof(size_t));
	req_buf[0] = 0x03;
	req_buf[1] = 0x13;
	req_buf[2] = 0x00;
	req_buf[3] = 0x00;
	niusb6218_send_request(handle,0x0D,4,req_buf,&res,res_buf);
	
	
	*/
	
	
	
	//usb_set_configuration(handle,dev->config->bConfigurationValue);
	//usb_control_msg(handle,USB_TYPE_VENDOR,160,0xe600,0,buf,1,400);
	//usb_control_msg(handle,USB_TYPE_VENDOR,160,0xe600,0,buf,0,400);
	
//	usb_alloc_urb(40,NULL);
	
	//printf("NI USB-6218 Opened successfully \n");

	//printf("NI USB-6218 demo - press CTRL-C to quit\n\n");


	//libusb_close(handle);
	libusb_close(handle);
	libusb_exit(NULL);
	free(res_buf_sz);
	return 0;
}
