 /* SPDX-License-Identifier: BSD-3-Clause */
 /* Copyright 2020-2021, Intel Corporation */

 //@file pmemstream.h

 #ifndef PMEMSTREAM_H
 #define PMEMSTREAM_H


 struct pmemstream{
     char *buf;
     size_t size;
 };

 int pmemstream_from_map(&stream, size_t size, map);

 struct pmemstream_tx *tx;
 pmemstream_tx_new(&tx, stream);

 struct pmemstream_region new_region;
 pmemstream_tx_region_allocate(tx, stream, 4096, &new_region);

 struct pmemstream_region_context *rcontext;
 pmemstream_region_context_new(&rcontext, stream, new_region);

 struct pmemstream_entry new_entry;

 int pmemstream_tx_append(tx, rcontext, &e, size_t size , &new_entry);

 int pmemstream_tx_reserve(tx, rcontext, sizeof(e), &new_entry);

 int pmemstream_tx_commit(&tx);

 pmemstream_append(rcontext, &e, sizeof(e), &new_entry);

 struct data_entry *new_data_entry = pmemstream_entry_data(stream, new_entry);
 printf("new_data_entry: %lu\n", new_data_entry->data);
 pmemstream_region_context_delete(&rcontext);

 pmemstream_tx_region_free(tx, new_region);
 pmemstream_region_clear(stream, new_region)

 struct pmemstream_region region;

 pmemstream_delete(&stream);

 #endif /* PMEMSTREAM_H */

