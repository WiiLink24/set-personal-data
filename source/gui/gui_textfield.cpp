/****************************************************************************
 * libwiigui
 *
 * Spotlight 2021
 *
 * gui_textfield.cpp
 *
 * GUI class definitions
 ***************************************************************************/

#include "gui.h"

/**
 * Constructor for the GuiTextField class.
 */

GuiTextField::GuiTextField(std::wstring content, u32 max) {
    width = 540;
    height = 400;
    selectable = true;
    focus = 0; // allow focus
    alignmentHor = ALIGN_CENTRE;
    alignmentVert = ALIGN_MIDDLE;
    max_len = max;

    keyTextbox = new GuiImageData(keyboard_textbox_png);
    keyTextboxImg = new GuiImage(keyTextbox);
    keyTextboxImg->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    keyTextboxImg->SetPosition(0, 0);
    this->Append(keyTextboxImg);

    kbText = new GuiText(content, 20, (GXColor){0, 0, 0, 0xff});
    kbText->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    kbText->SetPosition(0, 13);
    this->Append(kbText);
}

/**
 * Destructor for the GuiTextField class.
 */
GuiTextField::~GuiTextField() {
    delete kbText;
    delete keyTextbox;
}

void GuiTextField::SetText(std::wstring newText) { kbText->SetText(newText); }

void GuiTextField::Update(GuiTrigger *t) {
    if (_elements.size() == 0 || (state == STATE_DISABLED && parentElement))
        return;

    for (u8 i = 0; i < _elements.size(); i++) {
        try {
            _elements.at(i)->Update(t);
        } catch (const std::exception &e) {
        }
    }
}
