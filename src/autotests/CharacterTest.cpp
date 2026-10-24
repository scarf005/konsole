/*
    SPDX-FileCopyrightText: 2019 Tomaz Canabrava <tomaz.canabrava@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "CharacterTest.h"
#include "Character.h"
#include "KittyPlaceholder.h"

#include <QTest>
#include <cstdint>

using namespace Konsole;

static Character placeholderCell(std::u32string_view chars, CharacterColor foreground)
{
    Character cell(chars[0]);
    if (chars.size() > 1) {
        cell.character = ExtendedCharTable::instance.createExtendedChar(chars.data(), chars.size(), [] {
            return QSet<uint>();
        });
        cell.rendition.f.extended = 1;
    }
    cell.foregroundColor = foreground;
    return cell;
}

void CharacterTest::testKittyPlaceholderDecode()
{
    const CharacterColor rgb42(COLOR_SPACE_RGB, 42);
    const CharacterColor indexed7(COLOR_SPACE_256, 7);
    struct Expected {
        Character cell;
        quint32 imageId;
        int row, column;
    };
    const Expected line[] = {
        {placeholderCell(U"\U0010EEEE\u0305\u0305\u030E", rgb42), (2 << 24) | 42, 0, 0}, // row, column and id MSB
        {placeholderCell(U"\U0010EEEE", rgb42), (2 << 24) | 42, 0, 1}, // none: same row, next column, same MSB
        {placeholderCell(U"\U0010EEEE\u0305", rgb42), (2 << 24) | 42, 0, 2}, // row only, same row: next column, same MSB
        {placeholderCell(U"\U0010EEEE\u0305\u0312", rgb42), 42, 0, 4}, // row and column, column not adjacent: MSB 0
        {placeholderCell(U"\U0010EEEE\u030D", rgb42), 42, 1, 0}, // row only, different row: column 0
        {placeholderCell(U"\U0010EEEE\u030Dx", rgb42), 42, 1, 1}, // diacritics end at the first other character
        {placeholderCell(U"\U0010EEEE", indexed7), 7, 0, 0}, // different foreground color: nothing inherited
        {placeholderCell(U"x", indexed7), 0, 0, 0}, // not a placeholder
        {placeholderCell(U"\U0010EEEE", indexed7), 7, 0, 0}, // nothing to inherit from
        {placeholderCell(U"\U0010EEEE\u0305\u0305", CharacterColor(COLOR_SPACE_DEFAULT, 0)), 0, 0, 0}, // no image id
        {placeholderCell(U"\U0010EEEE\U0001D244\U0001D244", CharacterColor(COLOR_SPACE_RGB, 0xFFFFFF)), 0xFFFFFF, 296, 296}, // last diacritic
    };
    KittyPlaceholder cell;
    for (const Expected &e : line) {
        cell = KittyPlaceholder::decode(e.cell, cell);
        QCOMPARE(cell.imageId, e.imageId);
        if (e.imageId) {
            QCOMPARE(cell.row, e.row);
            QCOMPARE(cell.column, e.column);
        }
    }
}

QTEST_GUILESS_MAIN(Konsole::CharacterTest)

#include "moc_CharacterTest.cpp"
