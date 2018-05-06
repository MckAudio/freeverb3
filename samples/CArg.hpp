/**
 *  CArg arg table
 *
 *  Copyright (C) 2007-2018 Teru Kamogashira
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CArg_HPP
#define __CArg_HPP

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <map>

class CArg
{
public:
  CArg();
  int registerArg(int argc, char ** argv);
  const char * getString(const char * key);
  int getInt(const char * key);
  long getLong(const char * key);
  double getDouble(const char * key);
  const char * getFileArg(unsigned at);
private:
  char blank[1];
  std::map<std::string,std::string> optionMap;
  std::vector<std::string> argVector;
};

#endif
