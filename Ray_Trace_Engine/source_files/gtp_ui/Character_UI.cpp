#include "Character_UI.hpp"

gtp::Character_UI::Character_UI() {}

void gtp::Character_UI::SetSaveOpenFlag(bool saveOpenFlag) {
  this->saveMenuData.saveOpen = true;
}

void gtp::Character_UI::SetLoadOpenFlag(bool loadOpenFlag) {
  this->loadMenuData.loadOpen = true;
}

void gtp::Character_UI::SetCreateOpenFlag(bool createOpenFlag) {
  this->createMenuData.createOpen = createOpenFlag;
}

std::string gtp::Character_UI::GetLoadPath() {
  return this->loadMenuData.loadFilePath;
}

bool gtp::Character_UI::GetLoadCharacterFlag() { return this->loadCharacter; }

void gtp::Character_UI::HandleSave() {

  if (this->saveMenuData.saveOpen) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Save File",
                                            ".json", config);

    // choose file window
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        this->saveMenuData.saveFilePath =
            ImGuiFileDialog::Instance()->GetFilePathName();
        std::cout << "saved file path: " << this->saveMenuData.saveFilePath
                  << '\n';
      }

      // close
      this->saveMenuData.saveOpen = false;
      ImGuiFileDialog::Instance()->Close();
    }
  }
}

void gtp::Character_UI::HandleLoad() {
  if (this->loadMenuData.loadOpen) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Load Player",
                                            ".json", config);

    // choose file window
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        this->loadMenuData.loadFilePath =
            ImGuiFileDialog::Instance()->GetFilePathName();
        // this->rendererData.loadPlayer = true;
        this->loadCharacter = true;
        // std::cout << "load file path: " << this->loadMenuData.loadFilePath
        //   << '\n';
      }

      // close
      this->loadMenuData.loadOpen = false;
      ImGuiFileDialog::Instance()->Close();
    }
  }
}

void gtp::Character_UI::HandleCreate() {
  if (this->createMenuData.createOpen) {
    ImGui::Begin("Create Character Menu", &this->createMenuData.createOpen);
    ImGui::Text("where is this text going to appear?");
    ImGui::End();
  }
}
