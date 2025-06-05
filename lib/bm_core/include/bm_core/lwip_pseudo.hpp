#pragma once

#include <cstdint>
#include <netinet/ip6.h>
#include <netinet/udp.h>

#include <spdlog/spdlog.h>

typedef  uintptr_t mem_ptr_t;
typedef  uint8_t u8_t;
typedef  uint16_t u16_t;
typedef  uint32_t u32_t;

#ifndef FOLD_U32T
#define FOLD_U32T(u)          ((u32_t)(((u) >> 16) + ((u) & 0x0000ffffUL)))
#endif

/** Swap the bytes in an u16_t: much like lwip_htons() for little-endian */
#ifndef SWAP_BYTES_IN_WORD
#define SWAP_BYTES_IN_WORD(w) (((w) & 0xff) << 8) | (((w) & 0xff00) >> 8)
#endif /* SWAP_BYTES_IN_WORD */

struct ip6_addr_t {
  u32_t addr[4];
};

/*
 * Curt McDowell
 * Broadcom Corp.
 * csm@broadcom.com
 *
 * IP checksum two bytes at a time with support for
 * unaligned buffer.
 * Works for len up to and including 0x20000.
 * by Curt McDowell, Broadcom Corp. 12/08/2005
 *
 * @param dataptr points to start of data to be summed at any boundary
 * @param len length of data to be summed
 * @return host order (!) lwip checksum (non-inverted Internet sum)
 */
static u16_t
lwip_standard_chksum(const void *dataptr, int len)
{
  const u8_t *pb = (const u8_t *)dataptr;
  const u16_t *ps;
  u16_t t = 0;
  u32_t sum = 0;
  int odd = ((mem_ptr_t)pb & 1);

  /* Get aligned to u16_t */
  if (odd && len > 0) {
    ((u8_t *)&t)[1] = *pb++;
    len--;
  }

  /* Add the bulk of the data */
  ps = (const u16_t *)(const void *)pb;
  while (len > 1) {
    sum += *ps++;
    len -= 2;
  }

  /* Consume left-over byte, if any */
  if (len > 0) {
    ((u8_t *)&t)[0] = *(const u8_t *)ps;
  }

  /* Add end bytes */
  sum += t;

  /* Fold 32-bit sum to 16 bits
     calling this twice is probably faster than if statements... */
  sum = FOLD_U32T(sum);
  sum = FOLD_U32T(sum);

  /* Swap if alignment was odd */
  if (odd) {
    sum = SWAP_BYTES_IN_WORD(sum);
  }

  return (u16_t)sum;
}

static u16_t
inet_cksum_pseudo_base(uint8_t* data, u8_t proto, u16_t proto_len, u32_t acc)
{
    int swapped = 0;

    acc += lwip_standard_chksum(data, proto_len);

    acc = FOLD_U32T(acc);
    if (proto_len % 2 != 0) {
        swapped = !swapped;
        acc = SWAP_BYTES_IN_WORD(acc);
    }

    if (swapped) {
        acc = SWAP_BYTES_IN_WORD(acc);
    }

    acc += (u32_t)htons((u16_t)proto);
    acc += (u32_t)htons(proto_len);

    /* Fold 32-bit sum to 16 bits
    calling this twice is probably faster than if statements... */
    acc = FOLD_U32T(acc);
    acc = FOLD_U32T(acc);
    return (u16_t)~(acc & 0xffffUL);
}

static u16_t
ip6_chksum_pseudo(uint8_t* data, u8_t proto, u16_t proto_len,
                  const ip6_addr_t *src, const ip6_addr_t *dest)
{
  u32_t acc = 0;
  u32_t addr;
  u8_t addr_part;

  for (addr_part = 0; addr_part < 4; addr_part++) {
    addr = src->addr[addr_part];
    acc = (u32_t)(acc + (addr & 0xffffUL));
    acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
    addr = dest->addr[addr_part];
    acc = (u32_t)(acc + (addr & 0xffffUL));
    acc = (u32_t)(acc + ((addr >> 16) & 0xffffUL));
  }
  /* fold down to 16 bits */
  acc = FOLD_U32T(acc);
  acc = FOLD_U32T(acc);
  
  return inet_cksum_pseudo_base(data, proto, proto_len, acc);
}