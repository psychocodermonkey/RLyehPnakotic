// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>

namespace RLyeh::Pnakotic {
struct ProjectPolicy;
enum class PolicyStatus;
}

class PnakoticController final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList projects READ projects CONSTANT)

public:
  explicit PnakoticController(QObject* parent = nullptr);

  [[nodiscard]] QStringList projects() const;

  Q_INVOKABLE [[nodiscard]] QVariantMap decode(int project_index, const QString& version) const;
  Q_INVOKABLE [[nodiscard]] QVariantMap encode(int project_index, const QString& commit_date,
                                               const QString& commit_hash) const;

private:
  [[nodiscard]] static const RLyeh::Pnakotic::ProjectPolicy* ProjectAt(int project_index);
  [[nodiscard]] static QString StatusText(RLyeh::Pnakotic::PolicyStatus status);
};
