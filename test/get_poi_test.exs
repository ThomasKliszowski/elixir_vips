defmodule Vips.GetPOITest do
  use ExUnit.Case

  test "success" do
    res =
      Path.join([:code.priv_dir(:vips), "test_medias", "owl.jpg"])
      |> Vips.get_poi()

    assert {:ok, {x, y}} = res
  end
end
