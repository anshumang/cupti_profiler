/*
 * This file is part of cupti_profiler
 * 
 * cupti_profiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cupti_profiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cupti_profiler. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Author: Anshuman Goswami <anshumang@gatech.edu>
 */

#include "CuptiProfiler.h"

CuptiProfiler *p_instance = NULL;

void CUPTIAPI take_buffer(uint8_t **buffer, size_t *size, size_t *max_num_records)
{
  uint8_t *bfr = (uint8_t *) malloc(BUF_SIZE + ALIGN_SIZE);
  if (bfr == NULL) {
    std::cerr << "Error: out of memory" << std::endl;
    std::exit(-1);
  }

  *size = BUF_SIZE;
  *buffer = ALIGN_BUFFER(bfr, ALIGN_SIZE);
  *max_num_records = 0;
}

void CUPTIAPI return_buffer(CUcontext ctx, uint32_t stream_id, uint8_t *buffer, size_t size, size_t valid_size)
{
  CUptiResult status;
  CUpti_Activity *record = NULL;

  if (valid_size > 0) {
    do {
      status = cuptiActivityGetNextRecord(buffer, valid_size, &record);
      if (status == CUPTI_SUCCESS) {
        p_instance->process(record);
      }
      else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED)
        break;
      else {
        CUPTI_CALL(status);
      }
    } while (1);

    // report any records dropped from the queue
    size_t dropped;
    CUPTI_CALL(cuptiActivityGetNumDroppedRecords(ctx, stream_id, &dropped));
    if (dropped != 0) {
      std::cerr << "Dropped " << (unsigned int) dropped << "activity records" << std::endl;
    }

  }

  free(buffer);
}

CuptiProfiler::CuptiProfiler()
  :m_tot_records(0), m_curr_records(0)
{
  size_t attr_val_size = sizeof(size_t);
  CUPTI_CALL(cuptiActivityGetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attr_val_size, &m_cupti_buffer_size));
  CUPTI_CALL(cuptiActivityGetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attr_val_size, &m_cupti_buffer_pool_limit));
  CUPTI_CALL(cuptiActivityRegisterCallbacks(take_buffer, return_buffer));
  CUPTI_CALL(cuptiGetTimestamp(&m_start));
  //Enable tracing
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
}

CuptiProfiler::~CuptiProfiler()
{
  //Disable tracing
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
}

void CuptiProfiler::read()
{
  //Flush trace buffer
  m_curr_records=0;
  CUPTI_CALL(cuptiActivityFlushAll(0));
  m_tot_records+=m_curr_records;
  std::cout << "CuptiProfiler::read " << m_curr_records << "/" << m_tot_records << std::endl;
}

void CuptiProfiler::process(CUpti_Activity *record)
{
  //std::cout << "CuptiProfiler::process" << std::endl;
  m_curr_records++;
}
