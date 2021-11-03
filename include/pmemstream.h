 /* SPDX-License-Identifier: BSD-3-Clause */
 /* Copyright 2020-2021, Intel Corporation */

 /// @file

 #ifndef PMEMSTREAM_H
 #define PMEMSTREAM_H

/** Stream structure
 *
 * @see pmem.io
 * */
 struct pmemstream{
     /** Buffer */
     char *buf;

     /** Buffer size*/
     size_t size;
 };


 /** Pmemstream transaction */
 struct pmemstream_tx{
 };

/** Create new transaction
 *
 * @param[out] tx new transaction
 * @param[in] stream stream
 *
 * @relates pmemstream_tx
 * */
int  pmemstream_tx_new(&tx, stream);

 /** Stream region */
 struct pmemstream_region{
 };


 int pmemstream_from_map(&stream, size_t size, map);

 int  pmemstream_tx_region_allocate(pmemstream_tx *tx, *stream, size_t size,pmemstream_region *new_region);

 struct pmemstream_region_context *rcontext;

 int  pmemstream_region_context_new(&rcontext, stream, new_region);

 struct pmemstream_entry new_entry;

/** Append data to stream inside a transaction.
 *
 * @param [in] tx transaction
 * @param [in] rcontext region context
 * @param [in] e entry
 * @param [in] size size of entry
 * @param [out] new_entry new entr
 *
 * @relates pmemstream
 * @return status
 * */
 int pmemstream_tx_append(pmemstream_tx tx, pmemstream_region rcontext, &e, size_t size , &new_entry);

 int pmemstream_tx_reserve(tx, rcontext, sizeof(e), &new_entry);

 int pmemstream_tx_commit(&tx);

/** Append one entry transactionally.
 *
 * @param [in] context some context
 * @param [in] e entry
 * @param [in] size size of entry
 * @param [out] new_entry new entr
 *
 * @relates pmemstream
 * @return status
 * */
 pmemstream_append(rcontext, &e, size_t size, &new_entry);

 struct data_entry *new_data_entry = pmemstream_entry_data(stream, new_entry);
 printf("new_data_entry: %lu\n", new_data_entry->data);
 pmemstream_region_context_delete(&rcontext);

 pmemstream_tx_region_free(tx, new_region);
 pmemstream_region_clear(stream, new_region)

 struct pmemstream_region region;

 /** Delete existing stream
  *
  * @relates pmemstream
  * */
 pmemstream_delete(&stream);

 #endif /* PMEMSTREAM_H */

