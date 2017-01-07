 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs

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

    HISTORY:
    0.1.00 - 2017-01-06 initial version
    
 ****************************************************/

#define PITMASTER1 15

float esum = 0;
int16_t pid_pause = 2000;
float eLast = 0;

struct PID {
  float p;
  float i;
  float d;
  int imin;
  int imax;
};

PID p1 = { 3.8 , 0.01 , 128 , 0 , 95};

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set Pitmaster Pin
void set_pitmaster() {
  
  pinMode(PITMASTER1, OUTPUT);
  digitalWrite(PITMASTER1, LOW);
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PID
int PID_Regler (float x, float w){   // x:IST,  w:SOLL
  
  float e = w - x;                        // Abweichung bestimmen  

  esum = esum + e*pid_pause/1000.0;       // Integralanteil             
      
  // ANTI-WIND-UP (sonst Verzögerung)
  // Limits an Ki anpassen: Ki*limit muss y_limit ergeben können
  if (esum * p1.i > p1.imax) esum = p1.imax/p1.i;
  else if (esum * p1.i < p1.imin) esum = p1.imin/p1.i;
  
  float edif = (e - eLast)/(pid_pause/1000.0);   // Differentialanteil
  eLast = e;

  float p_out = p1.p * e;
  float i_out = p1.i * esum;
  float d_out = p1.d * edif;

  float y = p_out + i_out + d_out;  // PI-Regler  
  y = constrain(y,-1023,1023);            // Auflösung am Ausgang ist begrenzt

  return map(y,-1023,1023,0,100);
}






