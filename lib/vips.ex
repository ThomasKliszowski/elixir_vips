defmodule Vips do
  alias Vips.NIF

  defmodule ImageHeaders do
    @type t :: %__MODULE__{
            width: integer(),
            height: integer()
          }
    defstruct [:width, :height]
  end

  @spec thumbnail(
          from: String.t(),
          to: String.t(),
          width: integer,
          height: integer
        ) :: :ok | any
  def thumbnail(opts \\ []) do
    NIF.thumbnail(
      opts[:from],
      opts[:to],
      opts[:width],
      opts[:height]
    )
  end

  @spec to_webp(
          from: String.t(),
          to: String.t(),
          quality: integer,
          reduction_effort: integer
        ) :: :ok | any
  def to_webp(opts \\ []) do
    NIF.to_webp(
      opts[:from],
      opts[:to],
      Keyword.get(opts, :quality, 80),
      Keyword.get(opts, :reduction_effort, 2)
    )
  end

  @spec get_headers(String.t()) :: {:ok, ImageHeaders.t()} | any
  def get_headers(from), do: NIF.get_headers(from)

  @spec get_avg_color(String.t()) :: :ok | any
  def get_avg_color(from), do: NIF.get_avg_color(from)

  @spec get_poi(String.t()) :: :ok | any
  def get_poi(from), do: NIF.get_poi(from)
end
