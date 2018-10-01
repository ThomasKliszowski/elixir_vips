defmodule Vips.GetHeaderTest do
  use ExUnit.Case

  test "no file" do
    res =
      Path.join([:code.priv_dir(:vips), "nowhere"])
      |> Vips.get_headers()

    assert {:error, {:file_not_found, _}} = res
  end

  test "bad media" do
    res =
      Path.join([:code.priv_dir(:vips), "test_medias", "media.bad"])
      |> Vips.get_headers()

    assert {:error, {:bad_input, _}} = res
  end

  test "jpg" do
    res =
      Path.join([:code.priv_dir(:vips), "test_medias", "arrival.jpg"])
      |> Vips.get_headers()

    assert {:ok, %{height: 3670, width: 5552}} = res
  end

  test "png" do
    res =
      Path.join([:code.priv_dir(:vips), "test_medias", "pop.png"])
      |> Vips.get_headers()

    assert {:ok, %{height: 480, width: 640}} = res
  end
end
