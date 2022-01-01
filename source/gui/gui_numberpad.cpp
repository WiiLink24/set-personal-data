/****************************************************************************
 * libwiigui
 *
 * Sketch 2021
 *
 * gui_numberpad.cpp
 *
 * GUI class definitions
 ***************************************************************************/

#include "gui.h"
#include <unistd.h>

GuiNumberpad::GuiNumberpad(wchar_t *t, u32 max) {
    width = 540;
    height = 400;
    selectable = true;
    focus = 0;
    alignmentHor = ALIGN_CENTRE;
    alignmentVert = ALIGN_MIDDLE;
    kbtextmaxlen = max;

    numKey thekeys[4][3] = {{{"1"}, {"2"}, {"3"}},
                            {{"4"}, {"5"}, {"6"}},
                            {{"7"}, {"8"}, {"9"}},
                            {{"\0"}, {"0"}, {"\0"}}};

    /// For some reason when copying to 'txt' array, it was adding 'm' to the numbers
    /// This is a very hacky fix. TODO: Use only 1 array
    padKey thePadKeys[4][3] = {{{'1'}, {'2'}, {'3'}},
                             {{'4'}, {'5'}, {'6'}},
                             {{'7'}, {'8'}, {'9'}},
                             {{'\0'}, {'0'}, {'\0'}}};

    memcpy(padKeys, thePadKeys, sizeof(thekeys));
    memcpy(numKeys, thekeys, sizeof(thekeys));

    swprintf(kbtextstr, 255, L"%ls", t);
    kbTextfield = new GuiTextField(kbtextstr, max);
    this->Append(kbTextfield);

    trigA = new GuiTrigger;
    trigA->SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                            PAD_BUTTON_A);

    trig2 = new GuiTrigger;
    trig2->SetSimpleTrigger(-1, WPAD_BUTTON_2, 0);

    keyMedium = new GuiImageData(keyboard_mediumkey_png);
    keyMediumOver = new GuiImageData(keyboard_mediumkey_over_png);
    key = new GuiImageData(keyboard_key_png);
    keyOver = new GuiImageData(keyboard_key_over_png);

    keyBackImg = new GuiImage(keyMedium);
    keyBackOverImg = new GuiImage(keyMediumOver);
    keyBackText = new GuiText("Back", 20, (GXColor){0, 0, 0, 0xff});
    keyBack = new GuiButton(keyMedium->GetWidth(), keyMedium->GetHeight());
    keyBack->SetImage(keyBackImg);
    keyBack->SetImageOver(keyBackOverImg);
    keyBack->SetLabel(keyBackText);
    keyBack->SetTrigger(trigA);
    keyBack->SetTrigger(trig2);
    keyBack->SetPosition(3 * 41 + 200, 120);
    keyBack->SetEffectGrow();
    this->Append(keyBack);

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            if (padKeys[i][j].num != L'\0') {
                keyImg[i][j] = new GuiImage(key);
                keyImgOver[i][j] = new GuiImage(keyOver);
                keyTxt[i][j] = new GuiText(numKeys[i][j].num, 20, (GXColor){0, 0, 0, 0xff});
                keyTxt[i][j]->SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
                keyTxt[i][j]->SetPosition(0, -10);
                keyBtn[i][j] = new GuiButton(key->GetWidth(), key->GetHeight());
                keyBtn[i][j]->SetImage(keyImg[i][j]);
                keyBtn[i][j]->SetImageOver(keyImgOver[i][j]);
                keyBtn[i][j]->SetTrigger(trigA);
                keyBtn[i][j]->SetTrigger(trig2);
                keyBtn[i][j]->SetLabel(keyTxt[i][j]);
                keyBtn[i][j]->SetPosition(j * 42 + 200, i * 42 + 120);
                keyBtn[i][j]->SetEffectGrow();
                this->Append(keyBtn[i][j]);
            }
        }
    }
}

GuiNumberpad::~GuiNumberpad() {
    delete kbTextfield;
    delete key;
    delete keyOver;
    delete keyMedium;
    delete keyMediumOver;
    delete keyBackText;
    delete keyBackImg;
    delete keyBackOverImg;
    delete keyBack;
    delete trigA;
    delete trig2;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            if (padKeys[i][j].num != L'\0') {
                delete keyImg[i][j];
                delete keyImgOver[i][j];
                delete keyTxt[i][j];
                delete keyBtn[i][j];
            }
        }
    }
}

void GuiNumberpad::Update(GuiTrigger *t) {
    if (_elements.size() == 0 || (state == STATE_DISABLED && parentElement))
        return;

    for (u8 i = 0; i < _elements.size(); i++) {
        try {
            _elements.at(i)->Update(t);
        } catch (const std::exception &e) {
        }
    }

    if (keyBack->GetState() == STATE_CLICKED) {
        if (wcslen(kbtextstr) > 0) {
            kbtextstr[wcslen(kbtextstr) - 1] = 0;
            kbTextfield->SetText(kbtextstr);
        }
        keyBack->SetState(STATE_SELECTED, t->chan);
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            if (padKeys[i][j].num != L'\0') {
                if (keyBtn[i][j]->GetState() == STATE_CLICKED) {
                    size_t len = wcslen(kbtextstr);

                    if (len < kbtextmaxlen - 1) {
                        kbtextstr[len] = padKeys[i][j].num;
                        kbtextstr[len + 1] = L'\0';
                    }

                    kbTextfield->SetText(kbtextstr);
                    keyBtn[i][j]->SetState(STATE_SELECTED, t->chan);
                }
            }
        }
    }

    this->ToggleFocus(t);

    if (focus) // only send actions to this window if it's in focus
    {
        // pad/joystick navigation
        if (t->Right())
            this->MoveSelectionHor(1);
        else if (t->Left())
            this->MoveSelectionHor(-1);
        else if (t->Down())
            this->MoveSelectionVert(1);
        else if (t->Up())
            this->MoveSelectionVert(-1);
    }
}