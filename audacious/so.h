/**
 *  XMMS/BMP/audacious plugin Framework
 *
 *  Copyright (C) 2006-2014 Teru Kamogashira
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

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct
{
  const char * name;
  gboolean (*init)(void);
  void (*cleanup)(void);
  void (*about)(void);
  void (*configure)(void);
  void (*start)(int *channels, int *rate);
  void (*process)(float **data, int *samples);
  void (*flush)(void);
  void (*finish)(float **data, int *samples);
} LibXmmsPluginTable;

#ifdef  __cplusplus
}
#endif
