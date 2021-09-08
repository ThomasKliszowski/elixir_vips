defmodule Vips.ThumbnailTest do
  use ExUnit.Case

  test "no file" do
    from_path = Path.join([:code.priv_dir(:vips), "nowhere"])
    to_path = Briefly.create!(extname: ".jpg")

    res = Vips.thumbnail(from: from_path, to: to_path, width: 100, height: 100)
    assert {:error, {:file_not_found, _}} = res
  end

  test "bad media" do
    from_path = Path.join([:code.priv_dir(:vips), "test_medias", "media.bad"])
    to_path = Briefly.create!(extname: ".jpg")

    res = Vips.thumbnail(from: from_path, to: to_path, width: 100, height: 100)
    assert {:error, {:bad_input, _}} = res
  end

  test "bad output" do
    from_path = Path.join([:code.priv_dir(:vips), "test_medias", "arrival.jpg"])
    to_path = Briefly.create!()

    res = Vips.thumbnail(from: from_path, to: to_path, width: 100, height: 100)
    assert {:error, {:write_to_file_failed, _}} = res
  end

  test "success" do
    from_path = Path.join([:code.priv_dir(:vips), "test_medias", "arrival.jpg"])
    to_path = Briefly.create!(extname: ".jpg")

    res = Vips.thumbnail(from: from_path, to: to_path, width: 100, height: 100)
    assert res == :ok
  end

  test "good dimensions" do
    from_path = Path.join([:code.priv_dir(:vips), "test_medias", "arrival.jpg"])
    to_path = Briefly.create!(extname: ".jpg")

    Vips.thumbnail(from: from_path, to: to_path, width: 100, height: 100)
    res = Vips.get_headers(to_path)
    assert {:ok, %{height: 66, width: 100}} = res
  end

  test "svg" do
    from_path = Path.join([:code.priv_dir(:vips), "test_medias", "shopping-cart.svg[scale=2.25]"])

    to_path = Briefly.create!(extname: ".png")
    res = Vips.thumbnail(from_path, to_path, 100, 100)
    assert res == :ok
  end
end
