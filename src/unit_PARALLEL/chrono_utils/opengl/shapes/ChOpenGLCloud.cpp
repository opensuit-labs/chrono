// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Generic renderable point cloud. Based on code provided by Perry Kivolowitz.
// Authors: Hammad Mazhar
// =============================================================================

#include <iostream>
#include "ChOpenGLCloud.h"

using namespace std;
using namespace glm;
using namespace chrono::utils;

ChOpenGLCloud::ChOpenGLCloud()
      :
        ChOpenGLObject() {
}

bool ChOpenGLCloud::Initialize(
      const std::vector<glm::vec3>& data) {
   if (this->GLReturnedError("Background::Initialize - on entry"))
      return false;

   if (!super::Initialize()) {
      //   return false;
   }

   this->vertices.resize(1);
   this->vertices[0] = vec3(0, 0, 0);
   this->vertex_indices.resize(1);
   this->vertex_indices[0] = 0;
//   this->vertices = data;
//   this->vertex_indices.resize(data.size());
//   for (int i = 0; i < data.size(); i++) {
//      this->vertex_indices[i] = i;
//   }

   glGenVertexArrays(1, &vertex_array_handle);
   glBindVertexArray(vertex_array_handle);

   glGenBuffers(1, &vertex_data_handle);
   glBindBuffer(GL_ARRAY_BUFFER, vertex_data_handle);
   glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(vec3), &this->vertices[0], GL_DYNAMIC_DRAW);

   glGenBuffers(1, &vertex_element_handle);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_element_handle);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertex_indices.size() * sizeof(GLuint), &vertex_indices[0], GL_DYNAMIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);  //Position

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   if (this->GLReturnedError("Cloud::Initialize - on exit"))
      return false;

   return true;
}
void ChOpenGLCloud::Update(
      const std::vector<glm::vec3>& data) {

   this->vertices = data;
   this->vertex_indices.resize(data.size());
   for (int i = 0; i < data.size(); i++) {
      this->vertex_indices[i] = i;
   }
   this->GLReturnedError("ChOpenGLCloud::Draw - after update");
}
void ChOpenGLCloud::TakeDown() {
   vertices.clear();
   super::TakeDown();
}

void ChOpenGLCloud::Draw(
      const mat4 & projection,
      const mat4 & modelview) {
   if (this->GLReturnedError("ChOpenGLCloud::Draw - on entry"))
      return;
   glEnable(GL_DEPTH_TEST);
   //compute the mvp matrix and normal matricies
   mat4 mvp = projection * modelview;
   mat3 nm = inverse(transpose(mat3(modelview)));

   //Enable the shader
   shader->Use();
   this->GLReturnedError("ChOpenGLCloud::Draw - after use");
   //Send our common uniforms to the shader
   shader->CommonSetup(value_ptr(projection), value_ptr(modelview), value_ptr(mvp), value_ptr(nm));

   this->GLReturnedError("ChOpenGLCloud::Draw - after common setup");
   //Bind and draw! (in this case we draw as triangles)
   glBindVertexArray(this->vertex_array_handle);

   glBindBuffer(GL_ARRAY_BUFFER, vertex_data_handle);
   glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(vec3), &this->vertices[0], GL_DYNAMIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_element_handle);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertex_indices.size() * sizeof(GLuint), &vertex_indices[0], GL_DYNAMIC_DRAW);

   this->GLReturnedError("ChOpenGLCloud::Draw - after bind");
   glDrawElements(GL_POINTS, this->vertex_indices.size(), GL_UNSIGNED_INT, (void*) 0);

   this->GLReturnedError("ChOpenGLCloud::Draw - after draw");
   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glUseProgram(0);

   if (this->GLReturnedError("ChOpenGLCloud::Draw - on exit"))
      return;
}
