#pragma once

#include <glTFModel.hpp>
#include <json.hpp>

using json = nlohmann::json;

namespace gtp {

class Player {
private:
  // core pointer
  EngineCore *pEngineCore = nullptr;

  // character model
  gtp::Model *defaultModel = nullptr;

  // positiion for character model
  glm::vec4 characterPosition = glm::vec4(0.0f);

  void CreatePlayer(std::string filePath);

public:
  std::string modelFilePath = "none";
  // save modelFilePath to a JSON file
  void SaveModelFilePath(const std::string &filename) const;

  // load modelFilePath from a JSON file
  void LoadModelFilePath(const std::string &filename);
  // default constructor
  explicit Player();

  // create constructor
  explicit Player(EngineCore *engineCorePtr = nullptr);
};

} // namespace gtp
