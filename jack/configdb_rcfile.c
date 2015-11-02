#ifdef HAVE_CONFIG_H
#include "fv3_config.h"
#endif

#include "configdb.h"
#include <string.h>
#include "rcfile.h"

#define RCFILE_DEFAULT_SECTION_NAME "fv3_jack"

struct _ConfigDb
{
    RcFile *file;
    gchar *filename;
    gboolean dirty;
};


ConfigDb * bmp_cfg_db_open()
{
  ConfigDb *db;
  db = g_new(ConfigDb, 1);
  db->filename = g_build_filename(g_get_home_dir(), BMP_RCPATH, "config", NULL);
  db->file = bmp_rcfile_open(db->filename);
  if(!db->file) db->file = bmp_rcfile_new();
  db->dirty = FALSE;
  return db;
}

void bmp_cfg_db_close(ConfigDb * db)
{
  if(db->dirty) bmp_rcfile_write(db->file, db->filename);
  bmp_rcfile_free(db->file);
  g_free(db->filename);
}

gboolean bmp_cfg_db_get_string(ConfigDb * db, const gchar * section, const gchar * key, gchar ** value)
{
  if(!section) section = RCFILE_DEFAULT_SECTION_NAME;
  return bmp_rcfile_read_string(db->file, section, key, value);
}

gboolean bmp_cfg_db_get_int(ConfigDb * db, const gchar * section, const gchar * key, gint * value)
{
  if(!section) section = RCFILE_DEFAULT_SECTION_NAME;
  return bmp_rcfile_read_int(db->file, section, key, value);
}

gboolean bmp_cfg_db_get_bool(ConfigDb * db, const gchar * section, const gchar * key, gboolean * value)
{
  if(!section) section = RCFILE_DEFAULT_SECTION_NAME;
  return bmp_rcfile_read_bool(db->file, section, key, value);
}

gboolean bmp_cfg_db_get_float(ConfigDb * db, const gchar * section, const gchar * key, gfloat * value)
{
  if(!section) section = RCFILE_DEFAULT_SECTION_NAME;
  return bmp_rcfile_read_float(db->file, section, key, value);
}

gboolean bmp_cfg_db_get_double(ConfigDb * db, const gchar * section, const gchar * key, gdouble * value)
{
  if(!section) section = RCFILE_DEFAULT_SECTION_NAME;
  return bmp_rcfile_read_double(db->file, section, key, value);
}

void bmp_cfg_db_set_string(ConfigDb * db, const gchar * section, const gchar * key, gchar * value)
{
  db->dirty = TRUE;
  if(!section) section = RCFILE_DEFAULT_SECTION_NAME;
  bmp_rcfile_write_string(db->file, section, key, value);
}

void bmp_cfg_db_set_int(ConfigDb * db, const gchar * section, const gchar * key, gint value)
{
  db->dirty = TRUE;
  if(!section) section = RCFILE_DEFAULT_SECTION_NAME;
  bmp_rcfile_write_int(db->file, section, key, value);
}

void bmp_cfg_db_set_bool(ConfigDb * db, const gchar * section, const gchar * key, gboolean value)
{
  db->dirty = TRUE;
  if (!section) section = RCFILE_DEFAULT_SECTION_NAME;
  bmp_rcfile_write_boolean(db->file, section, key, value);
}

void bmp_cfg_db_set_float(ConfigDb * db, const gchar * section, const gchar * key, gfloat value)
{
  db->dirty = TRUE;
  if (!section) section = RCFILE_DEFAULT_SECTION_NAME;
  bmp_rcfile_write_float(db->file, section, key, value);
}

void bmp_cfg_db_set_double(ConfigDb * db, const gchar * section, const gchar * key, gdouble value)
{
  db->dirty = TRUE;
  if(!section) section = RCFILE_DEFAULT_SECTION_NAME;
  bmp_rcfile_write_double(db->file, section, key, value);
}

void bmp_cfg_db_unset_key(ConfigDb * db, const gchar * section, const gchar * key)
{
    db->dirty = TRUE;
    g_return_if_fail(db != NULL);
    g_return_if_fail(key != NULL);
    if (!section) section = RCFILE_DEFAULT_SECTION_NAME;
    bmp_rcfile_remove_key(db->file, section, key);
}

gchar * cfg_string;
gchar * aud_get_str(const gchar *section, const gchar *key)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_get_string(cfg,section,key,&cfg_string);
  bmp_cfg_db_close(cfg);
  return cfg_string;
}

gint cfg_int;
gint aud_get_int(const gchar *section, const gchar *key)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_get_int(cfg,section,key,&cfg_int);
  bmp_cfg_db_close(cfg);
  return cfg_int;
}

gboolean cfg_bool;
gboolean aud_get_bool(const gchar *section, const gchar *key)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_get_bool(cfg,section,key,&cfg_bool);
  bmp_cfg_db_close(cfg);
  return cfg_bool;
}

gfloat cfg_float;
gfloat aud_get_float(const gchar *section, const gchar *key)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_get_float(cfg,section,key,&cfg_float);
  bmp_cfg_db_close(cfg);
  return cfg_float;
}

gdouble cfg_double;
gdouble aud_get_double(const gchar *section, const gchar *key)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_get_double(cfg,section,key,&cfg_double);
  bmp_cfg_db_close(cfg);
  return cfg_double;
}

void aud_set_str(const gchar *section, const gchar *key, gchar *value)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_set_string(cfg,section,key,value);
  bmp_cfg_db_close(cfg);
}

void aud_set_int(const gchar *section, const gchar *key, gint value)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_set_int(cfg,section,key,value);
  bmp_cfg_db_close(cfg);
}

void aud_set_bool(const gchar *section, const gchar *key, gboolean value)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_set_bool(cfg,section,key,value);
  bmp_cfg_db_close(cfg);
}

void aud_set_float(const gchar *section, const gchar *key, gfloat value)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_set_float(cfg,section,key,value);
  bmp_cfg_db_close(cfg);
}

void aud_set_double(const gchar *section, const gchar *key, gdouble value)
{
  ConfigDb * cfg = bmp_cfg_db_open();
  bmp_cfg_db_set_double(cfg,section,key,value);
  bmp_cfg_db_close(cfg);
}
