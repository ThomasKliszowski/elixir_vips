#include "erl_nif.h"
#include "vips/vips.h"
#include "unistd.h"
#include <stdio.h>

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
  if (access(path, F_OK) == -1)
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

  // Check if dirty schedulers are enabled
  ErlNifSysInfo sys_info;
  enif_system_info(&sys_info, sizeof(ErlNifSysInfo));
  if (!sys_info.smp_support || !sys_info.dirty_scheduler_support)
  {
    return enif_make_tuple2(
        env,
        enif_make_atom(env, "error"),
        enif_make_atom(env, "dirty_schedulers_not_enabled"));
  }
  enif_release_resource(&sys_info);

  // Check if `from_path` is an existing path
  if (access(from_path, F_OK) == -1)
  {
    return enif_make_tuple2(
        env,
        enif_make_atom(env, "error"),
        enif_make_atom(env, "input_not_found"));
  }

  ERL_NIF_TERM result;

  // Create thumbnail and write it to `to_path`
  VipsImage *image;
  if (vips_thumbnail(from_path, &image, width, "height", height, NULL))
  {
    result = enif_make_tuple2(
        env,
        enif_make_atom(env, "error"),
        enif_make_atom(env, "vips_internal_error"));
  }
  else
  {
    if (vips_image_write_to_file(image, to_path, NULL))
    {
      result = enif_make_tuple2(
          env,
          enif_make_atom(env, "error"),
          enif_make_atom(env, "vips_internal_error"));
    }
    else
    {
      result = enif_make_atom(env, "ok");
    }
    g_object_unref(image);
  }

  // Cleanup
  g_free(from_path);
  g_free(to_path);
  vips_error_clear();

  return result;
}

// ----------------------------------------------------------------------------

static ERL_NIF_TERM
get_headers(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{

  char *path = from_elixir_string(env, argv[0]);
  VipsImage *image;
  ERL_NIF_TERM err;

  if (!load_vips_image(env, path, &image, &err))
    return err;

  int width = vips_image_get_width(image);
  int height = vips_image_get_height(image);

  g_object_unref(image);

  ERL_NIF_TERM stats = enif_make_new_map(env);
  enif_make_map_put(env, stats, enif_make_atom(env, "width"), enif_make_int(env, width), &stats);
  enif_make_map_put(env, stats, enif_make_atom(env, "height"), enif_make_int(env, height), &stats);

  return enif_make_tuple2(
      env,
      enif_make_atom(env, "ok"),
      stats);
}

// ----------------------------------------------------------------------------

static ErlNifFunc funcs[] = {
    {"thumbnail", 4, thumbnail, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"get_headers", 1, get_headers, ERL_NIF_DIRTY_JOB_IO_BOUND},
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
