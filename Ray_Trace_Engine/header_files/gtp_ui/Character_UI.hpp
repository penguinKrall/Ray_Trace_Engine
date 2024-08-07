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

public:
  explicit Character_UI();

  void SetSaveOpenFlag(bool saveOpenFlag);
  void SetLoadOpenFlag(bool loadOpenFlag);
  std::string GetLoadPath();
  bool GetLoadCharacterFlag();

  // -- save menu for character dropdown selection
  void HandleSave();

  // -- load menu
  void HandleLoad();

};
} // namespace gtp
