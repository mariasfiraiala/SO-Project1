/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "exec_parser.h"

static so_exec_t *exec;
struct sigaction default_handler;
static uint32_t fd;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	// get the address that causes the fault
	uintptr_t addr = (uint32_t)info->si_addr;

	// get the segment for which we have a fault
	uint32_t i = 0;
	so_seg_t *seg = exec->segments;

	while (i < exec->segments_no &&
				(addr < seg->vaddr || addr >= seg->vaddr + seg->mem_size)) {
		++i;
		++seg;
	}

	// if the address is within a segment we investigate it further
	if (i < exec->segments_no) {
		// get page for the faulty address
		uint32_t page = (addr - seg->vaddr) / pagesize;

		// if there isn't any page mapped for the segment
		// we'll map some in our opaque (working) array
		if (!seg->data) {
			uint32_t no_pages = seg->mem_size / pagesize;

			seg->data = calloc(no_pages, sizeof(*seg->data));
			DIE(!(seg->data), "calloc() failed");
		}

		// when we're trying to map a page that already exists, that's a case
		// for SEGFAULT => reinstate the default handler
		if (*((char *)(seg->data + page)) == ISMAPPED)
			default_handler.sa_sigaction(signum, info, context);

		// if we reached this point, it means we now have to map virtual to
		// physical space
		char *mapped = mmap((void *)(seg->vaddr + page * pagesize),
		pagesize, PROT_WRITE, MAP_ANONYMOUS | MAP_FIXED | MAP_SHARED,
		-1, 0);

		DIE(mapped == MAP_FAILED, "mmap() failed");

		// mark the page as mapped
		*(char *)(seg->data + page) = ISMAPPED;

		// if the page is fully within the file size, copy data into file
		if ((page + 1) * pagesize <= seg->file_size) {
			off_t rc_lseek = lseek(fd, seg->offset + page * pagesize, SEEK_SET);

			DIE(rc_lseek == -1, "lseek() failed");

			ssize_t rc_read = read(fd, mapped, pagesize);

			DIE(rc_read == -1, "read() failed");
		} else if (page * pagesize >= seg->file_size) {
			// if the page is out of the file size completely, zero the page
			memset(mapped, 0, pagesize);
		} else {
			// if the page is partially within the file, copy the part of it that is
			// the rest should be zeroed
			off_t rc_lseek = lseek(fd, seg->offset + page * pagesize, SEEK_SET);

			DIE(rc_lseek == -1, "lseek() failed");

			memset((void *)seg->vaddr + seg->file_size, 0,
				  (page + 1) * pagesize - seg->file_size);

			ssize_t rc_read = read(fd, mapped, seg->file_size - page * pagesize);

			DIE(rc_read == -1, "read() failed");
		}

		int rc_mprotect = mprotect(mapped, pagesize, seg->perm);

		DIE(rc_mprotect == -1, "mprotect() failed");

	} else {
		// if the address is not in any of our segments, we use the default handler
		default_handler.sa_sigaction(signum, info, context);
	}
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;

	// save the previous handler
	rc = sigaction(SIGSEGV, &sa, &default_handler);
	DIE(rc < 0, "sigaction() failed");
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	fd = open(path, O_RDONLY);
	so_start_exec(exec, argv);

	return -1;
}
