/*
        Copyright 2021 Ian Tester

        This file is part of Photo Finish.

        Photo Finish is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        Photo Finish is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with Photo Finish.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#define cmsTRIPLE(N, T, A, B, C) class N {\
 private:\
 cms##N *_internal;\
 bool _own_data;\
 N(cms##N* n) :\
   _internal(n), _own_data(false) {}\
 cms##N* _data(void) const { return _internal; }\
 friend class Profile;\
 public:\
 N() {}\
 N(T a, T b, T c) : _internal(new cms##N), _own_data(true) {\
  _internal->A = a; _internal->B = b; _internal->C = c; } \
 ~N() { if (_own_data) delete _internal; }\
 T A(void) const { return _internal->A; }\
 N& set_##A(T a) { _internal->A = a; return *this; } \
 T B(void) const { return _internal->B; }\
 N& set_##B(T b) { _internal->B = b; return *this; }\
 T C(void) const { return _internal->C; }\
 N& set_##C(T c) { _internal->C = c; return *this; }\
 };

namespace CMS {

  class Profile;

  cmsTRIPLE(CIEXYZ,double,X,Y,Z);
  cmsTRIPLE(CIExyY,double,x,y,Y);
  cmsTRIPLE(CIELab,double,L,a,b);
  cmsTRIPLE(CIELCh,double,L,C,h);
  cmsTRIPLE(JCh,double,J,C,h);

  cmsTRIPLE(CIEXYZTRIPLE,cmsCIEXYZ,Red,Green,Blue);
  cmsTRIPLE(CIExyYTRIPLE,cmsCIExyY,Red,Green,Blue);

}; // namespace CMS

#undef cmsTRIPLE
