/*
    SPDX-FileCopyrightText: 2013 Kurt Hindenburg <kurt.hindenburg@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "TerminalTest.h"

#include <QAction>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QKeyEvent>
#include <QMimeData>
#include <QTest>

#include <KActionCollection>
#include <KShell>

#include <memory>

// Konsole
#include "../terminalDisplay/TerminalColor.h"
#include "../terminalDisplay/TerminalDisplay.h"
#include "../terminalDisplay/TerminalScrollBar.h"

#include "../Emulation.h"
#include "../characters/CharacterColor.h"
#include "../colorscheme/ColorScheme.h"
#include "../profile/ProfileManager.h"
#include "../session/Session.h"
#include "../session/SessionController.h"
#include "../session/SessionManager.h"

using namespace Konsole;

namespace
{
class PasteTerminal
{
public:
    PasteTerminal()
    {
        display.setWallpaper(ColorScheme().wallpaper());
        QObject::disconnect(session.emulation(), &Emulation::imageSizeInitialized, &session, &Session::run);
        SessionManager::instance()->setSessionProfile(&session, ProfileManager::instance()->defaultProfile());
        controller = std::make_unique<SessionController>(&session, &display, nullptr);
        session.addView(&display);
        QObject::connect(&display, &TerminalDisplay::keyPressedSignal, &display, [this](QKeyEvent *event) {
            if (event->type() == QEvent::KeyPress && event->key() == 0) {
                pasted = event->text();
            }
        });
    }

    ~PasteTerminal()
    {
        display.clearFocus();
        display.hide();
        display.setScreenWindow(nullptr);
    }

    void paste()
    {
        controller->actionCollection()->action(QStringLiteral("edit_paste"))->trigger();
    }

    TerminalDisplay display{nullptr};
    Session session{nullptr};
    std::unique_ptr<SessionController> controller;
    QString pasted;
};
} // namespace

void TerminalTest::testScrollBarPositions()
{
    TerminalDisplay display(nullptr);

    // ScrollBar Positions
    display.scrollBar()->setScrollBarPosition(Enum::ScrollBarLeft);
    QCOMPARE(display.scrollBar()->scrollBarPosition(), Enum::ScrollBarLeft);
    display.scrollBar()->setScrollBarPosition(Enum::ScrollBarRight);
    QCOMPARE(display.scrollBar()->scrollBarPosition(), Enum::ScrollBarRight);
    display.scrollBar()->setScrollBarPosition(Enum::ScrollBarHidden);
    QCOMPARE(display.scrollBar()->scrollBarPosition(), Enum::ScrollBarHidden);
}

void TerminalTest::testColorTable()
{
    // These are from ColorScheme.cpp but they can be anything to test
    const QColor defaultTable[TABLE_COLORS] = {QColor(0x00, 0x00, 0x00), QColor(0xFF, 0xFF, 0xFF), QColor(0x00, 0x00, 0x00), QColor(0xB2, 0x18, 0x18),
                                               QColor(0x18, 0xB2, 0x18), QColor(0xB2, 0x68, 0x18), QColor(0x18, 0x18, 0xB2), QColor(0xB2, 0x18, 0xB2),
                                               QColor(0x18, 0xB2, 0xB2), QColor(0xB2, 0xB2, 0xB2), QColor(0x00, 0x00, 0x00), QColor(0xFF, 0xFF, 0xFF),
                                               QColor(0x68, 0x68, 0x68), QColor(0xFF, 0x54, 0x54), QColor(0x54, 0xFF, 0x54), QColor(0xFF, 0xFF, 0x54),
                                               QColor(0x54, 0x54, 0xFF), QColor(0xFF, 0x54, 0xFF), QColor(0x54, 0xFF, 0xFF), QColor(0x00, 0xFF, 0xFF)};

    TerminalDisplay display(nullptr);

    display.terminalColor()->setColorTable(defaultTable);

    const QColor *colorTable = display.terminalColor()->colorTable();

    for (int i = 0; i < TABLE_COLORS; i++) {
        QCOMPARE(colorTable[i], defaultTable[i]);
    }

    QColor colorEntry = QColor(0x00, 0x00, 0x00);
    QVERIFY(colorTable[1] != colorEntry);
}

void TerminalTest::testSize()
{
    TerminalDisplay display(nullptr);

    QCOMPARE(display.columns(), 1);
    QCOMPARE(display.lines(), 1);

    // TODO: setSize doesn't change size...
    // display.setSize(80, 25);
}

void TerminalTest::testClipboardImage()
{
    QImage image(3, 2, QImage::Format_ARGB32);
    image.fill(QColor(20, 40, 60, 80));
    image.setPixelColor(1, 1, QColor(120, 140, 160, 180));
    QApplication::clipboard()->setImage(image);

    QString firstPath;
    QString secondPath;
    {
        PasteTerminal terminal;
        terminal.paste();
        const QStringList paths = KShell::splitArgs(terminal.pasted);
        QCOMPARE(paths.size(), 1);
        firstPath = paths.constFirst();
        QVERIFY(firstPath.endsWith(QLatin1String(".png")));
        QCOMPARE(QImage(firstPath).convertToFormat(QImage::Format_ARGB32), image);

        const auto publicPermissions =
            QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
        QVERIFY(!(QFileInfo(firstPath).permissions() & publicPermissions));
        QVERIFY(!(QFileInfo(QFileInfo(firstPath).absolutePath()).permissions() & publicPermissions));

        terminal.display.setBracketedPasteMode(true);
        terminal.paste();
        QVERIFY(terminal.pasted.startsWith(QLatin1String("\033[200~")));
        QVERIFY(terminal.pasted.endsWith(QLatin1String("\033[201~")));
        const QStringList secondPaths = KShell::splitArgs(terminal.pasted.mid(6, terminal.pasted.size() - 12));
        QCOMPARE(secondPaths.size(), 1);
        secondPath = secondPaths.constFirst();
        QVERIFY(secondPath != firstPath);
        QCOMPARE(QImage(secondPath).convertToFormat(QImage::Format_ARGB32), image);
    }
    QVERIFY(QFileInfo::exists(firstPath));
    QVERIFY(QFileInfo::exists(secondPath));
    QVERIFY(QFile::remove(firstPath));
    QVERIFY(QFile::remove(secondPath));
}

void TerminalTest::testClipboardImageShortcut()
{
    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QApplication::clipboard()->setImage(image);

    PasteTerminal terminal;
    QAction *pasteAction = terminal.controller->actionCollection()->action(QStringLiteral("edit_paste"));
    pasteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
    terminal.display.addAction(pasteAction);
    terminal.display.show();
    terminal.display.activateWindow();
    terminal.display.setFocus();
    QVERIFY(QTest::qWaitForWindowActive(&terminal.display));
    QVERIFY(terminal.display.hasFocus());
    QVERIFY(pasteAction->isEnabled());
    QTest::keyClick(terminal.display.windowHandle(), Qt::Key_V, Qt::ControlModifier);

    const QStringList paths = KShell::splitArgs(terminal.pasted);
    QCOMPARE(paths.size(), 1);
    QCOMPARE(QImage(paths.constFirst()).convertToFormat(QImage::Format_RGB32), image);
    QVERIFY(QFile::remove(paths.constFirst()));
}

void TerminalTest::testClipboardImageWithText()
{
    auto mimeData = new QMimeData;
    mimeData->setImageData(QImage(1, 1, QImage::Format_RGB32));
    mimeData->setText(QStringLiteral("existing text"));
    QApplication::clipboard()->setMimeData(mimeData);

    PasteTerminal terminal;
    terminal.paste();
    QCOMPARE(terminal.pasted, QStringLiteral("existing text"));
}

void TerminalTest::testClipboardImageWithUrls()
{
    auto mimeData = new QMimeData;
    mimeData->setImageData(QImage(1, 1, QImage::Format_RGB32));
    const QString path = QDir::tempPath() + QStringLiteral("/file with spaces.png");
    mimeData->setUrls({QUrl::fromLocalFile(path)});
    QApplication::clipboard()->setMimeData(mimeData);

    PasteTerminal terminal;
    terminal.paste();
    QCOMPARE(terminal.pasted, KShell::quoteArg(path) + QLatin1Char(' '));
}

void TerminalTest::testClipboardImageReadOnly()
{
    QApplication::clipboard()->setImage(QImage(1, 1, QImage::Format_RGB32));
    PasteTerminal terminal;
    terminal.display.updateReadOnlyState(true);
    terminal.display.pasteFromClipboard();
    QVERIFY(terminal.pasted.isEmpty());
}

void TerminalTest::testClipboardInvalidImage()
{
    auto mimeData = new QMimeData;
    mimeData->setImageData(QImage());
    QApplication::clipboard()->setMimeData(mimeData);

    PasteTerminal terminal;
    terminal.display.pasteFromClipboard(true);
    QVERIFY(terminal.pasted.isEmpty());
}

QTEST_MAIN(TerminalTest)

#include "moc_TerminalTest.cpp"
