/*
    SPDX-FileCopyrightText: 2026 Konsole Developers

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KITTYPLACEHOLDER_H
#define KITTYPLACEHOLDER_H

#include "Character.h"

namespace Konsole
{
// Character of the cells showing a kitty graphics virtual placement, see
// https://sw.kovidgoyal.net/kitty/graphics-protocol/#unicode-placeholders
constexpr uint KittyPlaceholderChar = 0x10EEEE;

// The part of an image shown by a placeholder cell
struct KittyPlaceholder {
    quint32 imageId = 0; // 0 if the cell is no valid placeholder
    int row = 0;
    int column = 0;
    CharacterColor foreground;
    ExtraFlags underline = 0;

    // Decodes the cell to the right of previous. Omitted row, column and image id
    // diacritics are inherited from previous as the specification describes.
    static KittyPlaceholder decode(const Character &cell, const KittyPlaceholder &previous);
};
}

#endif // KITTYPLACEHOLDER_H
