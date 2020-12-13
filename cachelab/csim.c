#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"

const unsigned int address_space_size = 4;
// https://stackoverflow.com/questions/9642732/parsing-command-line-arguments-in-c

typedef struct {
  bool verbose;
  int set_index_bits;
  int lines_per_set;
  int block_bits;
  char *tracefile;

} Arg;

void ParseArgs(int argc, char **argv, Arg *arg) {
  int opt;
  while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch (opt) {
      case 'h':
        printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
        break;
      case 'v':
        arg->verbose = true;
        break;
      case 's':
        arg->set_index_bits = atoi(optarg);
        break;
      case 'E':
        arg->lines_per_set = atoi(optarg);
        break;
      case 'b':
        arg->block_bits = atoi(optarg);
        break;
      case 't':
        arg->tracefile = optarg;
        break;
      default:
        abort();
    }
  }
}

typedef unsigned long long ULL;
typedef struct {
  ULL tag_bits;
  ULL index_bits;
  ULL offset_bits;
  ULL block_number;

} AddressBits;
// 64 bit address,m=64
// tag bit t = m-(b+s)
// parse 64 address bits into tag bits,index bits and offset bits.ƒ
void ParseAddress(const Arg *arg, const ULL raw_addr, AddressBits *addr) {
  const unsigned int index_and_block_bit_length =
      arg->block_bits + arg->set_index_bits;
  ULL block_bit_mask = (1ULL << arg->block_bits) - 1;
  ULL index_bit_mask = ((1ULL << arg->set_index_bits) - 1) << arg->block_bits;
  ULL tag_bit_mask =
      ((1ULL << (address_space_size - index_and_block_bit_length)) - 1)
      << index_and_block_bit_length;
  // printf("index_and_block_bit_length:%d\n",index_and_block_bit_length);
  // printf("mask:%lld %lld %lld\n", block_bit_mask, index_bit_mask,
  // tag_bit_mask);
  addr->tag_bits = (raw_addr & tag_bit_mask) > index_and_block_bit_length;
  addr->index_bits = (raw_addr & index_bit_mask) >> arg->block_bits;
  addr->offset_bits = raw_addr & block_bit_mask;
  addr->block_number = raw_addr / (1ULL << arg->block_bits);
//   printf("%lld:%lld %lld %lld %lld \n", raw_addr, addr->tag_bits,
        //  addr->index_bits, addr->offset_bits, addr->block_number);
}
void TestParseAddress(const Arg *arg) {
  for (int addr = 0; addr < (1ULL << address_space_size); addr++) {
    AddressBits tmp_addr;
    ParseAddress(arg, addr, &tmp_addr);
  }
}
typedef struct {
  enum operation { instruction_load, data_load, data_store, data_modify } op;
  ULL address;
  int size;
} Trace;
#define MAX_NUM_OF_TRACE 300000
Trace trace[MAX_NUM_OF_TRACE];
int lines_in_trace_file = 0;
void ParseTraceFile(const char *file) {
  FILE *fptr = fopen(file, "r");
  if (fptr == NULL) {
    printf("Error!");
    exit(1);
  }

  char op[5];
  ULL addr;
  int cnt = 0, siz;
  while (fscanf(fptr, "%s %lld,%d", op, &addr, &siz) != EOF) {
    // printf("op:%s addr:%lld size:%d\n", op, addr, siz);
    trace[cnt].address = addr;
    trace[cnt].size = siz;
    if (op[0] == 'L') {
      trace[cnt].op = data_load;
    } else if (op[0] == 'I') {
      trace[cnt].op = instruction_load;
    } else if (op[0] == 'S') {
      trace[cnt].op = data_store;
    } else if (op[0] == 'M') {
      trace[cnt].op = data_modify;
    }
    cnt++;
  }
  lines_in_trace_file = cnt;

  fclose(fptr);
}

void TestParseTraceFile() {
  char *file = "direct-mapped-4-bit.trace";
  ParseTraceFile(file);
}

// 不需要真的做cache,只需要模拟hit,miss,evition 的次数即可

typedef struct {
  unsigned int valid;
  unsigned int tag;
} Set;

Set *set;

void printSummary(int hits,       /* number of  hits */
                  int misses,     /* number of misses */
                  int evictions); /* number of evictions */
struct cache_summary_t {
  int hits;
  int misses;
  int evitions;
} cache_summary;

void SimulateCache(const Arg *arg) {
  memset(&cache_summary, 0, sizeof(cache_summary));
//   instruction_load, data_load, data_store, data_modify
  char operation_table[5]={'I','L','S','M'};
  for (int i = 0; i < lines_in_trace_file; i++) {
    AddressBits addr;
    ParseAddress(arg, trace[i].address, &addr);
    Set *cur_set = &set[addr.index_bits];
    if (cur_set->valid == 0) {
      cache_summary.misses++;
      cur_set->valid = 1;
      cur_set->tag = addr.tag_bits;
      if (arg->verbose) {
        printf("%c %lld,%d miss\n", operation_table[trace[i].op], trace[i].address, trace[i].size);
      }
    } else if (cur_set->tag == addr.tag_bits) {
      cache_summary.hits++;
      if (arg->verbose) {
        printf("%c %lld,%d hit\n", operation_table[trace[i].op], trace[i].address, trace[i].size);
      }
    } else {
      cache_summary.misses++;
      cache_summary.evitions++;
      cur_set->tag = addr.tag_bits;
      if (arg->verbose) {
        printf("%c %lld,%d miss eviction\n", operation_table[trace[i].op], trace[i].address, trace[i].size);
      }
    }
  }
}
int main(int argc, char **argv) {
//   printf("argc:%d\n", argc);
  Arg arg;
  ParseArgs(argc, argv, &arg);
//   printf("%d %d %d\n", arg.block_bits, arg.set_index_bits, arg.lines_per_set);
  //   TestParseAddress(&arg);
  //   TestParseTraceFile();
  ParseTraceFile(arg.tracefile);
  set = (Set *)malloc((1 << arg.block_bits) * sizeof(Set));
  memset(set, 0, (1 << arg.block_bits) * sizeof(Set));
  SimulateCache(&arg);
  printSummary(cache_summary.hits, cache_summary.misses,
               cache_summary.evitions);

  //   for (int i = 0; i < (1 << arg.block_bits); i++) {
  //     printf("valid:%d tag:%d\n", set[i].valid, set[i].tag);
  //   }
  // printSummary(0, 0, 0);
  // parse arguments from command line
  if (set != NULL) {
    free(set);
  }

  return 0;
}
