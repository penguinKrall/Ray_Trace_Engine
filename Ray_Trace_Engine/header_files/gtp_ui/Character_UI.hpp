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
    bool createOpen = false;
    std::string modelFilePath = "none";
  };
  CreateMenuData createMenuData{};

public:
  explicit Character_UI();

  void SetSaveOpenFlag(bool saveOpenFlag);
  void SetLoadOpenFlag(bool loadOpenFlag);
  void SetCreateOpenFlag(bool createOpenFlag);
  std::string GetLoadPath();
  bool GetLoadCharacterFlag();

  void CreateCharacter();

  // -- save menu for character dropdown selection
  void HandleSave();

  // -- load menu
  void HandleLoad();

  // -- handle create
  void HandleCreate();

};
} // namespace gtp
