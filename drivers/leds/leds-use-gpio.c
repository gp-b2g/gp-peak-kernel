/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/slab.h>

#include <mach/pmic.h>
#include <linux/gpio.h>//clf
#define LED_MPP(x)		((x) & 0xFF)
#define LED_CURR(x)		((x) >> 16)

struct gpio_led_data {
	struct led_classdev cdev;
	const char *name;
	const char *default_trigger;
	unsigned 	gpio;
	unsigned	active_low : 1;
	unsigned	retain_state_suspended : 1;
	unsigned	default_state : 2;
};
static void gpio_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct gpio_led_data *led;
	//int ret;
       
	led = container_of(led_cdev, struct gpio_led_data, cdev);

	if (value < LED_OFF || value > led->cdev.max_brightness) {
		dev_err(led->cdev.dev, "Invalid brightness value");
		return;
	}

	dev_err(led->cdev.dev, "gpio_led_set");
      if(value)
		gpio_set_value(led->gpio,1);
	else
		gpio_set_value(led->gpio,0);


}

static int gpio_led_probe(struct platform_device *pdev)
{
	const struct gpio_led_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_led_data *led, *tmp_led;
	int i, rc;

	if (!pdata) {
		dev_err(&pdev->dev, "platform data not supplied\n");
		return -EINVAL;
	}

	led = kcalloc(pdata->num_leds, sizeof(*led), GFP_KERNEL);
	if (!led) {
		dev_err(&pdev->dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, led);

	for (i = 0; i < pdata->num_leds; i++) {
		tmp_led	= &led[i];
		tmp_led->cdev.name = pdata->leds[i].name;
		tmp_led->cdev.brightness_set = gpio_led_set;
		tmp_led->cdev.brightness = LED_OFF;
		tmp_led->cdev.max_brightness = LED_FULL;
		tmp_led->gpio = pdata->leds[i].gpio;

             rc = gpio_tlmm_config(GPIO_CFG(tmp_led->gpio, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), GPIO_CFG_ENABLE);
             gpio_direction_output(tmp_led->gpio, 0);

		rc = led_classdev_register(&pdev->dev, &tmp_led->cdev);
		if (rc) {
			dev_err(&pdev->dev, "failed to register led\n");
			goto unreg_led_cdev;
		}
	}

	return 0;

unreg_led_cdev:
	while (i)
		led_classdev_unregister(&led[--i].cdev);

	kfree(led);
	return rc;

}

static int __devexit gpio_led_remove(struct platform_device *pdev)
{
	const struct gpio_led_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_led_data *led = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < pdata->num_leds; i++)
		led_classdev_unregister(&led[i].cdev);

	kfree(led);

	return 0;
}

static struct platform_driver gpio_led_driver = {
	.probe		= gpio_led_probe,
	.remove		= __devexit_p(gpio_led_remove),
	.driver		= {
		.name	= "gpio-leds",
		.owner	= THIS_MODULE,
	},
};

static int __init gpio_led_init(void)
{
	return platform_driver_register(&gpio_led_driver);
}
module_init(gpio_led_init);

static void __exit gpio_led_exit(void)
{
	platform_driver_unregister(&gpio_led_driver);
}
module_exit(gpio_led_exit);

MODULE_DESCRIPTION("GPIO LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:gpio-leds");
