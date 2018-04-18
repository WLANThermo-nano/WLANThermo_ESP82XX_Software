 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs

    The elements of this program were taken from the work of
    AUTHOR: Rob dot Tillaart at gmail dot com
    PURPOSE: RunningMedian library for Arduino 0.1.13

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    HISTORY: Please refer Github History
    
 ****************************************************/


#define MEDIAN_SIZE   16        // Einfluss auf Batterie-Simulation
uint16_t _cnt;
uint16_t _idx;
boolean _sorted;
int _ar[MEDIAN_SIZE];
uint16_t _p[MEDIAN_SIZE];


 //++++++++++++++++++++++++++++++++++++++++++++++++++++++
// add Value to Buffer
void median_add(int value) {
    
    _ar[_idx++] = value;
    if (_idx >= MEDIAN_SIZE) _idx = 0; // wrap around
    if (_cnt < MEDIAN_SIZE) _cnt++;
    _sorted = false;
}

void median_clear()
{
    _cnt = 0;
    _idx = 0;
    _sorted = false;
    for (uint8_t i = 0; i< MEDIAN_SIZE; i++) _p[i] = i;
}


 //++++++++++++++++++++++++++++++++++++++++++++++++++++++
// sort Buffer
void median_sort() {
    // bubble sort with flag
    for (uint8_t i = 0; i < _cnt-1; i++) {
        bool flag = true;
        for (uint8_t j = 1; j < _cnt-i; j++) {
            if (_ar[_p[j-1]] > _ar[_p[j]]) {
                uint8_t t = _p[j-1];
                _p[j-1] = _p[j];
                _p[j] = t;
                flag = false;
            }
        }
        if (flag) break;
    }
    _sorted = true;
}


 //++++++++++++++++++++++++++++++++++++++++++++++++++++++
// get Median from Buffer
double median_get()
{
    if (_cnt > 0)
    {
        if (_sorted == false) median_sort();
        if (_cnt & 0x01) return _ar[_p[_cnt/2]];
        else return (_ar[_p[_cnt/2]] + _ar[_p[_cnt/2 - 1]]) / 2;
    }
    return NAN;
}

double median_getHighest()
{
  if (_cnt > 0)
    {
        if (_sorted == false) median_sort();
        return _ar[_p[_cnt-1]];
    }
    return NAN;
}

double median_average()
{
    if (_cnt > 0)
    {
        double sum = 0;
        for (uint8_t i = 0; i < _cnt; i++) sum += _ar[i];
        return sum / _cnt;
    }
    return NAN;
}


