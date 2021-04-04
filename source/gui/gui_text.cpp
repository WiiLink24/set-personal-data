/****************************************************************************
 * libwiigui
 *
 * Tantric 2009
 *
 * gui_text.cpp
 *
 * GUI class definitions
 ***************************************************************************/

#include "gettext.h"
#include "gui.h"

static GXColor presetColor = (GXColor){255, 255, 255, 255};
static int currentSize = 0;
static int presetSize = 0;
static int presetMaxWidth = 0;
static int presetAlignmentHor = 0;
static int presetAlignmentVert = 0;
static u16 presetStyle = 0;

#define TEXT_SCROLL_DELAY 8
#define TEXT_SCROLL_INITIAL_DELAY 6

/**
 * Constructor for the GuiText class.
 */
GuiText::GuiText(std::wstring t, int s, GXColor c) {
    text = new std::wstring(t);
    size = s;
    color = c;
    alpha = c.a;
    style = FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE;
    maxWidth = 0;
    wrap = false;
    textDynNum = 0;
    textScroll = SCROLL_NONE;
    textScrollPos = 0;
    textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
    textScrollDelay = TEXT_SCROLL_DELAY;

    alignmentHor = ALIGN_CENTRE;
    alignmentVert = ALIGN_MIDDLE;

    for (int i = 0; i < 20; i++)
        textDyn[i] = NULL;
}

/**
 * Constructor for the GuiText class, uses presets
 */
GuiText::GuiText(std::wstring t) {
    text = new std::wstring(t);
    size = presetSize;
    color = presetColor;
    alpha = presetColor.a;
    style = presetStyle;
    maxWidth = presetMaxWidth;
    wrap = false;
    textDynNum = 0;
    textScroll = SCROLL_NONE;
    textScrollPos = 0;
    textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
    textScrollDelay = TEXT_SCROLL_DELAY;

    alignmentHor = presetAlignmentHor;
    alignmentVert = presetAlignmentVert;

    for (int i = 0; i < 20; i++)
        textDyn[i] = NULL;
}

/**
 * Destructor for the GuiText class.
 */
GuiText::~GuiText() {
    if (text)
        delete text;

    if (textDynNum > 0) {
        for (int i = 0; i < textDynNum; i++)
            if (textDyn[i])
                delete[] textDyn[i];
    }
}

void GuiText::SetText(std::wstring t) {
    if (text)
        delete text;

    if (textDynNum > 0) {
        for (int i = 0; i < textDynNum; i++)
            if (textDyn[i])
                delete[] textDyn[i];
    }

    text = new std::wstring(t);
    textDynNum = 0;
    textScrollPos = 0;
    textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
}

int GuiText::GetLength() {
    if (!text)
        return 0;

    return text->size();
}

void GuiText::SetPresets(int sz, GXColor c, int w, u16 s, int h, int v) {
    presetSize = sz;
    presetColor = c;
    presetStyle = s;
    presetMaxWidth = w;
    presetAlignmentHor = h;
    presetAlignmentVert = v;
}

void GuiText::SetFontSize(int s) { size = s; }

void GuiText::SetMaxWidth(int width) {
    maxWidth = width;

    for (int i = 0; i < textDynNum; i++) {
        if (textDyn[i]) {
            delete[] textDyn[i];
            textDyn[i] = NULL;
        }
    }

    textDynNum = 0;
}

int GuiText::GetTextWidth() {
    if (!text)
        return 0;

    if (currentSize != size) {
        ChangeFontSize(size);

        if (!fontSystem[size])
            fontSystem[size] = new FreeTypeGX(size);

        currentSize = size;
    }
    return fontSystem[size]->getWidth(text);
}

void GuiText::SetWrap(bool w, int width) {
    wrap = w;
    maxWidth = width;

    for (int i = 0; i < textDynNum; i++) {
        if (textDyn[i]) {
            delete[] textDyn[i];
            textDyn[i] = NULL;
        }
    }

    textDynNum = 0;
}

void GuiText::SetScroll(int s) {
    if (textScroll == s)
        return;

    for (int i = 0; i < textDynNum; i++) {
        if (textDyn[i]) {
            delete[] textDyn[i];
            textDyn[i] = NULL;
        }
    }

    textDynNum = 0;

    textScroll = s;
    textScrollPos = 0;
    textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
    textScrollDelay = TEXT_SCROLL_DELAY;
}

void GuiText::SetColor(GXColor c) {
    color = c;
    alpha = c.a;
}

void GuiText::SetStyle(u16 s) { style = s; }

void GuiText::SetAlignment(int hor, int vert) {
    style = 0;

    switch (hor) {
    case ALIGN_LEFT:
        style |= FTGX_JUSTIFY_LEFT;
        break;
    case ALIGN_RIGHT:
        style |= FTGX_JUSTIFY_RIGHT;
        break;
    default:
        style |= FTGX_JUSTIFY_CENTER;
        break;
    }
    switch (vert) {
    case ALIGN_TOP:
        style |= FTGX_ALIGN_TOP;
        break;
    case ALIGN_BOTTOM:
        style |= FTGX_ALIGN_BOTTOM;
        break;
    default:
        style |= FTGX_ALIGN_MIDDLE;
        break;
    }

    alignmentHor = hor;
    alignmentVert = vert;
}

void GuiText::ResetText() {
    if (text)
        delete text;

    for (int i = 0; i < textDynNum; i++) {
        if (textDyn[i]) {
            delete[] textDyn[i];
            textDyn[i] = NULL;
        }
    }

    textDynNum = 0;
    currentSize = 0;
}

/**
 * Draw the text on screen
 */
void GuiText::Draw() {
    if (!text)
        return;

    if (!this->IsVisible())
        return;

    GXColor c = color;
    c.a = this->GetAlpha();

    int newSize = size * this->GetScale();

    if (newSize > MAX_FONT_SIZE)
        newSize = MAX_FONT_SIZE;

    if (newSize != currentSize) {
        ChangeFontSize(newSize);
        if (!fontSystem[newSize])
            fontSystem[newSize] = new FreeTypeGX(newSize);
        currentSize = newSize;
    }

    if (maxWidth == 0) {
        fontSystem[currentSize]->drawText(this->GetLeft(), this->GetTop(), text,
                                          c, style);
        this->UpdateEffects();
        return;
    }

    // We convert from std::wstring to wchar_t
    // due to the complicated nature of the beneath code.
    // TODO: Find a way not to do this
    wchar_t *textCstr = (wchar_t*)text->c_str();
    u32 textlen = wcslen(textCstr);

    if (wrap) {
        if (textDynNum == 0) {
            u32 n = 0, ch = 0;
            int linenum = 0;
            int lastSpace = -1;
            int lastSpaceIndex = -1;

            while (ch < textlen && linenum < 20) {
                if (n == 0)
                    textDyn[linenum] = new wchar_t[textlen + 1];

                textDyn[linenum][n] = textCstr[ch];
                textDyn[linenum][n + 1] = 0;

                if (textCstr[ch] == ' ' || ch == textlen - 1) {
                    if (fontSystem[currentSize]->getWidth(textDyn[linenum]) >
                        maxWidth) {
                        if (lastSpace >= 0) {
                            // discard space, and everything after
                            textDyn[linenum][lastSpaceIndex] = '\0';
                            // go backwards to the last space
                            ch = lastSpace;
                            // we have used this space
                            lastSpace = -1;
                            lastSpaceIndex = -1;
                        }
                        ++linenum;
                        n = -1;
                    } else if (ch == textlen - 1) {
                        ++linenum;
                    }
                }
                if (textCstr[ch] == ' ' && n >= 0) {
                    lastSpace = ch;
                    lastSpaceIndex = n;
                }
                ++ch;
                ++n;
            }
            textDynNum = linenum;
        }

        int lineheight = newSize + 6;
        int voffset = 0;

        if (alignmentVert == ALIGN_MIDDLE)
            voffset = (lineheight >> 1) * (1 - textDynNum);

        int left = this->GetLeft();
        int top = this->GetTop() + voffset;

        for (int i = 0; i < textDynNum; ++i)
            fontSystem[currentSize]->drawText(left, top + i * lineheight,
                                              textDyn[i], c, style);
    } else {
        if (textDynNum == 0) {
            textDynNum = 1;
            textDyn[0] = wcsdup(textCstr);
            int len = wcslen(textDyn[0]);

            while (fontSystem[currentSize]->getWidth(textDyn[0]) > maxWidth)
                textDyn[0][--len] = 0;
        }

        if (textScroll == SCROLL_HORIZONTAL) {
            if (fontSystem[currentSize]->getWidth(text) > maxWidth &&
                (FrameTimer % textScrollDelay == 0)) {
                if (textScrollInitialDelay) {
                    --textScrollInitialDelay;
                } else {
                    ++textScrollPos;
                    if ((u32)textScrollPos > textlen - 1) {
                        textScrollPos = 0;
                        textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
                    }

                    wcscpy(textDyn[0], &textCstr[textScrollPos]);
                    u32 dynlen = wcslen(textDyn[0]);

                    if (dynlen + 2 < textlen) {
                        textDyn[0][dynlen] = ' ';
                        textDyn[0][dynlen + 1] = ' ';
                        textDyn[0][dynlen + 2] = 0;
                        dynlen += 2;
                    }

                    if (fontSystem[currentSize]->getWidth(textDyn[0]) >
                        maxWidth) {
                        while (fontSystem[currentSize]->getWidth(textDyn[0]) >
                               maxWidth)
                            textDyn[0][--dynlen] = 0;
                    } else {
                        int i = 0;

                        while (fontSystem[currentSize]->getWidth(textDyn[0]) <
                                   maxWidth &&
                               dynlen + 1 < textlen) {
                            textDyn[0][dynlen] = textCstr[i++];
                            textDyn[0][++dynlen] = 0;
                        }

                        if (fontSystem[currentSize]->getWidth(textDyn[0]) >
                            maxWidth)
                            textDyn[0][dynlen - 2] = 0;
                        else
                            textDyn[0][dynlen - 1] = 0;
                    }
                }
            }
        }
        fontSystem[currentSize]->drawText(this->GetLeft(), this->GetTop(),
                                          textDyn[0], c, style);
    }
    this->UpdateEffects();
}
