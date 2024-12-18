#ifndef TESTS_H
#define TESTS_H

#define FILENAMESIZE 32
#define BUFSIZE 33
#define VMEM (184 << 12)
#define PAGE_SIZE (VMEM + 4096)
#define FILEBUFSIZE 1024
#define SBUFSIZE 33

// test launcher
void launch_tests();

#endif /* TESTS_H */
