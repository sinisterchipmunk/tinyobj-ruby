# TinyOBJ

Provides a Ruby interface around [TinyOBJ](https://github.com/syoyo/tinyobjloader).

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'tiny_obj'
```

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install tiny_obj

## Usage

```ruby
    require 'tiny_obj'
    obj = TinyOBJ.load("path/to/file.obj") # finds materials in /path/to
    obj = TinyOBJ.load("path/to/file.obj", "path/to/materials/dir")

    hash = obj.to_hash
    #=> hash is a hash containing :materials, :vertices, :shapes, and other
    #   goodness.
```

Converting the OBJ into a hash is convenient but not performant. If you are
dealing with a large object, you may wish to fill a buffer with data without
having to convert it into Ruby hashes, arrays and numbers. You can do that
with TinyOBJ#fill_buffers. Below is a complete example, which uses Fiddle to
allocate the buffer.

**Note** that using Fiddle is not required (but is convenient since it ships
with Ruby). Only knowing the memory address of the buffer is necessary. Be
aware that using the wrong address or not a real (or adequately-sized) buffer
can lead to undefined behavior and program crashes.

```ruby
vertex_stride = Fiddle::SIZEOF_FLOAT * (
                  3 + # 3 floats for each position (x, y, z)
                  3 + # 3 floats for each normal (x, y, z)
                  2   # 2 floats for each texture coord (u, v)
                )

index_stride  = 2 # each index will be one uint16, or two 8-bit bytes
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
# index_buffer now contains index data.
```

## Development

After checking out the repo, run `bin/setup` to install dependencies. Then, run `rake test` to run the tests. You can also run `bin/console` for an interactive prompt that will allow you to experiment.

To install this gem onto your local machine, run `bundle exec rake install`. To release a new version, update the version number in `version.rb`, and then run `bundle exec rake release`, which will create a git tag for the version, push git commits and tags, and push the `.gem` file to [rubygems.org](https://rubygems.org).

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/sinisterchipmunk/tinyobj-ruby.

## License

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT).
