/* SPDX-License-Identifier: MIT
 *
 * Self-test fixture: persistence across two boots.
 *
 * The self-test boots one modified image copy twice. The controller gives each
 * boot an explicit index. The first boot checks the pristine content, changes
 * hello.txt, and writes it back. The second boot only checks the saved change,
 * so the fixture cannot pass by repeating the first boot. Both boots print two
 * assertions.
 */
#include "assert.h"
#include "kernel.h"

struct file *fs_lookup(const char *filename);
void fs_flush(void);

#define PRISTINE "Hello from the disk image.\n"
#define MODIFIED "Modified by the persistence check.\n"

void ktest_main(void) {
    ktest_begin("good_persistence");

    struct file *f = fs_lookup("hello.txt");
    KTEST_ASSERT(f != NULL);

    if (f == NULL) {
        KTEST_ASSERT(false);
    } else if (KTEST_BOOT == 1) {
        /* The first boot. The image is pristine, so change the file and write
         * it back to the disk. */
        KTEST_ASSERT(strcmp(f->data, PRISTINE) == 0);
        size_t n = sizeof(MODIFIED) - 1;
        memcpy(f->data, MODIFIED, n);
        f->size = n;
        fs_flush();
    } else {
        /* The second boot. Only the saved change gives this content. */
        KTEST_ASSERT(strcmp(f->data, MODIFIED) == 0);
    }

    ktest_end("good_persistence");
}
