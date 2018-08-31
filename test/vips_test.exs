defmodule VipsTest do
  use ExUnit.Case
  doctest Vips

  test "greets the world" do
    assert Vips.hello() == :world
  end
end
