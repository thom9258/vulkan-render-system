#!/bin/bash

SHADER_SOURCE_DIR="shaders"
RESOURCES_DIR="compiled_shaders"

[[ -d ${RESOURCES_DIR} ]] || mkdir ${RESOURCES_DIR}

function compile_vert_frag ()
{
  glslc ${SHADER_SOURCE_DIR}/"$1".vert -o ${RESOURCES_DIR}/"$1".vert.spv
  echo "compiled ${SHADER_SOURCE_DIR}/""$1"".vert to ${RESOURCES_DIR}/""$1"".vert.spv"
  glslc ${SHADER_SOURCE_DIR}/"$1".frag -o ${RESOURCES_DIR}/"$1".frag.spv
  echo "compiled ${SHADER_SOURCE_DIR}/""$1"".frag to ${RESOURCES_DIR}/""$1"".frag.spv"
}

compile_vert_frag "NormColor"
compile_vert_frag "Wireframe"
compile_vert_frag "Diffuse"
compile_vert_frag "Material"
