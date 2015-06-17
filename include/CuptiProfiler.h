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

#ifndef _CUPTI_PROFILER_H
#define _CUPTI_PROFILER_H

#include <utility>
#include <vector>
#include <cstddef>

struct CuptiProfiler
{
   typedef std::pair<unsigned long long, unsigned long long> StartSpanTuple; 
   typedef std::vector< StartSpanTuple > StartSpanTupleVector;
   StartSpanTupleVector m_pair_vec;
   CuptiProfiler();
   ~CuptiProfiler();
   void read();
private:
   void sort();
};

#endif
