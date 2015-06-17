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

extern CuptiProfiler *p_instance;

extern "C"
void CuptiProfilerCreate()
{
   p_instance = new CuptiProfiler();
}

extern "C"
void CuptiProfilerDestroy()
{
   delete p_instance;
}

extern "C"
void CuptiProfilerRead()
{
   p_instance->read();
}
