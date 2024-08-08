#include "Player.hpp"

void gtp::Player::CreatePlayer(std::string newSaveFilePath,
                               std::string newModelFilePath) {
  this->modelFilePath = newModelFilePath;
  this->savePath = newSaveFilePath;
  std::cout << "player.hpp create player function: save file path: "
            << this->savePath << std::endl;
  // this->SaveModelFilePath(saveFilePath);
}

void gtp::Player::SaveModelFilePath(const std::string &filename) const {
  json j;
  j["modelFilePath"] = modelFilePath;

  std::ofstream file(filename);
  if (file.is_open()) {
    file << j.dump(4); // Pretty print with 4 spaces
    file.close();
    std::cout << "Saved Character: model file path '" << modelFilePath
              << "'  to: " << filename << '\n';
  } else {
    throw std::runtime_error("Unable to open file for writing: " + filename);
  }
}

void gtp::Player::LoadModelFilePath(const std::string &filename) {
  std::ifstream file(filename);
  if (file.is_open()) {
    json j;
    file >> j;
    file.close();
    modelFilePath = j.value("modelFilePath", "none");
    std::cout << "Loaded model file path '" << modelFilePath
              << "'  from: " << filename << '\n';
  } else {
    // If the file cannot be opened, keep modelFilePath as "none"
    modelFilePath = "none";
    std::cout << "Default model file path is set (no file found: " << filename
              << ")\n";
  }
}

gtp::Player::Player() {}

gtp::Player::Player(EngineCore *engineCorePtr) : pEngineCore(engineCorePtr) {}
