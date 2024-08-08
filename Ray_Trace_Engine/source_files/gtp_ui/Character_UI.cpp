#include "Character_UI.hpp"

gtp::Character_UI::Character_UI() {}

void gtp::Character_UI::SetSaveOpenFlag(bool saveOpenFlag) {
  this->saveMenuData.saveOpen = saveOpenFlag;
}

void gtp::Character_UI::SetLoadOpenFlag(bool loadOpenFlag) {
  this->loadMenuData.loadOpen = loadOpenFlag;
}

void gtp::Character_UI::SetCreateOpenFlag(bool createOpenFlag) {
  this->createMenuData.createOpen = createOpenFlag;
}

void gtp::Character_UI::SetCreateReadyFlag(bool createReadyFlag) {
  this->createMenuData.createReady = createReadyFlag;
}

std::string gtp::Character_UI::GetCreateCharacterModelFilePath() {
  return this->createMenuData.modelFilePath;
}

std::string gtp::Character_UI::GetCreateCharacterSavePath() {
  return this->createMenuData.characterSavePath;
}

std::string gtp::Character_UI::GetCharacterLoadPath() {
  return this->loadMenuData.loadFilePath;
}

bool gtp::Character_UI::GetLoadCharacterFlag() { return this->loadCharacter; }

bool gtp::Character_UI::GetCreateCharacterFlag() {
  return this->createMenuData.createReady;
}

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
  // if "create character" from header drop down menu selected
  if (this->createMenuData.createOpen) {
    /* begin menu */
    ImGui::Begin("Create Character Menu", &this->createMenuData.createOpen);
    // output character name from the input text box below
    ImGui::Text("Character Name: %s",
                this->createMenuData.characterName.data());
    // change input text bg to blue
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.3f, 0.4f, 1.0f));
    // character name input text box
    ImGui::InputText(" ", this->createMenuData.input_text,
                     IM_ARRAYSIZE(this->createMenuData.input_text));
    // change input text bg color back to settings color
    ImGui::PopStyleColor();

    // assign entered chars to character name string
    this->createMenuData.characterName =
        std::string(this->createMenuData.input_text);

    // select model file button
    if (ImGui::Button("Select File")) {
      IGFD::FileDialogConfig config;
      config.path = ".";
      ImGuiFileDialog::Instance()->OpenDialog(
          "ChooseFileDlgKey", "Select Model File", ".gltf", config);
    }

    // output filepath for debug while building this system
    ImGui::Text("Model File Path: %s",
                this->createMenuData.modelFilePath.data());

    // select model file dialogue window
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        // assign selected model file path to create menu data struct variable
        // 'modelFilPath'
        this->createMenuData.modelFilePath =
            ImGuiFileDialog::Instance()->GetFilePathName();
      }

      // close select model file dialogue window
      ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Save and Continue")) {
      std::filesystem::path current_path = std::filesystem::current_path();
      std::string pathToCharacterSaveFolder =
          current_path.string() + "/character_save/";
      this->createMenuData.characterSavePath =
          pathToCharacterSaveFolder + this->createMenuData.characterName +
          ".json";
      std::cout << "this->createMenuData.characterSavePath: "
                << this->createMenuData.characterSavePath << std::endl;
      // this->createMenuData.characterSavePath =
      this->createMenuData.createReady = true;
    }

    /* end menu */
    ImGui::End();
  }
}
