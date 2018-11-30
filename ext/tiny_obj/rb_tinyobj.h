#ifndef RB_TINYOBJ_H
#define RB_TINYOBJ_H 1

#include "ruby.h"

#ifdef __cplusplus
#include "tiny_obj_loader.h"
extern "C" {
#endif
  VALUE rb_tinyobj_load(int argc, VALUE *argv, VALUE self);
  VALUE rb_obj_to_hash(VALUE self);
  VALUE rb_obj_num_distinct_vertices(int argc, VALUE *argv, VALUE self);
  VALUE rb_obj_num_indices(int argc, VALUE *argv, VALUE self);
  VALUE rb_obj_fill_buffers(int argc, VALUE *argv, VALUE self);
  VALUE rb_obj_vertices(int argc, VALUE *argv, VALUE self);
  VALUE rb_obj_normals(int argc, VALUE *argv, VALUE self);
  VALUE rb_obj_texcoords(int argc, VALUE *argv, VALUE self);
  VALUE rb_obj_colors(int argc, VALUE *argv, VALUE self);
  VALUE rb_obj_materials(VALUE self);
#ifdef __cplusplus
}
#endif

#endif /* RB_TINYOBJ_H */
