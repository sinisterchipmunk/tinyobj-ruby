require "test_helper"
require 'fiddle'
require 'fiddle/import'

class TinyOBJTest < Minitest::Test
  Vertex = Fiddle::Importer.struct(['float pos[3]', 'float normal[3]', 'float texcoord[2]'])

  def setup
    @obj = TinyOBJ.load(File.expand_path('data/mori_knob/testObj.obj', __dir__))
  end

  def test_that_it_has_a_version_number
    refute_nil ::TinyOBJ::VERSION
  end

  def test_obj_names
    refute_nil @obj.to_hash[:materials][0][:name]
  end

  def test_fill_buffers
    # In this example, we'll fill 2 buffers, one with vertex data and the
    # other with index data. In the vertex data we will gather positions,
    # normals and texture coordinates but we will omit vertex colors.

    vertex_stride = Fiddle::SIZEOF_FLOAT * (
                      3 + # 3 floats for each position (x, y, z)
                      3 + # 3 floats for each normal (x, y, z)
                      2   # 2 floats for each texture coord (u, v)
                    )
    index_stride = 2 # each index will be one uint16, or two 8-bit bytes
    vertex_buffer = Fiddle::Pointer.malloc(@obj.num_distinct_vertices * vertex_stride)
    index_buffer  = Fiddle::Pointer.malloc(@obj.num_indices           * index_stride)

    @obj.fill_buffers(positions:     vertex_buffer,
                      normals:       vertex_buffer + Fiddle::SIZEOF_FLOAT * 3,
                      texcoords:     vertex_buffer + Fiddle::SIZEOF_FLOAT * 6,
                      indices:       index_buffer,
                      vertex_stride: vertex_stride,
                      index_stride:  index_stride,
                      index_type:    :uint16)
    
    # vertex_buffer now contains interleaved vertex data, and
    # index_buffer now contains index data. Print the first complete
    # vertex:
    vertex = Vertex.new(vertex_buffer)
    assert_in_delta( 9.375, vertex.pos[0])
    assert_in_delta(-0.5,   vertex.pos[1])
    assert_in_delta(-3.75,  vertex.pos[2])

    assert_in_delta( 0.0, vertex.normal[0])
    assert_in_delta( 1.0, vertex.normal[1])
    assert_in_delta( 0.0, vertex.normal[2])

    assert_in_delta(-2.0, vertex.texcoord[0])
    assert_in_delta(-2.0, vertex.texcoord[1])



    # Test again, swapping normals for colors (same dimensionality/size)
    vertex_buffer = Fiddle::Pointer.malloc(@obj.num_distinct_vertices * vertex_stride)
    index_buffer  = Fiddle::Pointer.malloc(@obj.num_indices           * index_stride)

    @obj.fill_buffers(positions:     vertex_buffer,
                      colors:        vertex_buffer + Fiddle::SIZEOF_FLOAT * 3,
                      texcoords:     vertex_buffer + Fiddle::SIZEOF_FLOAT * 6,
                      indices:       index_buffer,
                      vertex_stride: vertex_stride,
                      index_stride:  index_stride,
                      index_type:    :uint16)
    
    # vertex_buffer now contains interleaved vertex data, and
    # index_buffer now contains index data. Print the first complete
    # vertex:
    vertex = Vertex.new(vertex_buffer)
    assert_in_delta( 9.375, vertex.pos[0])
    assert_in_delta(-0.5,   vertex.pos[1])
    assert_in_delta(-3.75,  vertex.pos[2])

    assert_in_delta( 1.0, vertex.normal[0]) # actually color
    assert_in_delta( 1.0, vertex.normal[1]) # actually color
    assert_in_delta( 1.0, vertex.normal[2]) # actually color

    assert_in_delta(-2.0, vertex.texcoord[0])
    assert_in_delta(-2.0, vertex.texcoord[1])
  end

  def test_index_and_vertex_count
    h = @obj.to_hash
    assert_equal h[:shapes].reduce(0) { |a, shape| a + shape[:mesh][:indices].size },
                 @obj.num_indices

    cache = {}
    h[:shapes].each do |shape|
      shape[:mesh][:indices].each_with_index do |index, i|
        hash = [
                 index[  :vertex_index] && 3.times.map { |i| h[:vertices][ 3 * index[:vertex_index]   + i] },
                 index[  :vertex_index] && 3.times.map { |i| h[:colors][   3 * index[:vertex_index]   + i] },
                 index[  :normal_index] && 3.times.map { |i| h[:normals][  3 * index[:normal_index]   + i] },
                 index[:texcoord_index] && 2.times.map { |i| h[:texcoords][2 * index[:texcoord_index] + i] }
               ]
        cache[hash] = cache.size unless cache.include?(hash)
      end
    end
    assert_equal cache.size, @obj.num_distinct_vertices
  end

  def test_as_hash
    h = @obj.to_hash
    assert_kind_of Hash, h
    assert h[:success]
    assert_includes h, :vertices
    assert_includes h, :normals
    assert_includes h, :texcoords
    assert_includes h, :colors
    assert_includes h, :materials
  end
end
