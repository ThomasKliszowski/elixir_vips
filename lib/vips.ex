defmodule Vips do
  @on_load :init

  def init do
    path = Path.join([:code.priv_dir(:vips), "vips"])
    :ok = :erlang.load_nif(path, 0)
  end

  def thumbnail(from: from, to: to, width: width, height: height) do
    thumbnail(from, to, width, height)
  end

  def thumbnail(_from, _to, _width, _height) do
    raise "NIF thumbnail/4 not implemented"
  end

  def get_headers(_from) do
    raise "NIF get_headers/1 not implemented"
  end

  def get_avg_color(_from) do
    raise "NIF get_avg_color/1 not implemented"
  end
end
