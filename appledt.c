/*
 * SPDX-License-Identifier: MIT
 * Copyright 2021 Alexander Graf <agraf@csgraf.de>
 *
 * A small tool to convert Apple's DT format into JSON
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct xnu_dt_prop {
        char name[32];
        uint32_t length;
        uint8_t data[0];
} xnu_dt_prop;

typedef struct xnu_dt_entry {
        uint32_t nr_props;
        uint32_t nr_children;
        /* xnu_dt_prop props[nr_props]; */
        /* xnu_dt_entry children[nr_children] */
} xnu_dt_entry;

static int is_ascii(xnu_dt_prop *prop)
{
	const uint8_t *data = prop->data;
	int len = prop->length;
	char lastchar = 0;
	int i;

	for (i = 0; i < len; i++) {
		if (data[i] & 0x80)
			return 0;
 		if (data[i] && data[i] < 0x20)
			return 0;
		if (!data[i] && !lastchar)
			return 0;
		lastchar = data[i];
	}

	return 1;
}

static int print_escaped(const uint8_t *str, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (str[i] == '\\')
			printf("\\\\");
		else if (!str[i])
			printf("<0>");
		else
			printf("%c", str[i]);
	}

	return 0;
}

static int print_hex(const uint8_t *data, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (i)
			printf(" ");
		printf("%02x", data[i]);
	}

	return 0;
}

static void *print_node(xnu_dt_entry *node)
{
	xnu_dt_prop *prop;
	xnu_dt_entry *child;
	void *tail = &node[1];
	int i;

	printf("{ ");
	prop = tail;
	for (i = 0; i < node->nr_props; i++) {
		prop->length &= 0xffffff;
		if (i)
			printf(", ");
		printf("\"%s\" : \"", prop->name);
		if (is_ascii(prop))
			print_escaped(prop->data, prop->length);
		else
			print_hex(prop->data, prop->length);
		printf("\"\n");
		tail += sizeof(*prop) + ((prop->length + 3) & ~0x3);
		prop = tail;
	}
	if (i)
		printf(", ");
	printf("\"children\" : [ ");
	child = tail;
	for (i = 0; i < node->nr_children; i++) {
		if (i)
			printf(", ");
		tail = print_node(child);
		child = tail;
	}
	printf("] } ");

	return tail;
}

int main(int argc, char **argv)
{
	char buf[1024 * 1024];
	int fd = 0;

	if (argc > 1)
		fd = open(argv[1], O_RDONLY);

	if (fd < 0)
		return 1;

	read(fd, buf, sizeof(buf));
	print_node((void*)buf);

	return 0;
}
