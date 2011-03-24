/// Accessor to the .gat map virtual files
// Note .gat files are mapped to .wlk files by data/resnametable.txt
// Note that there currently is a 1-1 correlation between them,
// but it is possible for a single .wlk to have multiple .gats reference it
#ifndef GRFIO_H
#define GRFIO_H

/// Load file into memory
# define grfio_read(resourcename) grfio_reads (resourcename, NULL)
/// Load file into memory and possibly record length
// For some reason, this allocates an extra 1024 bytes at the end
void *grfio_reads (const char *resourcename, size_t *size);
/// Get size of file
// This is only called once, and that is to check the existence of a file.
size_t grfio_size (const char *resourcename) __attribute__((deprecated));

#endif // GRFIO_H
