#include "turbineLauncher.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <algorithm>

namespace {

const QString glsNamespace = QStringLiteral("http://www.turbine.com/SE/GLS");
const QString soapNamespace = QStringLiteral("http://schemas.xmlsoap.org/soap/envelope/");
constexpr int requestTimeoutMilliseconds = 30000;

struct StepError {
  QString message;
};

QDomDocument parseXml(const QByteArray& text, const QString& what)
{
  QDomDocument document;
  const QDomDocument::ParseResult result = document.setContent(text, QDomDocument::ParseOption::UseNamespaceProcessing);
  if (!result) {
    throw StepError{QStringLiteral("%1 is not XML: %2").arg(what, result.errorMessage)};
  }
  return document;
}

// The key=value pairs of an appSettings document: the launcher's local and server-side configuration.
QMap<QString, QString> readAppSettings(const QByteArray& text, const QString& what)
{
  const QDomDocument document = parseXml(text, what);
  QMap<QString, QString> settings;
  const QDomNodeList entries = document.documentElement().firstChildElement(QStringLiteral("appSettings"))
    .elementsByTagName(QStringLiteral("add"));
  for (int index = 0; index < entries.size(); ++index) {
    const QDomElement entry = entries.at(index).toElement();
    if (!entry.hasAttribute(QStringLiteral("key")) || !entry.hasAttribute(QStringLiteral("value"))) {
      throw StepError{QStringLiteral("%1 has an appSettings entry without key or value").arg(what)};
    }
    settings.insert(entry.attribute(QStringLiteral("key")), entry.attribute(QStringLiteral("value")));
  }
  if (settings.isEmpty()) {
    throw StepError{QStringLiteral("%1 has no appSettings entries").arg(what)};
  }
  return settings;
}

QString requireSetting(const QMap<QString, QString>& settings, const QString& key, const QString& source)
{
  if (!settings.contains(key)) {
    throw StepError{QStringLiteral("%1 has no %2").arg(source, key)};
  }
  return settings.value(key);
}

QByteArray soapEnvelope(const QString& operation, const QList<QPair<QString, QString>>& arguments)
{
  QDomDocument document;
  QDomElement envelope = document.createElementNS(soapNamespace, QStringLiteral("Envelope"));
  document.appendChild(envelope);
  QDomElement body = document.createElementNS(soapNamespace, QStringLiteral("Body"));
  envelope.appendChild(body);
  QDomElement call = document.createElementNS(glsNamespace, operation);
  body.appendChild(call);
  for (const auto& [name, value] : arguments) {
    QDomElement argument = document.createElementNS(glsNamespace, name);
    argument.appendChild(document.createTextNode(value));
    call.appendChild(argument);
  }
  return "<?xml version=\"1.0\" encoding=\"utf-8\"?>" + document.toByteArray(-1);
}

QString soapFault(const QByteArray& body)
{
  QDomDocument document;
  if (!document.setContent(body, QDomDocument::ParseOption::UseNamespaceProcessing)) {
    return QString();
  }
  const QDomNodeList faults = document.elementsByTagNameNS(soapNamespace, QStringLiteral("Fault"));
  if (faults.isEmpty()) {
    return QString();
  }
  const QString text = faults.at(0).toElement().firstChildElement(QStringLiteral("faultstring")).text().trimmed();
  return text.isEmpty() ? QStringLiteral("the service answered with a fault without a message") : text;
}

QDomElement soapResult(const QByteArray& answer, const QString& operation, const QUrl& service)
{
  QDomDocument document = parseXml(answer, QStringLiteral("the answer of %1").arg(service.toString()));
  const QDomNodeList results = document.elementsByTagNameNS(glsNamespace, operation + QStringLiteral("Response"));
  if (results.isEmpty()) {
    throw StepError{QStringLiteral("%1 answered %2 without a %2Response").arg(service.toString(), operation)};
  }
  return results.at(0).toElement();
}

QString glsText(const QDomElement& element, const QString& name)
{
  const QDomElement child = element.firstChildElement(name);
  if (child.isNull()) {
    throw StepError{QStringLiteral("the service's answer lacks %1").arg(name)};
  }
  return child.text();
}

QStringList splitList(const QString& text)
{
  return text.split(QLatin1Char(';'), Qt::SkipEmptyParts);
}

qint64 hexNumber(const QString& text, const QString& what)
{
  bool isNumber = false;
  const qint64 value = text.trimmed().isEmpty() ? 0 : text.trimmed().toLongLong(&isNumber, 16);
  if (!text.trimmed().isEmpty() && !isNumber) {
    throw StepError{QStringLiteral("%1 is not a hexadecimal number: %2").arg(what, text)};
  }
  return value;
}

QUrl turbineSecureUrl(const QString& text)
{
  QUrl url(text, QUrl::StrictMode);
  if (!url.isValid() || (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https"))) {
    throw StepError{QStringLiteral("%1 is not a web address").arg(text)};
  }
  const bool isLoopback = url.host() == QLatin1String("127.0.0.1") || url.host() == QLatin1String("localhost");
  if (!isLoopback) {
    url.setScheme(QStringLiteral("https"));
  }
  return url;
}

}

TurbineLauncher::TurbineLauncher(QNetworkAccessManager* network, QObject* parent)
  : QObject(parent), network(network)
{
}

void TurbineLauncher::fetch(const QUrl& url, const QByteArray& body, const QString& contentType, const QString& soapAction,
  const std::function<void(const QByteArray&)>& onAnswer)
{
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Ankh"));
  request.setTransferTimeout(requestTimeoutMilliseconds);
  if (!soapAction.isEmpty()) {
    request.setRawHeader("SOAPAction", soapAction.toUtf8());
  }
  QNetworkReply* reply = nullptr;
  if (body.isNull()) {
    reply = network->get(request);
  } else {
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    reply = network->post(request, body);
  }
  connect(reply, &QNetworkReply::finished, this, [this, reply, url, onAnswer] {
    reply->deleteLater();
    const QByteArray answer = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
      const QString fault = soapFault(answer);
      emit failed(fault.isEmpty() ? QStringLiteral("%1 could not be reached: %2").arg(url.toString(), reply->errorString()) : fault);
      return;
    }
    try {
      onAnswer(answer);
    } catch (const StepError& error) {
      emit failed(error.message);
    }
  });
}

bool TurbineLauncher::readLauncherConfig(const QString& gameFolder, const QString& launcherConfig, LauncherConfig& config)
{
  QFile file(QDir(gameFolder).filePath(launcherConfig));
  if (!file.open(QIODevice::ReadOnly)) {
    emit failed(QStringLiteral("%1 could not be read: %2").arg(QDir::toNativeSeparators(file.fileName()), file.errorString()));
    return false;
  }
  try {
    const QMap<QString, QString> settings = readAppSettings(file.readAll(), launcherConfig);
    config.glsService = turbineSecureUrl(requireSetting(settings, QStringLiteral("Launcher.DataCenterService.GLS"), launcherConfig));
    config.gameName = requireSetting(settings, QStringLiteral("DataCenter.GameName"), launcherConfig);
  } catch (const StepError& error) {
    emit failed(error.message);
    return false;
  }
  return true;
}

void TurbineLauncher::loadDatacenter(const LauncherConfig& config, const std::function<void(const Datacenter&)>& onLoaded)
{
  const QString operation = QStringLiteral("GetDatacenters");
  fetch(config.glsService, soapEnvelope(operation, {{QStringLiteral("game"), config.gameName}}),
    QStringLiteral("text/xml; charset=utf-8"), glsNamespace + QLatin1Char('/') + operation,
    [this, config, operation, onLoaded](const QByteArray& answer) {
      const QDomElement result = soapResult(answer, operation, config.glsService);
      const QDomNodeList centers = result.firstChildElement(QStringLiteral("GetDatacentersResult"))
        .elementsByTagName(QStringLiteral("Datacenter"));
      QList<QDomElement> matching;
      for (int index = 0; index < centers.size(); ++index) {
        if (glsText(centers.at(index).toElement(), QStringLiteral("Name")) == config.gameName) {
          matching.append(centers.at(index).toElement());
        }
      }
      if (matching.size() != 1) {
        throw StepError{QStringLiteral("GetDatacenters returned %1 datacenters named %2").arg(matching.size()).arg(config.gameName)};
      }
      Datacenter center;
      center.authServer = turbineSecureUrl(glsText(matching.first(), QStringLiteral("AuthServer")));
      center.serverConfigUrl = turbineSecureUrl(glsText(matching.first(), QStringLiteral("LauncherConfigurationServer")));
      const QDomNodeList worlds = matching.first().firstChildElement(QStringLiteral("Worlds")).elementsByTagName(QStringLiteral("World"));
      for (int index = 0; index < worlds.size(); ++index) {
        const QDomElement world = worlds.at(index).toElement();
        center.worlds.append({glsText(world, QStringLiteral("Name")), glsText(world, QStringLiteral("ChatServerUrl")),
          turbineSecureUrl(glsText(world, QStringLiteral("StatusServerUrl"))), glsText(world, QStringLiteral("Order")).toInt()});
      }
      if (center.worlds.isEmpty()) {
        throw StepError{QStringLiteral("GetDatacenters returned no worlds")};
      }
      std::sort(center.worlds.begin(), center.worlds.end(), [](const TurbineWorld& left, const TurbineWorld& right) {
        return left.order < right.order;
      });
      onLoaded(center);
    });
}

void TurbineLauncher::listWorlds(const QString& gameFolder, const QString& launcherConfig)
{
  LauncherConfig config;
  if (!readLauncherConfig(gameFolder, launcherConfig, config)) {
    return;
  }
  loadDatacenter(config, [this](const Datacenter& center) { emit worldsListed(center.worlds); });
}

void TurbineLauncher::prepareLaunch(const QString& gameFolder, const QString& launcherConfig, const QString& worldName,
  const QString& username, const QString& password, const QString& language)
{
  LauncherConfig config;
  if (!readLauncherConfig(gameFolder, launcherConfig, config)) {
    return;
  }
  emit statusChanged(QStringLiteral("Asking %1 for its worlds...").arg(config.gameName));
  loadDatacenter(config, [this, config, worldName, username, password, language](const Datacenter& center) {
    const auto world = std::find_if(center.worlds.cbegin(), center.worlds.cend(), [&worldName](const TurbineWorld& entry) {
      return entry.name == worldName;
    });
    if (world == center.worlds.cend()) {
      QStringList names;
      for (const TurbineWorld& entry : center.worlds) {
        names.append(entry.name);
      }
      throw StepError{QStringLiteral("There is no world named %1; the worlds are %2.").arg(worldName, names.join(QStringLiteral(", ")))};
    }
    const TurbineWorld chosen = *world;
    emit statusChanged(QStringLiteral("Reading the launcher configuration..."));
    fetch(center.serverConfigUrl, QByteArray(), QString(), QString(),
      [this, config, center, chosen, username, password, language](const QByteArray& answer) {
        const QMap<QString, QString> serverConfig = readAppSettings(answer, QStringLiteral("the launcher's server configuration"));
        const QString source = QStringLiteral("the launcher's server configuration");
        const QString clientFile = requireSetting(serverConfig, QStringLiteral("GameClient.WIN64.Filename"), source);
        const QString argumentTemplate = requireSetting(serverConfig, QStringLiteral("GameClient.WIN32.ArgTemplate"), source);
        const QUrl queueUrl = turbineSecureUrl(requireSetting(serverConfig, QStringLiteral("WorldQueue.LoginQueue.URL"), source));
        const QString queueTemplate = requireSetting(serverConfig, QStringLiteral("WorldQueue.TakeANumber.Parameters"), source);
        emit statusChanged(QStringLiteral("Signing %1 in...").arg(username));
        const QString loginOperation = QStringLiteral("LoginAccount");
        fetch(center.authServer, soapEnvelope(loginOperation, {{QStringLiteral("username"), username},
            {QStringLiteral("password"), password}, {QStringLiteral("additionalInfo"), QString()}}),
          QStringLiteral("text/xml; charset=utf-8"), glsNamespace + QLatin1Char('/') + loginOperation,
          [this, config, center, chosen, username, serverConfig, clientFile, argumentTemplate, queueUrl, queueTemplate, language,
            loginOperation](const QByteArray& answer) {
            const QDomElement profile = soapResult(answer, loginOperation, center.authServer)
              .firstChildElement(QStringLiteral("LoginAccountResult"));
            if (profile.isNull()) {
              throw StepError{QStringLiteral("LoginAccount answered without a user profile")};
            }
            const QString ticket = glsText(profile, QStringLiteral("Ticket"));
            if (ticket.isEmpty()) {
              throw StepError{QStringLiteral("LoginAccount answered with an empty ticket")};
            }
            QStringList subscriptions;
            const QDomNodeList entries = profile.firstChildElement(QStringLiteral("Subscriptions"))
              .elementsByTagName(QStringLiteral("GameSubscription"));
            for (int index = 0; index < entries.size(); ++index) {
              const QDomElement entry = entries.at(index).toElement();
              if (glsText(entry, QStringLiteral("Game")) == config.gameName) {
                subscriptions.append(glsText(entry, QStringLiteral("Name")));
              }
            }
            if (subscriptions.size() != 1) {
              throw StepError{QStringLiteral("%1 has %2 %3 subscriptions; launching needs exactly one.")
                .arg(username).arg(subscriptions.size()).arg(config.gameName)};
            }
            const QString subscription = subscriptions.first();
            emit statusChanged(QStringLiteral("Checking %1...").arg(chosen.name));
            fetch(chosen.statusUrl, QByteArray(), QString(), QString(),
              [this, chosen, subscription, ticket, serverConfig, clientFile, argumentTemplate, queueUrl, queueTemplate, language](const QByteArray& answer) {
                if (answer.trimmed().isEmpty()) {
                  throw StepError{QStringLiteral("%1 answered nothing about its status: the world is unavailable.").arg(chosen.name)};
                }
                const QDomElement status = parseXml(answer, QStringLiteral("the status of %1").arg(chosen.name)).documentElement();
                if (status.tagName() != QLatin1String("Status")) {
                  throw StepError{QStringLiteral("the status of %1 is a %2, not a Status").arg(chosen.name, status.tagName())};
                }
                const QStringList loginServers = splitList(status.firstChildElement(QStringLiteral("loginservers")).text());
                const QStringList queueUrls = splitList(status.firstChildElement(QStringLiteral("queueurls")).text());
                if (loginServers.isEmpty() || queueUrls.isEmpty()) {
                  throw StepError{QStringLiteral("%1 names no login server or no queue: the world is unavailable.").arg(chosen.name)};
                }
                if (status.firstChildElement(QStringLiteral("world_full")).text().toLower() == QLatin1String("true")) {
                  throw StepError{QStringLiteral("%1 is full right now.").arg(chosen.name)};
                }
                emit statusChanged(QStringLiteral("Taking a place in the login queue of %1...").arg(chosen.name));
                QUrlQuery form;
                for (const QString& parameter : queueTemplate.split(QLatin1Char('&'))) {
                  const qsizetype separator = parameter.indexOf(QLatin1Char('='));
                  if (separator < 0) {
                    throw StepError{QStringLiteral("the queue parameter template has a parameter without a value: %1").arg(parameter)};
                  }
                  QString value = parameter.mid(separator + 1);
                  value.replace(QStringLiteral("{0}"), subscription).replace(QStringLiteral("{1}"), ticket)
                    .replace(QStringLiteral("{2}"), queueUrls.first());
                  form.addQueryItem(QString::fromUtf8(QUrl::toPercentEncoding(parameter.left(separator))),
                    QString::fromUtf8(QUrl::toPercentEncoding(value)));
                }
                fetch(queueUrl, form.query(QUrl::FullyEncoded).toUtf8(), QStringLiteral("application/x-www-form-urlencoded"), QString(),
                  [this, chosen, subscription, ticket, loginServers, serverConfig, clientFile, argumentTemplate, language](const QByteArray& answer) {
                    const QDomElement result = parseXml(answer, QStringLiteral("the login queue's answer")).documentElement();
                    if (result.tagName() != QLatin1String("Result")) {
                      throw StepError{QStringLiteral("the login queue answered with a %1, not a Result").arg(result.tagName())};
                    }
                    const QDomElement hresult = result.firstChildElement(QStringLiteral("HResult"));
                    if (hresult.isNull()) {
                      throw StepError{QStringLiteral("the login queue's answer has no HResult")};
                    }
                    const qint64 code = hexNumber(hresult.text(), QStringLiteral("HResult"));
                    if (code & 0x80000000) {
                      throw StepError{QStringLiteral("The login queue refused the account (HRESULT 0x%1).")
                        .arg(QString::number(code, 16).rightJustified(8, QLatin1Char('0')).toUpper())};
                    }
                    const QMap<QString, QString> values = {
                      {QStringLiteral("{SUBSCRIPTION}"), subscription}, {QStringLiteral("{LOGIN}"), loginServers.first()},
                      {QStringLiteral("{GLS}"), ticket}, {QStringLiteral("{CHAT}"), chosen.chatServer}, {QStringLiteral("{LANG}"), language},
                      {QStringLiteral("{AUTHSERVERURL}"), serverConfig.value(QStringLiteral("GameClient.Arg.authserverurl"))},
                      {QStringLiteral("{GLSTICKETLIFETIME}"), serverConfig.value(QStringLiteral("GameClient.Arg.glsticketlifetime"))},
                      {QStringLiteral("{CRASHRECEIVER}"), serverConfig.value(QStringLiteral("GameClient.Arg.crashreceiver"))},
                      {QStringLiteral("{SUPPORTURL}"), serverConfig.value(QStringLiteral("GameClient.Arg.supporturl"))},
                      {QStringLiteral("{SUPPORTSERVICEURL}"), serverConfig.value(QStringLiteral("GameClient.Arg.supportserviceurl"))},
                      {QStringLiteral("{BUGURL}"), serverConfig.value(QStringLiteral("GameClient.Arg.bugurl"))},
                    };
                    QStringList arguments;
                    for (const QString& word : argumentTemplate.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
                      if (word.startsWith(QLatin1Char('{')) && word.endsWith(QLatin1Char('}'))) {
                        if (!values.contains(word)) {
                          throw StepError{QStringLiteral("the argument template uses %1, which is not known").arg(word)};
                        }
                        if (values.value(word).isEmpty()) {
                          throw StepError{QStringLiteral("the argument template uses %1, which the server configuration does not provide").arg(word)};
                        }
                        arguments.append(values.value(word));
                      } else if (word.contains(QLatin1Char('{'))) {
                        throw StepError{QStringLiteral("the argument template has a placeholder inside a word: %1").arg(word)};
                      } else {
                        arguments.append(word);
                      }
                    }
                    emit launchReady(clientFile, arguments);
                  });
              });
          });
      });
  });
}
