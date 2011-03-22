/**
 * buffer.c
 * Author: Jan Viktorin
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include "pico.h"
#include "buffer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

struct buffer {
	char *begin;
	char *end;
	int fd;
};

bool buffer_isend(struct pico *p, char *offset)
{
	assert(offset >= p->buff->begin);
	return offset + 1 >= p->buff->end;
}

bool buffer_init(struct pico *p, char *srcfile)
{
	if(p->buff != NULL)
		return true;

	int fd = srcfile == NULL? 0 : open(srcfile, O_RDONLY);
	if(fd < 0)
		return error(p, "Can not open the source file");

	struct stat file_info;
	if(fstat(fd, &file_info)) {
		close(fd);
		return error(p, "Can not stat the source file");
	}
	
	size_t len = file_info.st_size;
	if(len == 0) {
		close(fd);
		return error(p, "The source file is empty");
	}

	void *source = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if(source == NULL || source == MAP_FAILED) {
		close(fd);
		return error(p, "Can not mmap the source file");
	}

	p->buff = (struct buffer *) malloc(sizeof(struct buffer));
	if(p->buff == NULL) {
		munmap(source, len);
		close(fd);
		return error(p, "Memory allocation error");
	}

	p->buff->begin = (char *) source;
	p->buff->end = p->buff->begin + len;
	p->buff->fd = fd;
	p->offset = p->buff->begin;
	return true;
}

void buffer_destroy(struct pico *p)
{
	if(p->buff == NULL)
		return;

	munmap((void *) p->buff->begin, p->buff->end - p->buff->begin);
	close(p->buff->fd);
	free(p->buff);
	p->buff = NULL;
}

