require "test_helper"

class TinyOBJTest < Minitest::Test
  def test_that_it_has_a_version_number
    refute_nil ::TinyOBJ::VERSION
  end

  def test_it_does_something_useful
    obj = TinyOBJ.load(File.expand_path('data/mori_knob/testObj.obj', __dir__))
    p obj.inspect.size
  end
end
