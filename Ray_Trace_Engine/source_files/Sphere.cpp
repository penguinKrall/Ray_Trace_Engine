#include "Sphere.hpp"

Sphere::Sphere() {}

Sphere::Sphere(EngineCore *engineCorePtr) { InitSphere(engineCorePtr); }

void Sphere::InitSphere(EngineCore *engineCorePtr) {
  this->pEngineCore = engineCorePtr;
}

void Sphere::InitSphereGeometry() {
  const unsigned int X_SEGMENTS = 50;
  const unsigned int Y_SEGMENTS = 50;
  const float PI = 3.14159265359f;

  for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
    for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
      float xSegment = (float)x / (float)X_SEGMENTS;
      float ySegment = (float)y / (float)Y_SEGMENTS;
      float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
      float yPos = std::cos(ySegment * PI);
      float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

      gtp::Model::Vertex vertex;
      vertex.pos = {xPos, yPos, zPos, 1.0f};
      vertex.normal = {xPos, yPos, zPos, 0.0f};
      vertex.uv0 = {xSegment, ySegment};
      vertex.uv1 = {0.0f, 0.0f};
      vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
      vertex.joint0 = {0.0f, 0.0f, 0.0f, 0.0f};
      vertex.weight0 = {0.0f, 0.0f, 0.0f, 0.0f};
      vertex.tangent = {0.0f, 0.0f, 0.0f, 0.0f};
      vertices.push_back(vertex);
    }
  }

  for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
    for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
      indices.push_back(y * (X_SEGMENTS + 1) + x);
      indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
      indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);

      indices.push_back(y * (X_SEGMENTS + 1) + x);
      indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
      indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
    }
  }
}
