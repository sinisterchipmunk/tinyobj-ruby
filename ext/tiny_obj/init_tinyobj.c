#include "rb_tinyobj.h"

VALUE rb_mTinyOBJ;
VALUE rb_cOBJ;

void Init_tiny_obj(void) {
  rb_mTinyOBJ = rb_define_module("TinyOBJ");
  rb_cOBJ = rb_define_class_under(rb_mTinyOBJ, "OBJ", rb_cObject);
  rb_define_method(rb_cOBJ, "to_hash",               rb_obj_to_hash,                0);
  rb_define_method(rb_cOBJ, "num_indices",           rb_obj_num_indices,           -1);
  rb_define_method(rb_cOBJ, "num_distinct_vertices", rb_obj_num_distinct_vertices, -1);
  rb_define_method(rb_cOBJ, "fill_buffers",          rb_obj_fill_buffers,          -1);
  rb_define_method(rb_cOBJ, "vertices",              rb_obj_vertices,              -1);
  rb_define_method(rb_cOBJ, "normals",               rb_obj_normals,               -1);
  rb_define_method(rb_cOBJ, "texcoords",             rb_obj_texcoords,             -1);
  rb_define_method(rb_cOBJ, "colors",                rb_obj_colors,                -1);
  rb_define_method(rb_cOBJ, "materials",             rb_obj_materials,              0);
  rb_define_singleton_method(rb_mTinyOBJ, "load", rb_tinyobj_load, -1);
}
