// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#include "PnakoticController.h"

#include "RLyehPnakotic/Pnakotic.h"

#include <QByteArray>
#include <QString>

#include <cstddef>
#include <string_view>

namespace {

[[nodiscard]] QString FromAscii(std::string_view text)
{
  return QString::fromLatin1(text.data(), static_cast<qsizetype>(text.size()));
}

[[nodiscard]] std::string_view AsStringView(const QByteArray& text)
{
  return {text.constData(), static_cast<std::size_t>(text.size())};
}

[[nodiscard]] QVariantMap InvalidProjectResult()
{
  return {
    {QStringLiteral("valid"), false},
    {QStringLiteral("status"), QStringLiteral("Select a supported project.")},
  };
}

} // namespace

PnakoticController::PnakoticController(QObject* parent) : QObject(parent) {}

QStringList PnakoticController::projects() const
{
  QStringList names;
  names.reserve(static_cast<qsizetype>(RLyeh::Pnakotic::Projects::Supported.size()));
  for (const auto& project : RLyeh::Pnakotic::Projects::Supported)
    names.append(FromAscii(project.display_name));
  return names;
}

QVariantMap PnakoticController::decode(int project_index, const QString& version) const
{
  const RLyeh::Pnakotic::ProjectPolicy* project = ProjectAt(project_index);
  if (project == nullptr)
    return InvalidProjectResult();

  const QByteArray version_utf8 = version.trimmed().toUtf8();
  const RLyeh::Pnakotic::SCMLocatorResult decoded =
    RLyeh::Pnakotic::DecodeSCMVersion(*project, AsStringView(version_utf8));

  QVariantMap result = {
    {QStringLiteral("valid"), decoded.valid()},
    {QStringLiteral("status"), StatusText(decoded.status)},
  };
  if (decoded.valid())
  {
    result.insert(QStringLiteral("commitDate"), FromAscii(decoded.commit_date.view()));
    result.insert(QStringLiteral("commitPrefix"), FromAscii(decoded.commit_prefix.view()));
  }
  return result;
}

QVariantMap PnakoticController::encode(int project_index, const QString& commit_date,
                                       const QString& commit_hash) const
{
  const RLyeh::Pnakotic::ProjectPolicy* project = ProjectAt(project_index);
  if (project == nullptr)
    return InvalidProjectResult();

  const QByteArray date_utf8 = commit_date.trimmed().toUtf8();
  const QByteArray hash_utf8 = commit_hash.trimmed().toUtf8();
  const RLyeh::Pnakotic::SCMVersionResult encoded = RLyeh::Pnakotic::EncodeSCMVersion(
    *project, AsStringView(date_utf8), AsStringView(hash_utf8));

  QVariantMap result = {
    {QStringLiteral("valid"), encoded.valid()},
    {QStringLiteral("status"), StatusText(encoded.status)},
  };
  if (encoded.valid())
    result.insert(QStringLiteral("version"), FromAscii(encoded.version.view()));
  return result;
}

const RLyeh::Pnakotic::ProjectPolicy* PnakoticController::ProjectAt(int project_index)
{
  if (project_index < 0 ||
      static_cast<std::size_t>(project_index) >= RLyeh::Pnakotic::Projects::Supported.size())
    return nullptr;
  return &RLyeh::Pnakotic::Projects::Supported[static_cast<std::size_t>(project_index)];
}

QString PnakoticController::StatusText(RLyeh::Pnakotic::PolicyStatus status)
{
  using RLyeh::Pnakotic::PolicyStatus;
  switch (status)
  {
    case PolicyStatus::Valid:
      return QStringLiteral("Valid");
    case PolicyStatus::InvalidCommitDate:
      return QStringLiteral("Enter a valid commit date in YYYY-MM-DD form.");
    case PolicyStatus::InvalidEpoch:
      return QStringLiteral("The selected project has an invalid epoch.");
    case PolicyStatus::InvalidCommitHash:
      return QStringLiteral("Enter a hexadecimal commit hash containing at least six characters.");
    case PolicyStatus::DateDeltaOutOfRange:
      return QStringLiteral("The commit date is outside the selected project's supported range.");
    case PolicyStatus::MalformedPublicVersion:
      return QStringLiteral("Enter four period-separated version components.");
    case PolicyStatus::ComponentOutOfRange:
      return QStringLiteral("Each version component must be between -512 and 511.");
    case PolicyStatus::InvalidParity:
      return QStringLiteral("The public version failed its parity check.");
  }
  return QStringLiteral("Unknown Pnakotic status.");
}
