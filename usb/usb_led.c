#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>

#define USBLED_VENDOR_ID   0x04D8
#define USBLED_PRODUCT_ID  0x003F


/* List of devices that works this devecs*/

static const struct usb_device_id id_table[] = {
    {USB_DEVICE(USBLED_VENDOR_ID, USBLED_PRODUCT_ID)},
    {}
};

MODULE_DEVICE_TABLE(usb, id_table);

/* led device struct*/
struct usb_led {
    struct usb_device *udev;
    u8 led_number;
};

static ssize_t led_show(struct device *dev, struct device_attribute *attr, char *buf){
    struct usb_interface *intf = to_usb_interface(dev);
    struct usb_led *led = usb_get_intfdata(intf);

    return sprintf(buf, "%d\n", led->led_number);
}

static ssize_t led_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    struct usb_interface *intf = to_usb_interface(dev);
    struct usb_led *led = usb_get_intfdata(intf);

    u8 val;
    int error, retval;
    dev_info(&intf->dev, "led_store() functioin called");

    /* Transform char array to u8 value */
    error = kstrtou8(buf, 10, &val);

    if(error){
        return error;
    }

    led->led_number = val;

    if (val == 1 || val == 2 || val == 3){
        dev_info(&led->udev->dev, "led = %d\n", led->led_number);
    }
    else{
        dev_info(&led->udev->dev, "unknown led %d\n", led->led_number);
    }

    /* Toggle led */
    retval = usb_bulk_msg(led->udev, usb_sndctrlpipe(led->led_number, 1), &led->led_number, 1, NULLL, 0);
    if(retval){
        retval = -EFAULT;
        return retval;

    }
    return count;
}

static DEVICE_ATTR_RW(led);

static int led_probe(struct usb_interface *interface, struct usb_device_id *id){
    struct usb_device *udev = interface_to_usbdev(interface);
    struct usb_led *dev = NULL;

    int retval = -ENOMEM;

    dev = kzalloc(sizeof(struct usb_led), GFP_KERNEL);
    if(!dev){
        dev_err(&interface->dev, "out of memory");
        retval = -ENOMEM;
        goto error;
    }

    dev->udev = usb_get_dev(udev);
    usb_set_intfdata(interface, dev);

    retval = device_create_file(&inteface->dev, &dev_attr_led);
    if(retval){
        goto error_create_file;
    }
    return 0;

    error_create_file:
        usb_put_dev(udev);
        usb_set_intfdata(interface, NULL);
    error:
        kfree(dev);
        return retval;
}

static void led_disconnect(struct usb_interface *interface){
    struct usb_led *dev;
    dev = usb_get_intfdata(interface);

    device_remove_file(&interface->dev, &dev_attr_led);
    usb_get_intfdata(interface, NULL);
    usb_put_dev(dev->udev);
    kfree(dev);

    dev_info(&interface->dev, "USB LED now disconnect");
}

static struct usb_driver led_driver = {
    .name = "usbled",
    .probe = led_probe,
    .disconnect = led_disconnect,
    id_table = id_table,
};

module_usb_driver(led_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Santosh Mahto <eisantosh95@gmail.com>");
MODULE_DESCRIPTION("This is led module");
