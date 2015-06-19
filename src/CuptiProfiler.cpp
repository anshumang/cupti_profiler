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
        p_instance->insert(record);
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
  :m_tot_records(0), m_curr_records(0), m_last(0)
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
  std::cout << "CuptiProfiler::read " << m_curr_records << "/" << m_tot_records << "/" << m_tot_disjoint_records << std::endl;
  this->process();
}

void CuptiProfiler::insert(CUpti_Activity *record)
{
  //std::cout << "CuptiProfiler::process" << std::endl;
  m_curr_records++;
  if((record->kind == CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL)||(record->kind == CUPTI_ACTIVITY_KIND_KERNEL))
  {
      CUpti_ActivityKernel2 *kernel = (CUpti_ActivityKernel2 *) record;
      CuptiTuple curr_tup(kernel->start-m_start, kernel->end-m_start);
      if(m_last)
      {
      //std::cout << m_curr_records << " " << kernel->end - kernel->start << " " << kernel->start-m_start - m_last << " " << kernel->start-m_start << " " << kernel->end-m_start << std::endl;
      }
      else
      {
      //std::cout << m_curr_records << " " << kernel->end - kernel->start << " " << kernel->start-m_start << " " << kernel->end-m_start << std::endl;
      }
      //std::cout << kernel->start-m_start << " " << kernel->end-m_start << std::endl;
      m_last = kernel->end-m_start;
      m_tup_vec_raw.push_back(curr_tup);
  }
}

bool CuptiProfiler::cupti_tuple_compare(CuptiTuple& first, CuptiTuple& second) 
{
  return first.first < second.first;
}

void CuptiProfiler::process()
{
   /*std::sort(m_tup_vec_raw.begin(), m_tup_vec_raw.end(), [](CuptiTuple& first, CuptiTuple& second)
   {
     return first.first < second.first;
   }
   )*/;
   //std::cout << "m_tot_records = " << m_tot_records << " m_curr_records = " << m_curr_records << std::endl;
   int offset = m_tot_records-m_curr_records;
   std::sort(m_tup_vec_raw.begin()+offset, m_tup_vec_raw.end());
   int count=1;
   m_tup_vec.push_back(m_tup_vec_raw[offset]);
   int disjoint_records=1;
   //std::cout << "Size " << m_tup_vec_raw.size() << " offset " << offset << std::endl;
   while (offset+count<m_tup_vec_raw.size())
   {
     unsigned long last_start, last_end, curr_start, curr_end;
     last_start = m_tup_vec.back().first;
     last_end = m_tup_vec.back().second;
     //std::cout << "last " << disjoint_records-1 << " " << last_start << " " << last_end << std::endl;
     curr_start = m_tup_vec_raw[offset+count].first;
     curr_end = m_tup_vec_raw[offset+count].second;
     //std::cout << "curr " << count << " " << curr_start << " " << curr_end << std::endl;
     int last_count = count, lookahead_search=0, lookahead_search_empty=1;

     if(curr_start > last_end)
     {
        //std::cout << "DISJOINT " << count << " " << curr_end - curr_start << " " << last_end << " " << curr_start << " " << curr_start-last_end << std::endl;
        m_tup_vec.push_back(m_tup_vec_raw[m_tot_disjoint_records+count]);
        disjoint_records++;
     }
     else
     {
        //std::cout << "OVERLAP " << count << " " << curr_end - curr_start << " " << last_end << " " << curr_start << " " << curr_start-last_end << std::endl;
        lookahead_search=1;
        int lookahead=1;
        while (lookahead < m_tup_vec_raw.size()-count-1)
        {
           unsigned lookahead_start, lookahead_end;
           lookahead_start = m_tup_vec_raw[offset+count+lookahead].first;   
           lookahead_end = m_tup_vec_raw[offset+count+lookahead].second;   
           if(lookahead_start>last_end)
           {
             int prev_end = m_tup_vec_raw[offset+count+lookahead-1].second;
             if(prev_end > last_end)
             {
		m_tup_vec.pop_back();
                m_tup_vec.push_back(CuptiTuple(last_start, prev_end));
             }
             m_tup_vec.push_back(CuptiTuple(lookahead_start, lookahead_end));
             disjoint_records++;
             count = count + lookahead + 1;
             lookahead_search_empty=0;
             break;
           }
           lookahead++;
        }
     }

     if(lookahead_search && lookahead_search_empty)
        break;

     if(last_count == count)
     {
       count++;
     }
   }
   
   count = 0;
   while(count < disjoint_records)
   {
      std::cout << count << " " << m_tup_vec[m_tot_disjoint_records+count].second - m_tup_vec[m_tot_disjoint_records+count].first << std::endl;
      count++; 
   }
   m_tot_disjoint_records+=disjoint_records;
}
