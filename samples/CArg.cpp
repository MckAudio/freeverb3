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

#include "CArg.hpp"

CArg::CArg()
{
  blank[0] = '\0';
}

int CArg::registerArg(int argc, char ** argv)
{
  for(int i = 1;i < argc;i ++)
    {
      if(strlen(argv[i]) > 1&&strncmp(argv[i], "-", 1) == 0)
	{
	  std::string option, value;
	  std::pair<std::map<std::string,std::string>::iterator,bool> result;
	  if(i + 1 >= argc)
	    {
	      std::cerr << "CArg::registerArg: Missing value to ["
			<< argv[i] << "].\n" << std::endl;
	      return -1;
	    }
	  option = argv[i];
	  i ++;
	  value = argv[i];
	  result =
	    optionMap.insert(std::pair<std::string,std::string>(option,value));
	  if(!result.second)
	    {
	      optionMap[option] = value;
	      std::cerr << "CArg::registerArg: Value to ["
			<< option << "] was overwritten.\n" << std::endl;
	    }
	}
      else
	{
	  argVector.push_back(argv[i]);
	}
    }
  return 0;
}

int CArg::getInt(const char * key)
{
  return std::atol(getString(key));
} 

long CArg::getLong(const char * key)
{
  return std::atol(getString(key));
} 

double CArg::getDouble(const char * key)
{
  return std::atof(getString(key));
} 

const char * CArg::getString(const char * key)
{
  std::string keyString = key;
  std::map<std::string,std::string>::iterator result;
  result = optionMap.find(keyString);
  if(result != optionMap.end())
    {
      return result->second.c_str();
    }
  return blank;
}

const char * CArg::getFileArg(unsigned at)
{
  if(argVector.size() > at)
    {
      return argVector[at].c_str();
    }
  return blank;
}
