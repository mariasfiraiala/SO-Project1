Copyright 2022 Maria Sfiraiala (maria.sfiraiala@stud.acs.upb.ro)

# ELF Loader - Project1

## Description

The project aims to implement a simple pagefault handler that would intercept the `SIGSEGV` signal and treat it accordingly.
Ergo, we'll save the default handler (used in case of an illegal access or an address that doesn't belong to any segment of our executable) and we'll enable it when needed.
Otherwise, the newly created handler will map the page in which we received the fault, read from file and set permissions:

1. get the address that caused the page fault from `info->si_addr`

2. get the segment that contains the address that caused the fault

3. get the page that contains the faulty address

4. check whether that page was already mapped, if so, reinstate the default handler (we used the auxiliary `data` array to keep information about the pages)

**Note**: take a look at the flag used for marking the page as already mapped (`exec_parser.h`)

5. use `mmap` to reserve space

6. regarding reading from file, there are 3 cases:

    * the page is fully within the file size: we read the whole page

    * the page is fully out the file size: we set all bits to 0

    * the page is partially within the file size: we set the bits that are out with 0, we read the ones from within the file size

7. set page's permissions using the ones from the segment

**Note**: Take a look at the way we got the page size (`exec_parser.h`).

## Obervations Regarding the Project

It was definitely one of the most fun projects to work on, actually writing something that changes the flow of an otherwise unsurprising program, :sparkles: the pagefault handler :sparkles:, feels rewarding and surely made me check man pages more attentively.

I've realized that until starting this project I hadn't understood the process of reserving, allocating and mapping memory to the fullest, thus practical work brought quality of life improvements.

As a note to future self, never code while being tired again, messing up pointers for the `memset` function, contrary to the popular belif, isn't amusing and left me with quite a headache.
