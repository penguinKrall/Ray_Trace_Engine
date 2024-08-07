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
  // if "create character" from header drop down menu selected
  if (this->createMenuData.createOpen) {
    ImGui::Begin("Create Character Menu", &this->createMenuData.createOpen);
    // output character name from the input text box below
    ImGui::Text("Character Name: %s",
                this->createMenuData.characterName.data());
    ImGui::PushStyleColor(
        ImGuiCol_FrameBg,
        ImVec4(0.2f, 0.3f, 0.4f, 1.0f)); // change color to blue
    // character name input text box
    ImGui::InputText(" ", this->createMenuData.input_text,
                     IM_ARRAYSIZE(this->createMenuData.input_text));
    ImGui::PopStyleColor(); // change color back to settings color

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
        this->createMenuData.modelFilePath =
            ImGuiFileDialog::Instance()->GetFilePathName();
        // this->loadModelFlags.loadModelName =
        //   ImGuiFileDialog::Instance()->GetFilePathName();
        //  this->modelData.loadModel = true;
      }

      // close select model file dialogue window
      ImGuiFileDialog::Instance()->Close();
    }
    ImGui::End();
  }
}
