#ifndef CRC32_H_
#define CRC32_H_

# include <stdio.h>
# include <stdlib.h>

// compute_CRC_buffer
//   * compute CRC32 of buffer
unsigned long compute_CRC_buffer(const void *buf, size_t bufLen);

#endif
