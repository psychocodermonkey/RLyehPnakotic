// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#include "PnakoticController.h"

#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStringList>
#include <QVariant>

#include <cstdlib>

int main(int argc, char* argv[])
{
  QGuiApplication application(argc, argv);
  QCoreApplication::setOrganizationName(QStringLiteral("R'Lyeh"));
  QCoreApplication::setApplicationName(QStringLiteral("RLyehPnakotic"));
  QCoreApplication::setApplicationVersion(QStringLiteral("0.0.0"));
  QGuiApplication::setApplicationDisplayName(QStringLiteral("R'Lyeh Pnakotic"));

  const int font_id =
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Metamorphous-Regular.ttf"));
  if (font_id >= 0)
  {
    const QStringList families = QFontDatabase::applicationFontFamilies(font_id);
    if (!families.isEmpty())
      application.setFont(QFont(families.constFirst()));
  }

  PnakoticController controller;
  QQmlApplicationEngine engine;
  engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)}});
  QObject::connect(
    &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
    [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
  engine.loadFromModule(QStringLiteral("RLyeh.Pnakotic"), QStringLiteral("Main"));

  return application.exec();
}
