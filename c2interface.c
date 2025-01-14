/*
 * Copyright 2014 Dirk Eibach <eibach@gdsys.de>
 *           2022 Alex Kiselev <a.kiselev@volz-servos.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef DEF_NO_GPIO
  #include <pigpio.h>
#else
  #define gpioInitialise()        true
  #define gpioTerminate()
  #define gpioWrite(x, y)
  #define gpioSetMode(x, y)
  #define gpioSetPullUpDown(x, y)
  #define gpioRead(x)             true
  #define PI_ON                   true
  #define PI_OFF                  false
#endif

#include "defines.h"
#include "c2family.h"
#include "c2interface.h"
#include "c2tool.h"
#include "hexdump.h"
/*
 * C2 registers & commands defines
 */

/* C2 registers */
#define C2_DEVICEID		0x00
#define C2_REVID		  0x01
#define C2_FPCTL		  0x02

/* C2 interface commands */
#define C2_GET_VERSION	0x01
#define C2_DEVICE_ERASE	0x03
#define C2_BLOCK_READ	  0x06
#define C2_BLOCK_WRITE	0x07
#define C2_PAGE_ERASE	  0x08

#define C2_FPDAT_GET_VERSION	  0x01
#define C2_FPDAT_GET_DERIVATIVE	0x02
#define C2_FPDAT_DEVICE_ERASE	  0x03
#define C2_FPDAT_BLOCK_READ	    0x06
#define C2_FPDAT_BLOCK_WRITE	  0x07
#define C2_FPDAT_PAGE_ERASE	    0x08
#define C2_FPDAT_DIRECT_READ	  0x09
#define C2_FPDAT_DIRECT_WRITE	  0x0a
#define C2_FPDAT_INDIRECT_READ	0x0b
#define C2_FPDAT_INDIRECT_WRITE	0x0c

#define C2_FPDAT_RETURN_INVALID_COMMAND	0x00
#define C2_FPDAT_RETURN_COMMAND_FAILED	0x02
#define C2_FPDAT_RETURN_COMMAND_OK	    0x0D

#define C2_FPCTL_HALT		    0x01
#define C2_FPCTL_RESET		  0x02
#define C2_FPCTL_CORE_RESET	0x04

#define GPIO_C2D		23
#define GPIO_C2CK		24

static uint16_t c2_poll_out_timeout;

/*
 * state 0: drive low
 *       1: high-z
 */
static void c2d_set(struct c2interface *c2if, int state)
{
	if (state)
		gpioWrite(GPIO_C2D, PI_ON);
	else
		gpioWrite(GPIO_C2D, PI_OFF);
	usleep(1);
}

static int c2d_get(struct c2interface *c2if)
{
	//usleep(1);
	return (gpioRead(GPIO_C2D) == PI_ON);
}

static void c2ck_set(struct c2interface *c2if, int state)
{
	if (state)
		gpioWrite(GPIO_C2CK, PI_ON);
	else
		gpioWrite(GPIO_C2CK, PI_OFF);
	usleep(1);
}

static void c2ck_strobe(struct c2interface *c2if)
{
	gpioWrite(GPIO_C2CK, PI_OFF);
	gpioWrite(GPIO_C2CK, PI_ON);
	usleep(1);
}

/*
 * C2 primitives
 */

int c2_init(void)
{
	int res = gpioInitialise();
	if (res >= 0)
	{
		gpioSetMode(GPIO_C2CK, PI_OUTPUT);
		gpioWrite(GPIO_C2CK, PI_ON);
		gpioSetMode(GPIO_C2D, PI_INPUT);
		gpioSetPullUpDown(GPIO_C2D, PI_PUD_UP);
	}
	return res;
}

void c2_terminate(void)
{
	gpioTerminate();
}

void c2_reset(struct c2interface *c2if)
{
	gpioSetMode(GPIO_C2D, PI_INPUT);
	/* To reset the device we have to keep clock line low for at least
	 * 20us.
	 */
	c2ck_set(c2if, 0);
	usleep(25);
	c2ck_set(c2if, 1);
	usleep(1);
}

static void c2_write_ar(struct c2interface *c2if, unsigned char addr)
{
	int i;

	gpioSetMode(GPIO_C2D, PI_OUTPUT);
	usleep(1);
	/* START field */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);

	/* INS field (11b, LSB first) */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);

	/* ADDRESS field */
	for (i = 0; i < 8; i++) {
		c2d_set(c2if, addr & 0x01);
		c2ck_strobe(c2if);

		addr >>= 1;
	}

	/* STOP field */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);
	usleep(1);
	gpioSetMode(GPIO_C2D, PI_INPUT);
}

static int c2_read_ar(struct c2interface *c2if, unsigned char *addr)
{
	int i;

	gpioSetMode(GPIO_C2D, PI_OUTPUT);
	usleep(1);
	/* START field */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);

	/* INS field (10b, LSB first) */
	c2d_set(c2if, 0);
	c2ck_strobe(c2if);
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);

	/* ADDRESS field */
	c2d_set(c2if, 1);
	gpioSetMode(GPIO_C2D, PI_INPUT);
	*addr = 0;
	for (i = 0; i < 8; i++) {
		*addr >>= 1;	/* shift in 8-bit ADDRESS field LSB first */

		c2ck_strobe(c2if);
		if (c2d_get(c2if))
			*addr |= 0x80;
	}
	/* STOP field */
	c2ck_strobe(c2if);

	return 0;
}

static int c2_write_dr(struct c2interface *c2if, unsigned char data)
{
	int timeout, i;

	gpioSetMode(GPIO_C2D, PI_OUTPUT);
	usleep(1);
	/* START field */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);

	/* INS field (01b, LSB first) */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);
	c2d_set(c2if, 0);
	c2ck_strobe(c2if);

	/* LENGTH field (00b, LSB first -> 1 byte) */
	c2d_set(c2if, 0);
	c2ck_strobe(c2if);
	c2d_set(c2if, 0);
	c2ck_strobe(c2if);

	/* DATA field */
	for (i = 0; i < 8; i++) {
		c2d_set(c2if, data & 0x01);
		c2ck_strobe(c2if);

		data >>= 1;
	}

	/* WAIT field */
	c2d_set(c2if, 1);
	gpioSetMode(GPIO_C2D, PI_INPUT);
	timeout = 20;
	do {
		c2ck_strobe(c2if);
		if (c2d_get(c2if))
			break;

		usleep(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	/* STOP field */
	c2ck_strobe(c2if);

	return 0;
}

static int c2_read_dr(struct c2interface *c2if, unsigned char *data)
{
	int timeout, i;

	gpioSetMode(GPIO_C2D, PI_OUTPUT);
	usleep(1);
	/* START field */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);

	/* INS field (00b, LSB first) */
	c2d_set(c2if, 0);
	c2ck_strobe(c2if);
	c2d_set(c2if, 0);
	c2ck_strobe(c2if);

	/* LENGTH field (00b, LSB first -> 1 byte) */
	c2d_set(c2if, 0);
	c2ck_strobe(c2if);
	c2d_set(c2if, 0);
	c2ck_strobe(c2if);

	/* WAIT field */
	c2d_set(c2if, 1);
	gpioSetMode(GPIO_C2D, PI_INPUT);
	timeout = 50;
	do {
		c2ck_strobe(c2if);
		if (c2d_get(c2if))
			break;

		usleep(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	/* DATA field */
	*data = 0;
	for (i = 0; i < 8; i++) {
		*data >>= 1;	/* shift in 8-bit DATA field LSB first */

		c2ck_strobe(c2if);
		if (c2d_get(c2if))
			*data |= 0x80;
	}
	/* STOP field */
	c2ck_strobe(c2if);

	return 0;
}

static int c2_poll_in_busy(struct c2interface *c2if)
{
	unsigned char addr;
	int ret, timeout = 20;

	do {
		ret = (c2_read_ar(c2if, &addr));
		if (ret < 0)
			return -EIO;

		if (!(addr & 0x02))
			break;

		usleep(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	return 0;
}

void c2_set_poll_out_timeout(uint16_t timeout)
{
  c2_poll_out_timeout = timeout;
}

static int c2_poll_out_ready(struct c2interface *c2if)
{
	unsigned char addr;
	/* erase flash needs long time... */
	int ret, timeout = 100;

	do {
		ret = (c2_read_ar(c2if, &addr));
		if (ret < 0)
			return -EIO;

		if (addr & 0x01)
			break;

		usleep(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	return 0;
}

int c2_read_sfr(struct c2interface *c2if, unsigned char sfr)
{
	unsigned char data;

	c2_write_ar(c2if, sfr);

	if (c2_read_dr(c2if, &data) < 0)
		return -EIO;

	return data;
}

int c2_write_sfr(struct c2interface *c2if, unsigned char sfr, unsigned char data)
{
	c2_write_ar(c2if, sfr);

	if (c2_write_dr(c2if, data) < 0)
		return -EIO;

	return 0;
}

/*
 * Programming interface (PI)
 * Each command is executed using a sequence of reads and writes of the FPDAT register.
 */

static int c2_pi_write_command(struct c2interface *c2if, unsigned char command)
{
	if (c2_write_dr(c2if, command) < 0)
		return -EIO;

	if (c2_poll_in_busy(c2if) < 0)
		return -EIO;

	return 0;
}

static int c2_pi_get_data(struct c2interface *c2if, unsigned char *data)
{
	if (c2_poll_out_ready(c2if) < 0)
		return -EIO;

	if (c2_read_dr(c2if, data) < 0)
		return -EIO;

	return 0;
}

static int c2_pi_check_command(struct c2interface *c2if)
{
	unsigned char response;

	if (c2_pi_get_data(c2if, &response) < 0)
		return -EIO;

	if (response != C2_FPDAT_RETURN_COMMAND_OK)
		return -EIO;

	return 0;
}

static int c2_pi_command(struct c2interface *c2if, unsigned char command, int verify,
			 unsigned char *result)
{
	if (c2_pi_write_command(c2if, command) < 0)
		return -EIO;

	if (!verify)
		return 0;

	if (c2_pi_check_command(c2if) < 0)
		return -EIO;

	if (!result)
		return 0;

	if (c2_pi_get_data(c2if, result) < 0)
		return -EIO;

	return 0;
}

int c2_read_direct(struct c2tool_state *state, unsigned char reg)
{
	unsigned char data;
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DIRECT_READ, 1, NULL))
		return -EIO;

	if (c2_pi_write_command(c2if, reg))
		return -EIO;

	if (c2_pi_write_command(c2if, 0x01))
		return -EIO;
	if (c2_poll_out_ready(c2if) < 0)
		return -EIO;
	if (c2_read_dr(c2if, &data) < 0)
		return -EIO;

	return data;
}

int c2_write_direct(struct c2tool_state *state, unsigned char reg, unsigned char value)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DIRECT_WRITE, 1, NULL))
		return -EIO;

	if (c2_pi_write_command(c2if, reg))
		return -EIO;

	if (c2_pi_write_command(c2if, 0x01))
		return -EIO;

	if (c2_pi_write_command(c2if, value))
		return -EIO;

	return 0;
}

int c2_flash_read(struct c2tool_state *state, unsigned int addr, unsigned int length,
			 unsigned char *dest)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	while (length) {
		unsigned int blocksize;
		unsigned int k;

		if (c2_pi_command(c2if, C2_FPDAT_BLOCK_READ, 1, NULL) < 0)
			return -EIO;

		if (c2_pi_command(c2if, addr >> 8, 0, NULL) < 0)
			return -EIO;
		if (c2_pi_command(c2if, addr & 0xFF, 0, NULL) < 0)
			return -EIO;

		if (length > 255) {
			if (c2_pi_command(c2if, 0, 1, NULL) < 0)
				return -EIO;
			blocksize = 256;
		} else {
			if (c2_pi_command(c2if, length, 1, NULL) < 0)
				return -EIO;
			blocksize = length;
		}

		for (k = 0; k < blocksize; ++k) {
			unsigned char data;

			if (c2_pi_get_data(c2if, &data) < 0)
				return -EIO;
			if (dest)
				*dest++ = data;
		}

		length -= blocksize;
		addr += blocksize;
	}

	return 0;
}

int c2_flash_write(struct c2tool_state *state, unsigned int addr, unsigned int length,
			 unsigned char *src)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	while (length) {
		unsigned int blocksize;
		unsigned int k;

		if (c2_pi_command(c2if, C2_FPDAT_BLOCK_WRITE, 1, NULL) < 0)
			return -1;//-EIO;

		if (c2_pi_command(c2if, addr >> 8, 0, NULL) < 0)
			return -2;//-EIO;
		if (c2_pi_command(c2if, addr & 0xff, 0, NULL) < 0)
			return -3;//-EIO;

		if (length > 255) {
			if (c2_pi_command(c2if, 0, 1, NULL) < 0)
				return -4;//-EIO;
			blocksize = 256;
		} else {
			if (c2_pi_command(c2if, length, 1, NULL) < 0)
				return -6;//-EIO;
			blocksize = length;
		}

		for (k = 0; k < blocksize; ++k) {
			if (c2_pi_command(c2if, *src++, 0, NULL) < 0)
				return -7;//-EIO;
		}

		length -= blocksize;
		addr += blocksize;
	}

	return 0;
}

int c2_flash_erase(struct c2tool_state *state, unsigned char page)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_PAGE_ERASE, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, page, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0, 1, NULL) < 0)
		return -EIO;

  // for F58x, restore PSBANK
  if (c2_write_sfr(c2if, 0xF5, 0x11) < 0)
    return -EIO;

	return 0;
}

int c2_halt(struct c2interface *c2if)
{
	c2_reset(c2if);

	usleep(2);

	c2_write_ar(c2if, C2_FPCTL);

	if (c2_write_dr(c2if, C2_FPCTL_RESET) < 0)
		return -EIO;

	if (c2_write_dr(c2if, C2_FPCTL_CORE_RESET) < 0)
		return -EIO;

	if (c2_write_dr(c2if, C2_FPCTL_HALT) < 0)
		return -EIO;

	usleep(30000);

	return 0;
}

int c2_get_device_info(struct c2interface *c2if, struct c2_device_info *info)
{
	unsigned char data;
	int ret;

	/* Select DEVID register for C2 data register accesses */
	c2_write_ar(c2if, C2_DEVICEID);

	/* Read and return the device ID register */
	ret = c2_read_dr(c2if, &data);
	if (ret < 0)
		return -EIO;

	info->device_id = data;

	/* Select REVID register for C2 data register accesses */
	c2_write_ar(c2if, C2_REVID);

	/* Read and return the revision ID register */
	ret = c2_read_dr(c2if, &data);
	if (ret < 0)
		return -EIO;

	info->revision_id = data;

	return 0;
}

int c2_get_pi_info(struct c2tool_state *state, struct c2_pi_info *info)
{
	unsigned char data;
	int ret;
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	/* Select FPDAT register for C2 data register accesses */
	c2_write_ar(c2if, family->fpdat);

	ret = c2_pi_command(c2if, C2_FPDAT_GET_VERSION, 1, &data);
	if (ret < 0)
		return -EIO;

	info->version = data;

	ret = c2_pi_command(c2if, C2_FPDAT_GET_DERIVATIVE, 1, &data);
	if (ret < 0)
		return -EIO;

	info->derivative = data;

	return 0;
}

int c2_flash_erase_device(struct c2tool_state *state)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DEVICE_ERASE, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_write_command(c2if, 0xDE) < 0)
		return -EIO;

	if (c2_pi_write_command(c2if, 0xAD) < 0)
		return -EIO;

	if (c2_pi_write_command(c2if, 0xA5) < 0)
		return -EIO;

  if (c2_pi_check_command(c2if) < 0)
    return -EIO;

	return 0;
}

int flash_chunk(struct c2tool_state *state, unsigned int addr, unsigned int length,
		       unsigned char *src)
{
	//struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;
	unsigned int page_size = family->page_size;
	unsigned int page = addr / page_size;
	unsigned char buf[page_size];
	int must_read = (addr % page_size) || (length < page_size);
	unsigned int page_base = page * page_size;
	unsigned int chunk_start = addr - page_base;
	unsigned int chunk_len = (chunk_start + length > page_size) ?
				 (page_size  - chunk_start) : length;

	if (must_read) {
		if (c2_flash_read(state, page_base, page_size, buf))
			return -EIO;
	}

	memcpy(buf + addr - page_base, src, chunk_len);

	if (c2_flash_erase(state, page))
		return -EIO;

	if (c2_flash_write(state, page_base, page_size, buf))
		return -EIO;

	return chunk_len;
}
