defmodule Vips.GetPOITest do
  use ExUnit.Case

  test "success" do
    res =
      Path.join([:code.priv_dir(:vips), "test_medias", "owl.jpg"])
      |> Vips.get_poi()

    assert {:ok, {_x, _y}} = res
  end
end
