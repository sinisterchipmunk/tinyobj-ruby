#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_DOUBLE
#include "rb_tinyobj.h"
#include <stdlib.h>
#include <unordered_map>
#include <stdint.h>

// taken from Fiddle
#if SIZEOF_VOIDP == SIZEOF_LONG
# define NUM2PTR(x)   ((void*)(NUM2ULONG(x)))
#else
# define NUM2PTR(x)   ((void*)(NUM2ULL(x)))
#endif

#define NIL_OR_STR(cstr) (strlen(cstr) == 0 ? Qnil : rb_str_new2(cstr))

extern VALUE rb_cOBJ;

typedef struct {
  bool result;
  std::string *warnings;
  std::vector<tinyobj::shape_t> *shapes;
  std::vector<tinyobj::material_t> *materials;
  tinyobj::attrib_t *attrib;
} obj_t;

void obj_free(void* data) {
  obj_t *obj = (obj_t *) data;
  delete obj->warnings;
  delete obj->shapes;
  delete obj->materials;
  delete obj->attrib;
  free(data);
}

size_t obj_size(const void* data) {
  return sizeof(obj_t);
}

static const rb_data_type_t obj_type = {
  .wrap_struct_name = "OBJ3D",
  .function = {
    .dmark = NULL,
    .dfree = obj_free,
    .dsize = obj_size,
    .reserved = { 0, 0 }
  },
  .parent = NULL,
  .data = NULL,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

VALUE build_rb_texopt(tinyobj::texture_option_t *opt) {
  VALUE rt = rb_hash_new();
  if (opt->colorspace.size() > 0)
    rb_hash_aset(rt, ID2SYM(rb_intern("colorspace")), rb_str_new2(opt->colorspace.c_str()));
  const char *type_str = NULL;
  switch(opt->type) {
    case tinyobj::TEXTURE_TYPE_NONE:        type_str = "none";        break;
    case tinyobj::TEXTURE_TYPE_SPHERE:      type_str = "sphere";      break;
    case tinyobj::TEXTURE_TYPE_CUBE_TOP:    type_str = "cube_top";    break;
    case tinyobj::TEXTURE_TYPE_CUBE_BOTTOM: type_str = "cube_bottom"; break;
    case tinyobj::TEXTURE_TYPE_CUBE_FRONT:  type_str = "cube_front";  break;
    case tinyobj::TEXTURE_TYPE_CUBE_BACK:   type_str = "cube_back";   break;
    case tinyobj::TEXTURE_TYPE_CUBE_LEFT:   type_str = "cube_left";   break;
    case tinyobj::TEXTURE_TYPE_CUBE_RIGHT:  type_str = "cube_right";  break;
    default:                                type_str = "unknown";
  }
  VALUE rb_origin_offset = rb_ary_new2(3),
        rb_scale         = rb_ary_new2(3),
        rb_turbulence    = rb_ary_new2(3);
  for (int i = 0; i < 3; i++) {
    rb_ary_push(rb_origin_offset, DBL2NUM(opt->origin_offset[i]));
    rb_ary_push(rb_scale,         DBL2NUM(opt->scale[i]));
    rb_ary_push(rb_turbulence,    DBL2NUM(opt->turbulence[i]));
  }
  rb_hash_aset(rt, ID2SYM(rb_intern("type")),            ID2SYM(rb_intern(type_str)));
  rb_hash_aset(rt, ID2SYM(rb_intern("sharpness")),       DBL2NUM(opt->sharpness));
  rb_hash_aset(rt, ID2SYM(rb_intern("brightness")),      DBL2NUM(opt->brightness));
  rb_hash_aset(rt, ID2SYM(rb_intern("contrast")),        DBL2NUM(opt->contrast));
  rb_hash_aset(rt, ID2SYM(rb_intern("origin_offset")),   rb_origin_offset);
  rb_hash_aset(rt, ID2SYM(rb_intern("scale")),           rb_scale);
  rb_hash_aset(rt, ID2SYM(rb_intern("turbulence")),      rb_turbulence);
  rb_hash_aset(rt, ID2SYM(rb_intern("clamp")),           opt->clamp ? Qtrue : Qfalse);
  rb_hash_aset(rt, ID2SYM(rb_intern("blendu")),          opt->blendu ? Qtrue : Qfalse);
  rb_hash_aset(rt, ID2SYM(rb_intern("blendv")),          opt->blendv ? Qtrue : Qfalse);
  rb_hash_aset(rt, ID2SYM(rb_intern("imfchan")),         rb_str_new(&opt->imfchan, 1));
  rb_hash_aset(rt, ID2SYM(rb_intern("bump_multiplier")), DBL2NUM(opt->bump_multiplier));

  return rt;
}

inline void hash_combine(std::size_t& seed, int count, const float *vals) {
  std::hash<float> hasher;
  for (int i = 0; i < count; i++)
    seed ^= hasher(vals[i]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

inline void hash_combine(std::size_t& seed, int count, const long *vals) {
  std::hash<long> hasher;
  for (int i = 0; i < count; i++)
    seed ^= hasher(vals[i]) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

extern "C" {
  VALUE rb_tinyobj_load(int argc, VALUE *argv, VALUE self) {
    VALUE fn, dir;
    rb_scan_args(argc, argv, "11", &fn, &dir);
    if (NIL_P(dir)) {
      dir = rb_funcall(rb_cFile, rb_intern("dirname"), 1, fn);
    }

    obj_t *obj = (obj_t *) malloc(sizeof(obj_t));
    memset(obj, 0, sizeof(obj_t));
    obj->warnings  = new std::string();
    obj->shapes    = new std::vector<tinyobj::shape_t>();
    obj->materials = new std::vector<tinyobj::material_t>();
    obj->attrib    = new tinyobj::attrib_t();
    std::string err;

    obj->result = tinyobj::LoadObj(obj->attrib, obj->shapes, obj->materials, obj->warnings, &err, StringValuePtr(fn), StringValuePtr(dir));
    if (!err.empty()) {
      obj_free(obj);
      rb_raise(rb_eRuntimeError, "error loading OBJ: %s", err.c_str());
    }

    return TypedData_Wrap_Struct(rb_cOBJ, &obj_type, obj);
  }

  static inline void *VAL2PTR(VALUE val) {
    if (val == Qundef || NIL_P(val)) return NULL;
    if (rb_respond_to(val, rb_intern("to_ptr"))) {
      val = rb_funcall(val, rb_intern("to_ptr"), 0);
    }
    if (NIL_P(val)) return NULL;
    return NUM2PTR(rb_funcall(val, rb_intern("to_i"), 0));
  }

  /* call-seq: fill_buffers(positions: nil, normals: nil, texcoords: nil, colors: nil, indices: nil,
   *                        vertex_stride:, index_stride:, index_type:)
   *
   * Fills one or more buffers with data. This is much faster than doing it in
   * Ruby because it avoids populating Ruby arrays with Ruby numeric types.
   * Instead everything happens at a low level, giving essentially native
   * performance for filling vertices, normals, texcoords, etc., which would
   * otherwise be an expensive operation in Ruby.
   *
   * * `positions`, `normals`, `texcoords`, `colors` and `indices` are all
   *   pointers to the memory location into which to store the data. Any of
   *   these which is `nil` is simply omitted (data is not filled). If it
   *   `responds_to?(:to_ptr)` then that method is called first, and the data
   *   will be placed into whatever memory the result of the method refers to.
   *   If it does not, then the object itself represents the buffer. In both
   *   cases, the buffer must return a memory location from #to_i. Basically,
   *   the data buffer is meant to be an instance of Fiddle::Pointer, but can
   *   be any object which satisfies these constraints.
   *
   *   Note that each object can point to different offsets into the same
   *   buffer. You don't need to use a separate allocation of memory for each
   *   argument.
   *
   *   **IMPORTANT:** If the buffer's `to_i` method does not return an
   *   appropriate memory offset, the program will have undefined behavior
   *   and will typically result in a crash.
   *
   * * `vertex_stride` represents how many bytes to 'skip over' from one
   *   vertex position, color, normal or texcoord to the next. This can be used
   *   to populate a single buffer with interleaved vertex data.
   *
   * * `index_stride` is the same as `vertex_stride`, but for vertex indices.
   *   It is separated from `vertex_stride` because index data is often not
   *   interleaved with vertex data, and often lives in its own separate
   *   buffer. Nothing prevents you from interleaving it with the vertex data,
   *   of course, by specifying the same value as you did for `vertex_stride`.
   *
   * * `index_type` represents the primitive data type of a single index. It
   *   can be one of:
   *
   *   * `:uint8`  - each index is an 8-bit unsigned integer
   *   * `:uint16` - each index is a 16-bit unsigned integer
   *   * `:uint32` - each index is a 32-bit unsigned integer
   *   * `:uint64` - each index is a 64-bit unsigned integer
   *
   * Example:
   *
   *     # In this example, we'll fill 2 buffers, one with vertex data and the
   *     # other with index data. In the vertex data we will gather positions,
   *     # normals and texture coordinates but we will omit vertex colors.
   *
   *     vertex_stride = Fiddle::SIZEOF_FLOAT * (
   *                       3 + # 3 floats for each position (x, y, z)
   *                       3 + # 3 floats for each normal (x, y, z)
   *                       2   # 2 floats for each texture coord (u, v)
   *                     )
   *     index_stride = 2 # each index will be one uint16, or two 8-bit bytes
   *     vertex_buffer = Fiddle::Pointer.malloc(obj.num_distinct_vertices * vertex_stride)
   *     index_buffer  = Fiddle::Pointer.malloc(obj.num_indices           * index_stride)
   *     obj.fill_buffers(positions:     vertex_buffer,
   *                      normals:       vertex_buffer + Fiddle::SIZEOF_FLOAT * 3,
   *                      texcoords:     vertex_buffer + Fiddle::SIZEOF_FLOAT * 6,
   *                      indices:       index_buffer,
   *                      vertex_stride: vertex_stride,
   *                      index_stride:  index_stride,
   *                      index_type:    :uint16)
   *
   *     # vertex_buffer now contains interleaved vertex data, and
   *     # index_buffer now contains index data. Print the first complete
   *     # vertex:
   *     Vertex = Fiddle::Importer.struct(['float pos[3]', 'float normal[3]', 'float texcoord[2]'])
   *     v = Vertex.new(vertex_buffer)
   *     p position: v.pos, normal: v.normal, texcoord: v.texcoord
   */
  VALUE rb_obj_fill_buffers(int argc, VALUE *argv, VALUE self) {
    #define UINT8  0
    #define UINT16 1
    #define UINT32 2
    #define UINT64 3
    static ID kwargs_ids[9];
    if (!kwargs_ids[0]) {
      kwargs_ids[0] = rb_intern_const("vertex_stride");
      kwargs_ids[1] = rb_intern_const("index_stride");
      kwargs_ids[2] = rb_intern_const("index_type");
      kwargs_ids[3] = rb_intern_const("positions");
      kwargs_ids[4] = rb_intern_const("normals");
      kwargs_ids[5] = rb_intern_const("texcoords");
      kwargs_ids[6] = rb_intern_const("colors");
      kwargs_ids[7] = rb_intern_const("indices");
      kwargs_ids[8] = rb_intern_const("flip_v");
    };
    VALUE kwargs[9] = {Qnil, Qnil, Qnil, Qnil, Qnil, Qnil, Qnil, Qnil, Qnil};
    VALUE opts = Qnil;
    rb_scan_args(argc, argv, "00:", &opts);
    rb_get_kwargs(opts, kwargs_ids, 3, 6, kwargs);

    bool flip_v     = kwargs[8] == Qundef || kwargs[8] == Qfalse || kwargs[8] == Qnil ? false : true;
    char *positions = (char *) VAL2PTR(kwargs[3]);
    char *normals   = (char *) VAL2PTR(kwargs[4]);
    char *texcoords = (char *) VAL2PTR(kwargs[5]);
    char *colors    = (char *) VAL2PTR(kwargs[6]);
    char *indices   = (char *) VAL2PTR(kwargs[7]);
    size_t vertex_stride = NUM2SIZET(kwargs[0]);
    size_t index_stride  = NUM2SIZET(kwargs[1]);
    int    index_type = -1;
    if      (SYM2ID(kwargs[2]) == rb_intern("uint8" )) index_type = UINT8;
    else if (SYM2ID(kwargs[2]) == rb_intern("uint16")) index_type = UINT16;
    else if (SYM2ID(kwargs[2]) == rb_intern("uint32")) index_type = UINT32;
    else if (SYM2ID(kwargs[2]) == rb_intern("uint64")) index_type = UINT64;
    else rb_raise(rb_eArgError, "unknown index_type");

    obj_t *obj;
    TypedData_Get_Struct(self, obj_t, &obj_type, obj);
    std::vector<tinyobj::shape_t> &shapes = *(obj->shapes);
    tinyobj::attrib_t             &attrib = *(obj->attrib);

    size_t vertex_offset = 0;
    size_t index_offset = 0;
    std::unordered_map<size_t, int64_t> vertex_map;
    for (size_t s = 0; s < shapes.size(); s++) {
      for (size_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
        size_t hash = 0;
        long vertex[] = {
          3 * shapes[s].mesh.indices[i].vertex_index   + 0,
          3 * shapes[s].mesh.indices[i].vertex_index   + 1,
          3 * shapes[s].mesh.indices[i].vertex_index   + 2,
          2 * shapes[s].mesh.indices[i].texcoord_index + 0,
          2 * shapes[s].mesh.indices[i].texcoord_index + 1,
          3 * shapes[s].mesh.indices[i].normal_index   + 0,
          3 * shapes[s].mesh.indices[i].normal_index   + 1,
          3 * shapes[s].mesh.indices[i].normal_index   + 2,
        };
        hash_combine(hash, sizeof(vertex) / sizeof(long), vertex);
        uint64_t index;
        if (vertex_map.count(hash) == 0) {
          if (positions && vertex[0] >= 0) {
            *((float *)(positions + vertex_offset                )) = attrib.vertices[vertex[0]];
            *((float *)(positions + vertex_offset+1*sizeof(float))) = attrib.vertices[vertex[1]];
            *((float *)(positions + vertex_offset+2*sizeof(float))) = attrib.vertices[vertex[2]];
          }
          if (colors && vertex[0] >= 0) {
            *((float *)(colors    + vertex_offset                )) = attrib.colors[vertex[0]];
            *((float *)(colors    + vertex_offset+1*sizeof(float))) = attrib.colors[vertex[1]];
            *((float *)(colors    + vertex_offset+2*sizeof(float))) = attrib.colors[vertex[2]];
          }
          if (texcoords && vertex[3] >= 0) {
            *((float *)(texcoords + vertex_offset                )) = attrib.texcoords[vertex[3]];
            if (flip_v)
              *((float *)(texcoords + vertex_offset+1*sizeof(float))) = 1.0f - attrib.texcoords[vertex[4]];
            else
              *((float *)(texcoords + vertex_offset+1*sizeof(float))) = attrib.texcoords[vertex[4]];
          }
          if (normals && vertex[5] >= 0) {
            *((float *)(normals   + vertex_offset                )) = attrib.normals[vertex[5]];
            *((float *)(normals   + vertex_offset+1*sizeof(float))) = attrib.normals[vertex[6]];
            *((float *)(normals   + vertex_offset+2*sizeof(float))) = attrib.normals[vertex[7]];
          }
          index = vertex_map.size();
          vertex_map[hash] = index;
          vertex_offset += vertex_stride;
        } else {
          index = vertex_map[hash];
        }
        // printf("%zu\n", index);
        if (indices) {
          switch(index_type) {
            case UINT8:  *((uint8_t  *)(indices + index_offset)) = (uint8_t)  index; break;
            case UINT16: *((uint16_t *)(indices + index_offset)) = (uint16_t) index; break;
            case UINT32: *((uint32_t *)(indices + index_offset)) = (uint32_t) index; break;
            case UINT64: *((uint64_t *)(indices + index_offset)) = (uint64_t) index; break;
            default: rb_raise(rb_eRuntimeError, "BUG: unexpected index type: %d", index_type);
          }
          index_offset += index_stride;
        }
      }
    }

    return self;
  }

  VALUE rb_obj_num_distinct_vertices(int argc, VALUE *argv, VALUE self) {
    // cache = {}
    // h[:shapes].each do |shape|
    //   shape[:mesh][:indices].each_with_index do |index, i|
    //     hash = [
    //              index[  :vertex_index] && 3.times.map { |i| 3 * index[:vertex_index]   + i },
    //              index[  :vertex_index] && 3.times.map { |i| 3 * index[:vertex_index]   + i },
    //              index[  :normal_index] && 3.times.map { |i| 3 * index[:normal_index]   + i },
    //              index[:texcoord_index] && 2.times.map { |i| 2 * index[:texcoord_index] + i }
    //            ]
    //     cache[hash] = cache.size unless cache.include?(hash)
    //   end
    // end

    obj_t *obj;
    TypedData_Get_Struct(self, obj_t, &obj_type, obj);
    std::vector<tinyobj::shape_t> &shapes = *(obj->shapes);

    size_t count = 0;
    std::unordered_map<size_t, size_t> vertex_map;
    for (size_t s = 0; s < shapes.size(); s++) {
      for (size_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
        tinyobj::index_t &index = shapes[s].mesh.indices[i];
        size_t hash = 0;
        long vertex[] = {
          3 * index.vertex_index   + 0,
          3 * index.vertex_index   + 1,
          3 * index.vertex_index   + 2,
          2 * index.texcoord_index + 0,
          2 * index.texcoord_index + 1,
          3 * index.normal_index   + 0,
          3 * index.normal_index   + 1,
          3 * index.normal_index   + 2
        };
        hash_combine(hash, sizeof(vertex) / sizeof(long), vertex);
        if (vertex_map.count(hash) == 0) {
          vertex_map[hash] = hash;
          count++;
        }
      }
    }

    return SIZET2NUM(count);
  }

  VALUE rb_obj_num_indices(int argc, VALUE *argv, VALUE self) {
    // h[:shapes].reduce(0) { |a, shape| a + shape[:mesh][:indices].size }

    obj_t *obj;
    TypedData_Get_Struct(self, obj_t, &obj_type, obj);
    std::vector<tinyobj::shape_t>    &shapes    = *(obj->shapes);

    long count = 0;
    for (size_t s = 0; s < shapes.size(); s++) {
      count += shapes[s].mesh.indices.size();
    }

    return LONG2NUM(count);
  }

  VALUE rb_obj_vertices(int argc, VALUE *argv, VALUE self) {
    obj_t *obj;
    TypedData_Get_Struct(self, obj_t, &obj_type, obj);
    tinyobj::attrib_t &attrib = *(obj->attrib);
    VALUE rb_vertices = rb_ary_new2(attrib.vertices.size());
    for (size_t i = 0; i < attrib.vertices.size(); i++)
      rb_ary_push(rb_vertices, DBL2NUM(attrib.vertices[i]));
    return rb_vertices;
  }

  VALUE rb_obj_normals(int argc, VALUE *argv, VALUE self) {
    obj_t *obj;
    TypedData_Get_Struct(self, obj_t, &obj_type, obj);
    tinyobj::attrib_t &attrib = *(obj->attrib);
    VALUE rb_normals = rb_ary_new2(attrib.normals.size());
    for (size_t i = 0; i < attrib.normals.size(); i++)
      rb_ary_push(rb_normals, DBL2NUM(attrib.normals[i]));
    return rb_normals;
  }

  VALUE rb_obj_texcoords(int argc, VALUE *argv, VALUE self) {
    obj_t *obj;
    TypedData_Get_Struct(self, obj_t, &obj_type, obj);
    tinyobj::attrib_t &attrib = *(obj->attrib);
    VALUE rb_texcoords = rb_ary_new2(attrib.texcoords.size());
    for (size_t i = 0; i < attrib.texcoords.size(); i++)
      rb_ary_push(rb_texcoords, DBL2NUM(attrib.texcoords[i]));
    return rb_texcoords;
  }

  VALUE rb_obj_colors(int argc, VALUE *argv, VALUE self) {
    obj_t *obj;
    TypedData_Get_Struct(self, obj_t, &obj_type, obj);
    tinyobj::attrib_t &attrib = *(obj->attrib);
    VALUE rb_colors = rb_ary_new2(attrib.colors.size());
    for (size_t i = 0; i < attrib.colors.size(); i++)
      rb_ary_push(rb_colors, DBL2NUM(attrib.colors[i]));
    return rb_colors;
  }

  VALUE rb_obj_to_hash(VALUE self) {
    obj_t *obj;
    TypedData_Get_Struct(self, obj_t, &obj_type, obj);
    std::string                      &warnings  = *(obj->warnings);
    std::vector<tinyobj::shape_t>    &shapes    = *(obj->shapes);
    std::vector<tinyobj::material_t> &materials = *(obj->materials);

    VALUE ret = rb_hash_new();
    rb_hash_aset(ret, ID2SYM(rb_intern("success")), obj->result ? Qtrue : Qfalse);

    if (warnings.size() > 0)
      rb_hash_aset(ret, ID2SYM(rb_intern("warnings")), rb_str_new2(warnings.c_str()));

    VALUE rb_materials = rb_ary_new2(materials.size());
    rb_hash_aset(ret, ID2SYM(rb_intern("materials")), rb_materials);
    for (size_t m = 0; m < materials.size(); m++) {
      VALUE rb_material = rb_hash_new();
      rb_ary_push(rb_materials, rb_material);
      VALUE rb_ambient       = rb_ary_new2(3),
            rb_diffuse       = rb_ary_new2(3),
            rb_specular      = rb_ary_new2(3),
            rb_transmittance = rb_ary_new2(3),
            rb_emission      = rb_ary_new2(3);
      rb_hash_aset(rb_material, ID2SYM(rb_intern("ambient")),                    rb_ambient);
      rb_hash_aset(rb_material, ID2SYM(rb_intern("diffuse")),                    rb_diffuse);
      rb_hash_aset(rb_material, ID2SYM(rb_intern("specular")),                   rb_specular);
      rb_hash_aset(rb_material, ID2SYM(rb_intern("transmittance")),              rb_transmittance);
      rb_hash_aset(rb_material, ID2SYM(rb_intern("emission")),                   rb_emission);
      rb_hash_aset(rb_material, ID2SYM(rb_intern("name")),                       NIL_OR_STR(materials[m].name.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("shininess")),                  DBL2NUM(materials[m].shininess));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("ior")),                        DBL2NUM(materials[m].ior));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("dissolve")),                   DBL2NUM(materials[m].dissolve));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("illum")),                      DBL2NUM(materials[m].illum));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("ambient_texname")),            NIL_OR_STR(materials[m].ambient_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("diffuse_texname")),            NIL_OR_STR(materials[m].diffuse_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("specular_texname")),           NIL_OR_STR(materials[m].specular_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("specular_highlight_texname")), NIL_OR_STR(materials[m].specular_highlight_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("bump_texname")),               NIL_OR_STR(materials[m].bump_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("displacement_texname")),       NIL_OR_STR(materials[m].displacement_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("alpha_texname")),              NIL_OR_STR(materials[m].alpha_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("reflection_texname")),         NIL_OR_STR(materials[m].reflection_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("roughness")),                  DBL2NUM(materials[m].roughness));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("metallic")),                   DBL2NUM(materials[m].metallic));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("sheen")),                      DBL2NUM(materials[m].sheen));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("clearcoat_thickness")),        DBL2NUM(materials[m].clearcoat_thickness));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("clearcoat_roughness")),        DBL2NUM(materials[m].clearcoat_roughness));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("anisotropy")),                 DBL2NUM(materials[m].anisotropy));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("anisotropy_rotation")),        DBL2NUM(materials[m].anisotropy_rotation));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("roughness_texname")),          NIL_OR_STR(materials[m].roughness_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("metallic_texname")),           NIL_OR_STR(materials[m].metallic_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("sheen_texname")),              NIL_OR_STR(materials[m].sheen_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("emissive_texname")),           NIL_OR_STR(materials[m].emissive_texname.c_str()));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("normal_texname")),             NIL_OR_STR(materials[m].normal_texname.c_str()));
      for (int i = 0; i < 3; i++) {
        rb_ary_push(rb_ambient,       DBL2NUM(materials[m].ambient[i]));
        rb_ary_push(rb_diffuse,       DBL2NUM(materials[m].diffuse[i]));
        rb_ary_push(rb_specular,      DBL2NUM(materials[m].specular[i]));
        rb_ary_push(rb_transmittance, DBL2NUM(materials[m].transmittance[i]));
        rb_ary_push(rb_emission,      DBL2NUM(materials[m].emission[i]));
      }
      rb_hash_aset(rb_material, ID2SYM(rb_intern("ambient_texopt")),            build_rb_texopt(&materials[m].ambient_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("diffuse_texopt")),            build_rb_texopt(&materials[m].diffuse_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("specular_texopt")),           build_rb_texopt(&materials[m].specular_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("specular_highlight_texopt")), build_rb_texopt(&materials[m].specular_highlight_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("bump_texopt")),               build_rb_texopt(&materials[m].bump_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("displacement_texopt")),       build_rb_texopt(&materials[m].displacement_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("alpha_texopt")),              build_rb_texopt(&materials[m].alpha_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("reflection_texopt")),         build_rb_texopt(&materials[m].reflection_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("roughness_texopt")),          build_rb_texopt(&materials[m].roughness_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("metallic_texopt")),           build_rb_texopt(&materials[m].metallic_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("sheen_texopt")),              build_rb_texopt(&materials[m].sheen_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("emissive_texopt")),           build_rb_texopt(&materials[m].emissive_texopt));
      rb_hash_aset(rb_material, ID2SYM(rb_intern("normal_texopt")),             build_rb_texopt(&materials[m].normal_texopt));
      VALUE rb_unknown_parameter = Qnil;
      typedef std::map<std::string, std::string>::iterator it_type;
      for (it_type iterator = materials[m].unknown_parameter.begin(); iterator != materials[m].unknown_parameter.end(); iterator++) {
        if (NIL_P(rb_unknown_parameter)) rb_unknown_parameter = rb_hash_new();
        rb_hash_aset(rb_unknown_parameter, rb_str_new2(iterator->first.c_str()), rb_str_new2(iterator->second.c_str()));
      }
      if (!NIL_P(rb_unknown_parameter))
        rb_hash_aset(rb_material, ID2SYM(rb_intern("unknown_parameters")), rb_unknown_parameter);
    }

    rb_hash_aset(ret, ID2SYM(rb_intern("vertices")),  rb_funcall(self, rb_intern("vertices"),  0));
    rb_hash_aset(ret, ID2SYM(rb_intern("normals")),   rb_funcall(self, rb_intern("normals"),   0));
    rb_hash_aset(ret, ID2SYM(rb_intern("texcoords")), rb_funcall(self, rb_intern("texcoords"), 0));
    rb_hash_aset(ret, ID2SYM(rb_intern("colors")),    rb_funcall(self, rb_intern("colors"),    0));

    VALUE rb_shapes = rb_ary_new2(shapes.size());
    rb_hash_aset(ret, ID2SYM(rb_intern("shapes")), rb_shapes);
    for (size_t s = 0; s < shapes.size(); s++) {
      VALUE rb_shape = rb_hash_new();
      VALUE rb_mesh  = rb_hash_new();
      VALUE rb_path  = rb_hash_new();
      rb_ary_push(rb_shapes, rb_shape);
      rb_hash_aset(rb_shape, ID2SYM(rb_intern("name")), rb_str_new2(shapes[s].name.c_str()));
      rb_hash_aset(rb_shape, ID2SYM(rb_intern("mesh")), rb_mesh);
      rb_hash_aset(rb_shape, ID2SYM(rb_intern("path")), rb_path);

      VALUE rb_path_indices = rb_ary_new2(shapes[s].path.indices.size());
      rb_hash_aset(rb_shape, ID2SYM(rb_intern("indices")), rb_path_indices);
      for (size_t i = 0; i < shapes[s].path.indices.size(); i++) {
        rb_ary_push(rb_path_indices, INT2NUM(shapes[s].path.indices[i]));
      }

      VALUE rb_mesh_indices = rb_ary_new2(shapes[s].mesh.indices.size());
      rb_hash_aset(rb_mesh, ID2SYM(rb_intern("indices")), rb_mesh_indices);
      for (size_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
        VALUE index = rb_hash_new();
        if (shapes[s].mesh.indices[i].vertex_index != -1)
          rb_hash_aset(index, ID2SYM(rb_intern("vertex_index")),   INT2NUM(shapes[s].mesh.indices[i].vertex_index));
        if (shapes[s].mesh.indices[i].normal_index != -1)
          rb_hash_aset(index, ID2SYM(rb_intern("normal_index")),   INT2NUM(shapes[s].mesh.indices[i].normal_index));
        if (shapes[s].mesh.indices[i].texcoord_index != -1)
          rb_hash_aset(index, ID2SYM(rb_intern("texcoord_index")), INT2NUM(shapes[s].mesh.indices[i].texcoord_index));
        rb_ary_push(rb_mesh_indices, index);
      }

      VALUE rb_mesh_num_face_vertices = rb_ary_new2(shapes[s].mesh.num_face_vertices.size());
      rb_hash_aset(rb_mesh, ID2SYM(rb_intern("num_face_vertices")), rb_mesh_num_face_vertices);
      for (size_t i = 0; i < shapes[s].mesh.num_face_vertices.size(); i++) {
        int v = (int) shapes[s].mesh.num_face_vertices[i];
        rb_ary_push(rb_mesh_num_face_vertices, INT2NUM(v));
      }

      VALUE rb_mesh_material_ids = rb_ary_new2(shapes[s].mesh.material_ids.size());
      rb_hash_aset(rb_mesh, ID2SYM(rb_intern("material_ids")), rb_mesh_material_ids);
      for (size_t i = 0; i < shapes[s].mesh.material_ids.size(); i++) {
        int v = (int) shapes[s].mesh.material_ids[i];
        rb_ary_push(rb_mesh_material_ids, INT2NUM(v));
      }

      VALUE rb_mesh_smoothing_group_ids = rb_ary_new2(shapes[s].mesh.smoothing_group_ids.size());
      rb_hash_aset(rb_mesh, ID2SYM(rb_intern("smoothing_group_ids")), rb_mesh_smoothing_group_ids);
      for (size_t i = 0; i < shapes[s].mesh.smoothing_group_ids.size(); i++) {
        long v = (long) shapes[s].mesh.smoothing_group_ids[i];
        rb_ary_push(rb_mesh_smoothing_group_ids, v == 0 ? Qnil : LONG2NUM(v));
      }

      VALUE rb_mesh_tags = rb_ary_new2(shapes[s].mesh.tags.size());
      rb_hash_aset(rb_mesh, ID2SYM(rb_intern("tags")), rb_mesh_tags);
      for (size_t i = 0; i < shapes[s].mesh.tags.size(); i++) {
        VALUE rb_tag = rb_hash_new();
        rb_ary_push(rb_mesh_tags, rb_tag);
        rb_hash_aset(rb_tag, ID2SYM(rb_intern("name")), rb_str_new2(shapes[s].mesh.tags[i].name.c_str()));
        VALUE rb_int_values = rb_ary_new2(shapes[s].mesh.tags[i].intValues.size());
        rb_hash_aset(rb_tag, ID2SYM(rb_intern("int_values")), rb_int_values);
        for (size_t j = 0; j < shapes[s].mesh.tags[i].intValues.size(); j++) {
          rb_ary_push(rb_int_values, INT2NUM(shapes[s].mesh.tags[i].intValues[j]));
        }
        VALUE rb_float_values = rb_ary_new2(shapes[s].mesh.tags[i].floatValues.size());
        rb_hash_aset(rb_tag, ID2SYM(rb_intern("float_values")), rb_float_values);
        for (size_t j = 0; j < shapes[s].mesh.tags[i].floatValues.size(); j++) {
          rb_ary_push(rb_float_values, INT2NUM(shapes[s].mesh.tags[i].floatValues[j]));
        }
        VALUE rb_string_values = rb_ary_new2(shapes[s].mesh.tags[i].stringValues.size());
        rb_hash_aset(rb_tag, ID2SYM(rb_intern("string_values")), rb_string_values);
        for (size_t j = 0; j < shapes[s].mesh.tags[i].stringValues.size(); j++) {
          rb_ary_push(rb_string_values, rb_str_new2(shapes[s].mesh.tags[i].stringValues[j].c_str()));
        }
      }
    }

    return ret;
  }
}
