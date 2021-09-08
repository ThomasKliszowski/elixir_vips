defmodule Vips.ToWebpTest do
  use ExUnit.Case

  test "jpeg" do
    from_path = Path.join([:code.priv_dir(:vips), "test_medias", "arrival.jpg"])
    to_path = Briefly.create!(extname: ".webp")
    res = Vips.to_webp(from: from_path, to: to_path)
    assert res == :ok
  end

  test "transparency" do
    from_path = Path.join([:code.priv_dir(:vips), "test_medias", "ps1_logo.png"])
    to_path = Briefly.create!(extname: ".webp")
    res = Vips.to_webp(from: from_path, to: to_path)
    assert res == :ok
  end
end
