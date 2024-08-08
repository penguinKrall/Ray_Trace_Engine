#pragma once
#include <Utilities_UI.hpp>

namespace gtp {

class Character_UI {
private:
  bool loadCharacter = false;

  struct SaveMenuData {
    bool saveOpen = false;
    std::string saveFilePath = "none";
  };
  SaveMenuData saveMenuData{};

  struct LoadMenuData {
    bool loadOpen = false;
    std::string loadFilePath = "none";
  };
  LoadMenuData loadMenuData{};

  struct CreateMenuData {
    bool createReady = false;
    bool createOpen = false;
    char input_text[128] = "";
    std::string modelFilePath = "none";
    std::string characterSavePath = "none";
    std::string characterName = "none";
  };
  CreateMenuData createMenuData{};

public:
  explicit Character_UI();

  void SetSaveOpenFlag(bool saveOpenFlag);
  void SetLoadOpenFlag(bool loadOpenFlag);
  void SetCreateOpenFlag(bool createOpenFlag);
  void SetCreateReadyFlag(bool createReadyFlag);

  std::string GetCreateCharacterModelFilePath();
  std::string GetCreateCharacterSavePath();

  std::string GetCharacterLoadPath();
  bool GetLoadCharacterFlag();
  bool GetCreateCharacterFlag();

  // void CreateCharacter();

  // -- save menu for character dropdown selection
  void HandleSave();

  // -- load menu
  void HandleLoad();

  // -- handle create
  void HandleCreate();
};
} // namespace gtp
