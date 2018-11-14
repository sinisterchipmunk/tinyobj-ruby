#include "ruby.h"

VALUE rb_mTinyOBJ;

VALUE rb_tinyobj_load(int argc, VALUE *argv, VALUE self);

void Init_tiny_obj(void) {
  rb_mTinyOBJ = rb_define_module("TinyOBJ");
  rb_define_singleton_method(rb_mTinyOBJ, "load", rb_tinyobj_load, -1);
}
