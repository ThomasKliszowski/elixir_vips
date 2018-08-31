#include "erl_nif.h"
#include "vips/vips.h"
#include "unistd.h"
#include <stdio.h>

static char *
from_elixir_string(ErlNifEnv *env, ERL_NIF_TERM param) {
  ErlNifBinary bin;
  if(!enif_inspect_iolist_as_binary(env, param, &bin)) {
    exit(enif_make_badarg(env));
  }
  char *result = strndup((char*) bin.data, bin.size);
  enif_release_binary(&bin);
  return result;
}

static int
from_elixir_int(ErlNifEnv *env, ERL_NIF_TERM param) {
  int result;
  enif_get_int(env, param, &result);
  return result;
}

static ERL_NIF_TERM
thumbnail(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  char * from_path = from_elixir_string(env, argv[0]);
  char * to_path = from_elixir_string(env, argv[1]);
  int width = from_elixir_int(env, argv[2]);
  int height = from_elixir_int(env, argv[3]);

  // Check if dirty schedulers are enabled
  ErlNifSysInfo sys_info;
  enif_system_info(&sys_info, sizeof(ErlNifSysInfo));
  if(!sys_info.smp_support || !sys_info.dirty_scheduler_support) {
    return enif_make_tuple2(env,
      enif_make_atom(env, "error"),
      enif_make_atom(env, "dirty_schedulers_not_enabled")
    );
  }
  enif_release_resource(&sys_info);

  // Check if `from_path` is an existing path
  if (access(from_path, F_OK) == -1) {
    return enif_make_tuple2(env,
      enif_make_atom(env, "error"),
      enif_make_atom(env, "input_not_found")
    );
  }

  // Create thumbnail and write it to `to_path`
  VipsImage *image;
  vips_thumbnail(from_path, &image, width, "height", height, NULL);
  vips_image_write_to_file(image, to_path, NULL);

  // Cleanup
  g_object_unref(image);
  g_free(from_path);
  g_free(to_path);

  return enif_make_atom(env, "ok");
}

static ErlNifFunc
funcs[] = {{"thumbnail", 4, thumbnail, ERL_NIF_DIRTY_JOB_CPU_BOUND}};

static int
load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info) {
  return vips_init("vips");
}

static void
unload(ErlNifEnv *env, void *priv) {
  vips_shutdown();
}

ERL_NIF_INIT(Elixir.Vips, funcs, &load, NULL, NULL, &unload)
