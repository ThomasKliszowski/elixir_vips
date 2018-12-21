defmodule Vips.GetAvgColorTest do
  use ExUnit.Case

  test "jpg" do
    res =
      Path.join([:code.priv_dir(:vips), "test_medias", "arrival.jpg"])
      |> Vips.get_avg_color()

    assert {:ok, {r, g, b, a}} = res
  end

  test "jpg grayscale" do
    res =
      Path.join([:code.priv_dir(:vips), "test_medias", "nike.jpg"])
      |> Vips.get_avg_color()

    assert {:ok, {r, g, b, a}} = res
  end

  test "png" do
    res =
      Path.join([:code.priv_dir(:vips), "test_medias", "ps1_logo.png"])
      |> Vips.get_avg_color()

    assert {:ok, {r, g, b, a}} = res
    # Average should include an alpha value
    assert a < 255
  end
end
