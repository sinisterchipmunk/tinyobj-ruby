#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_DOUBLE
#include "rb_tinyobj.h"
#include <stdlib.h>

#define NIL_OR_STR(cstr) (strlen(cstr) > 0 ? Qnil : rb_str_new2(cstr))

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

extern "C" {
  VALUE rb_tinyobj_load(int argc, VALUE *argv, VALUE self) {
    VALUE fn, dir;
    rb_scan_args(argc, argv, "11", &fn, &dir);
    if (NIL_P(dir)) {
      dir = rb_funcall(rb_cFile, rb_intern("dirname"), 1, fn);
    }

    VALUE ret = rb_hash_new();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    bool result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, StringValuePtr(fn), StringValuePtr(dir));

    if (!err.empty())
      rb_raise(rb_eRuntimeError, "error loading OBJ: %s", err.c_str());

    rb_hash_aset(ret, ID2SYM(rb_intern("success")), result ? Qtrue : Qfalse);

    if (!warn.empty())
      rb_hash_aset(ret, ID2SYM(rb_intern("warnings")), rb_str_new2(warn.c_str()));

    VALUE rb_materials = rb_ary_new2(materials.size());
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

    VALUE rb_vertices  = rb_ary_new2(attrib.vertices.size()),
          rb_normals   = rb_ary_new2(attrib.normals.size()),
          rb_texcoords = rb_ary_new2(attrib.texcoords.size()),
          rb_colors    = rb_ary_new2(attrib.colors.size());
    rb_hash_aset(ret, ID2SYM(rb_intern("vertices")),  rb_vertices);
    rb_hash_aset(ret, ID2SYM(rb_intern("normals")),   rb_normals);
    rb_hash_aset(ret, ID2SYM(rb_intern("texcoords")), rb_texcoords);
    rb_hash_aset(ret, ID2SYM(rb_intern("colors")),    rb_colors);
    for (size_t i = 0; i < attrib.vertices.size();  i++)
      rb_ary_push(rb_vertices,  DBL2NUM(attrib.vertices[i]));
    for (size_t i = 0; i < attrib.normals.size();   i++)
      rb_ary_push(rb_normals,   DBL2NUM(attrib.normals[i]));
    for (size_t i = 0; i < attrib.texcoords.size(); i++)
      rb_ary_push(rb_texcoords, DBL2NUM(attrib.texcoords[i]));
    for (size_t i = 0; i < attrib.colors.size();    i++)
      rb_ary_push(rb_colors,    DBL2NUM(attrib.colors[i]));

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
