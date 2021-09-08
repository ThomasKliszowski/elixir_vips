defmodule Vips.NIF do
  @moduledoc false
  @on_load :init

  def init do
    path = Path.join([:code.priv_dir(:vips), "vips"])
    :ok = :erlang.load_nif(path, 0)
  end

  def thumbnail(_from, _to, _width, _height), do: nif_error()
  def to_webp(_from, _to, _quality, _reduction_effort), do: nif_error()
  def get_headers(_from), do: nif_error()
  def get_avg_color(_from), do: nif_error()
  def get_poi(_from), do: nif_error()

  defp nif_error(), do: :erlang.nif_error(:nif_not_loaded)
end
