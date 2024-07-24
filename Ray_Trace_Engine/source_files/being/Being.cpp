#include "Being.hpp"

void gtp::Being::CreateBeing(std::string filePath) {
  this->modelFilePath = filePath;
}

void gtp::Being::SaveModelFilePath(const std::string& filename) const {
  json j;
  j["modelFilePath"] = modelFilePath;

  std::ofstream file(filename);
  if (file.is_open()) {
    file << j.dump(4); // Pretty print with 4 spaces
    file.close();
  }
  else {
    throw std::runtime_error("Unable to open file for writing: " + filename);
  }
}

void gtp::Being::LoadModelFilePath(const std::string& filename) {
  std::ifstream file(filename);
  if (file.is_open()) {
    json j;
    file >> j;
    file.close();
    modelFilePath = j.at("modelFilePath").get<std::string>();
  }
  else {
    throw std::runtime_error("Unable to open file for reading: " + filename);
  }
}

gtp::Being::Being() {}

gtp::Being::Being(EngineCore *engineCorePtr) : pEngineCore(engineCorePtr) {}
