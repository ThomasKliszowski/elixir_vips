#include "erl_nif.h"
#include "vips/vips.h"
#include "unistd.h"
#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------------------

static char *
from_elixir_string(ErlNifEnv *env, ERL_NIF_TERM param)
{
  ErlNifBinary bin;
  if (!enif_inspect_iolist_as_binary(env, param, &bin))
  {
    exit(enif_make_badarg(env));
  }
  char *result = strndup((char *)bin.data, bin.size);
  enif_release_binary(&bin);
  return result;
}

static int
from_elixir_int(ErlNifEnv *env, ERL_NIF_TERM param)
{
  int result;
  enif_get_int(env, param, &result);
  return result;
}

static ERL_NIF_TERM
make_elixir_binary(ErlNifEnv *env, const char *buffer)
{
  ERL_NIF_TERM res;
  unsigned char *tmp = enif_make_new_binary(env, strlen(buffer), &res);
  strncpy((char *)tmp, buffer, strlen(buffer));
  return res;
}

static ERL_NIF_TERM
elixir_vips_error(ErlNifEnv *env, const char *err_type, const char *msg)
{
  if (!msg)
    msg = vips_error_buffer();

  ERL_NIF_TERM ctx = enif_make_tuple2(
      env,
      enif_make_atom(env, err_type),
      make_elixir_binary(env, msg));

  vips_error_clear();

  return enif_make_tuple2(
      env,
      enif_make_atom(env, "error"),
      ctx);
}

static char *fmt(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  size_t needed = vsnprintf(NULL, 0, format, args) + 1;
  char *buffer = malloc(needed);
  va_start(args, format);
  vsnprintf(buffer, needed, format, args);
  va_end(args);
  return buffer;
}

// ----------------------------------------------------------------------------

static int
assert_dirty_schedulers(ErlNifEnv *env, ERL_NIF_TERM *err)
{
  int ret = 1;
  ErlNifSysInfo sys_info;
  enif_system_info(&sys_info, sizeof(ErlNifSysInfo));
  if (!sys_info.smp_support || !sys_info.dirty_scheduler_support)
  {
    *err = enif_make_tuple2(
        env,
        enif_make_atom(env, "error"),
        enif_make_atom(env, "dirty_schedulers_not_enabled"));
    ret = 0;
  }
  enif_release_resource(&sys_info);
  return ret;
}

static int
assert_file_exists(ErlNifEnv *env, const char *path, ERL_NIF_TERM *err)
{
  char *safe_path = strtok(strdup(path), "[");
  if (access(safe_path, F_OK) == -1)
  {
    *err = elixir_vips_error(
        env,
        "file_not_found",
        fmt("File '%s' not found", path));
    return 0;
  }
  return 1;
}

static int
load_vips_image(ErlNifEnv *env, const char *path, VipsImage **image, ERL_NIF_TERM *err)
{
  if (!assert_file_exists(env, path, err))
    return 0;

  *image = vips_image_new_from_file(path, NULL);
  if (!*image)
  {
    *err = elixir_vips_error(env, "bad_input", NULL);
    return 0;
  }

  return 1;
}

// ----------------------------------------------------------------------------

static ERL_NIF_TERM
thumbnail(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  char *from_path = from_elixir_string(env, argv[0]);
  char *to_path = from_elixir_string(env, argv[1]);
  int width = from_elixir_int(env, argv[2]);
  int height = from_elixir_int(env, argv[3]);

  VipsImage *in = NULL;
  VipsImage *out = NULL;
  ERL_NIF_TERM res;

  if (assert_dirty_schedulers(env, &res))
    if (load_vips_image(env, from_path, &in, &res))
    {
      if (vips_thumbnail_image(in, &out, width, "height", height, NULL))
      {
        res = elixir_vips_error(env, "thumbnail_image_failed", NULL);
      }
      else
      {
        if (vips_image_write_to_file(out, to_path, NULL))
        {
          res = elixir_vips_error(env, "write_to_file_failed", NULL);
        }
        else
        {
          res = enif_make_atom(env, "ok");
        }
        g_object_unref(out);
      }
      g_object_unref(in);
    }

  g_free(from_path);
  g_free(to_path);
  return res;
}

// ----------------------------------------------------------------------------

static ERL_NIF_TERM
get_headers(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  char *path = from_elixir_string(env, argv[0]);
  VipsImage *image = NULL;
  ERL_NIF_TERM res;

  if (load_vips_image(env, path, &image, &res))
  {
    int width = vips_image_get_width(image);
    int height = vips_image_get_height(image);

    ERL_NIF_TERM stats = enif_make_new_map(env);
    enif_make_map_put(env, stats, enif_make_atom(env, "width"), enif_make_int(env, width), &stats);
    enif_make_map_put(env, stats, enif_make_atom(env, "height"), enif_make_int(env, height), &stats);

    res = enif_make_tuple2(
        env,
        enif_make_atom(env, "ok"),
        stats);

    g_object_unref(image);
  }

  g_free(path);
  return res;
}

// ----------------------------------------------------------------------------

static double get_stats_point(VipsImage *stats, int x, int y)
{
  double value;
  double *vector;
  int l;
  vips_getpoint(stats, &vector, &l, x, y, NULL);
  value = vector[0];
  g_free(vector);
  return value;
}

static ERL_NIF_TERM
get_avg_color(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  char *path = from_elixir_string(env, argv[0]);
  VipsImage *image = NULL;
  ERL_NIF_TERM res;

  if (load_vips_image(env, path, &image, &res))
  {
    VipsImage *stats;
    vips_stats(image, &stats, NULL);

    if (stats->Ysize < 4 || stats->Ysize > 5)
    {
      res = elixir_vips_error(env, "bad_input", fmt("Unsupported number of bands for file `%s`", path));
    }
    else
    {
      double alpha = 255;
      if (stats->Ysize == 5)
      {
        alpha = get_stats_point(stats, 4, 4);
      }

      res = enif_make_tuple2(
          env,
          enif_make_atom(env, "ok"),
          enif_make_tuple4(
              env,
              enif_make_double(env, get_stats_point(stats, 4, 1)), // avg red
              enif_make_double(env, get_stats_point(stats, 4, 2)), // avg green
              enif_make_double(env, get_stats_point(stats, 4, 3)), // avg blue
              enif_make_double(env, alpha)));
    }

    g_object_unref(stats);
    g_object_unref(image);
  }

  g_free(path);
  return res;
}

// ----------------------------------------------------------------------------

static ERL_NIF_TERM
get_poi(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  char *from_path = from_elixir_string(env, argv[0]);

  VipsImage *in = NULL;
  VipsImage *out = NULL;
  ERL_NIF_TERM res;

  if (assert_dirty_schedulers(env, &res))
    if (load_vips_image(env, from_path, &in, &res))
    {
      int target_size = (in->Xsize > in->Ysize ? in->Ysize : in->Xsize) * 0.8;
      if (vips_smartcrop(in, &out, target_size, target_size, "interesting", VIPS_INTERESTING_ENTROPY, NULL))
      {
        res = elixir_vips_error(env, "smartcrop_failed", NULL);
      }
      else
      {
        res = enif_make_tuple2(
            env,
            enif_make_atom(env, "ok"),
            enif_make_tuple2(
                env,
                enif_make_int(env, -out->Xoffset + (out->Xsize / 2)),
                enif_make_int(env, -out->Yoffset + (out->Ysize / 2))));

        g_object_unref(out);
      }
      g_object_unref(in);
    }

  g_free(from_path);
  return res;
}

// ----------------------------------------------------------------------------

static ErlNifFunc funcs[] = {
    {"thumbnail", 4, thumbnail, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"get_headers", 1, get_headers, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"get_avg_color", 1, get_avg_color, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"get_poi", 1, get_poi, ERL_NIF_DIRTY_JOB_CPU_BOUND},
};

// ----------------------------------------------------------------------------

static int
load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
  if (vips_init("vips"))
  {
    vips_error_exit("Error loading vips");
  }
  return 0;
}

static void
unload(ErlNifEnv *env, void *priv)
{
  vips_shutdown();
}

static int
reload(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
  return 0;
}

static int
upgrade(ErlNifEnv *env, void **priv, void **old_priv, ERL_NIF_TERM info)
{
  return load(env, priv, info);
}

// ----------------------------------------------------------------------------

ERL_NIF_INIT(Elixir.Vips, funcs, &load, &reload, &upgrade, &unload)
